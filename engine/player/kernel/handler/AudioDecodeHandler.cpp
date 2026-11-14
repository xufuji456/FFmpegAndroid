/**
 * Note: Handler of Audio Decoder
 * Date: 2026/6/3
 * Author: frank
 */

#include "AudioDecodeHandler.h"

#include <unistd.h>

#include "decode/FFmpegAudioDecoder.h"
#include "NextLog.h"
#include "NextMessage.h"
#include "CommonUtil.h"

#define TAG "AuDecodeHandler"

AudioDecodeHandler::AudioDecodeHandler(sp<MediaParseHandler> &mediaParseHandler,
                                       const sp<PlayerLink> &pLink,
                                       NotifyCallback notifyCb,
                                       const char *threadName)
        : BaseThread(threadName),
          mPlayerLink(pLink),
          mRedSourceController(mediaParseHandler),
          mNotifyCb(std::move(notifyCb)) {}

AudioDecodeHandler::~AudioDecodeHandler() {
    mFrameQueue.reset();
    mAudioDecoder.reset();
    mMetaData.reset();
}

void AudioDecodeHandler::setConfig(const sp<GeneralConfig> &config) {
    std::lock_guard<std::mutex> lock(mLock);
    mGeneralConfig = config;
}

int AudioDecodeHandler::prepare(const sp<MetaData> &metadata) {
    int ret = RESULT_OK;
    mMetaData = metadata;
    ret = init();
    if (ret != RESULT_OK) {
        notifyListener(MSG_ON_ERROR, ERROR_PARSE_STREAM_OPEN);
        return ret;
    }

    this->start();

    return ret;
}

int AudioDecodeHandler::init() {
    if (!mMetaData) {
        return ERROR_PLAYER_NOT_INIT;
    }
    std::string codecType, codecName;
    auto trackInfo = mMetaData->track_info[mMetaData->audio_index];

    mAudioDecoder = std::make_unique<FFmpegAudioDecoder>();
    if (!mAudioDecoder) {
        NEXT_LOGE(TAG, "Audio decoder create error");
        notifyListener(MSG_ON_ERROR, ERROR_DECODE_AUDIO_DEC, ERROR_DECODE_AUDIO_OPEN);
        return ERROR_DECODE_AUDIO_OPEN;
    }
    mAudioDecoder->setDecodeCallback(this);
    resetDecoderFormat();

    mFrameQueue = std::make_unique<FrameQueue>(SAMPLE_QUEUE_SIZE);
    if (!mFrameQueue) {
        NEXT_LOGE(TAG, "Audio frame queue create error\n");
        return ERROR_DECODE_AUDIO_OPEN;
    }

    codecType = AVCODEC_MODULE_NAME;
    codecName = avcodec_get_name((AVCodecID) trackInfo.codec_id);
    mPlayerLink->audio_codec_name = codecName;
    mPlayerLink->audio_codec_type = codecType;

    NEXT_LOGE(TAG, "Audio decoder created, name=%s, type=%s\n", codecName.c_str(), codecType.c_str());
    return RESULT_OK;
}

static void ReleaseFrame(FFmpegBufferContext *context) {
    if (context && context->av_frame) {
        auto *frame = reinterpret_cast<AVFrame *>(context->av_frame);
        av_frame_unref(frame);
        av_frame_free(&frame);
    }
}

int AudioDecodeHandler::onDecodedFrame(AVFrame *frame) {

    std::unique_ptr<FrameBuffer> buffer(new FrameBuffer());
    if (!buffer) {
        return ERROR_OTHER_OOM;
    }

    buffer->pts           = frame->best_effort_timestamp;
    buffer->format        = frame->format;
    buffer->serial        = mSerial;
    buffer->isAudio       = true;
    buffer->sampleSize    = frame->nb_samples;
    buffer->sampleRate    = frame->sample_rate ? frame->sample_rate : 44100;
    buffer->numChannels   = frame->ch_layout.nb_channels;
    buffer->channelLayout = &frame->ch_layout;

    for (int i = 0; i < frame->ch_layout.nb_channels; i++) {
        buffer->channel[i] =  frame->data[i];
    }

    auto bufferContext           = new FFmpegBufferContext();
    bufferContext->av_frame      = frame;
    bufferContext->opaque        = reinterpret_cast<void *>(ReleaseFrame);
    bufferContext->release_frame =
        [](FFmpegBufferContext *ctx) -> void {
            FFmpegBufferContext ffmpegCtx{};
            ffmpegCtx.av_frame = ctx->av_frame;
            auto *releaseFrame = (void (*)(FFmpegBufferContext *context)) ctx->opaque;
            releaseFrame(&ffmpegCtx);
        };
    buffer->opaque = bufferContext;


    if (!mFrameQueue) {
        return ERROR_OTHER_OOM;
    }
    if (bAbort) {
        return RESULT_OK;
    }
    if (checkAccurateSeek(buffer)) {
        return RESULT_OK;
    }
    mFrameQueue->putFrame(buffer);
    return RESULT_OK;
}

void AudioDecodeHandler::onDecodeError(int error) {}

int AudioDecodeHandler::performDecode(AVPacket *pkt) {
    int ret = RESULT_OK;
    if (!mAudioDecoder)
        return ERROR_DECODE_AUDIO_DEC;

    AVRational timebase = {1, 1000};
    auto trackInfo = mMetaData->track_info[mMetaData->audio_index];
    AVRational srcTimebase =
            (AVRational) {trackInfo.time_base_num, trackInfo.time_base_den};
    // TODO: check rescale pts/dts
    pkt->pts = av_rescale_q(pkt->pts, srcTimebase, timebase);
    ret = mAudioDecoder->decode(pkt);

    if (ret != RESULT_OK) {
        return ERROR_DECODE_AUDIO_DEC;
    }
    return ret;
}

int AudioDecodeHandler::readPacketOrBuffering(std::unique_ptr<NextPacket> &pkt) {
    if (!mRedSourceController)
        return ERROR_PARSE_NOT_INIT;
    int ret = mRedSourceController->getPacket(pkt, AVMEDIA_TYPE_AUDIO, false);
    if (ret == ERROR_PLAYER_TRY_AGAIN && !bEOF) {
        if (mPlayerLink->first_audio_rendered) {
            mRedSourceController->toggleBuffering(true);
        }
        ret = mRedSourceController->getPacket(pkt, AVMEDIA_TYPE_AUDIO, true);
    }
    return ret;
}

int AudioDecodeHandler::performFlush() {
    std::lock_guard<std::mutex> lock(mLock);
    mSerial++;
    bEOF = false;
    if (mAudioDecoder) {
        mAudioDecoder->flush();
    }
    if (mFrameQueue) {
        mFrameQueue->flush();
    }
    return RESULT_OK;
}

int AudioDecodeHandler::resetDecoderFormat() {
    if (!mMetaData) {
        return ERROR_PLAYER_TRY_AGAIN;
    }
    if (!mAudioDecoder)
        return ERROR_DECODE_AUDIO_OPEN;
    auto trackInfo = mMetaData->track_info[mMetaData->audio_index];
    AudioCodecConfig config;

    config.profile        = trackInfo.codec_profile;
    config.codec_id       = trackInfo.codec_id;
    config.channels       = trackInfo.channels;
    config.sample_rate    = trackInfo.sample_rate;
    config.extradata      = trackInfo.extra_data;
    config.extradata_size = trackInfo.extra_data_size;
    mAudioDecoder->init(config);

    return RESULT_OK;
}

void AudioDecodeHandler::notifyListener(int what, int arg1, int arg2) {
    if (mNotifyCb) {
        mNotifyCb(what, arg1, arg2, nullptr, 0);
    }
}

bool AudioDecodeHandler::checkAccurateSeek(const std::unique_ptr<FrameBuffer> &buffer) {
    int64_t now           = 0;
    int64_t diff          = 0;
    double audioClock     = 0;
    int64_t audioSeekPos  = 0;
    bool accurateSeekFail = false;
    double framePts       = static_cast<double>(buffer->pts) / 1000;
    double frameDuration  = static_cast<double>(buffer->sampleSize) / buffer->sampleRate;
    PlayerConfig *playerConfig = mGeneralConfig->playerConfig->get();

    if (playerConfig->enable_accurate_seek &&
        mPlayerLink->aud_accurate_seek_req && !mPlayerLink->seek_req) {
        now = CurrentTimeMs();
        if (!isnan(framePts)) {
            audioClock = framePts + frameDuration;
            mPlayerLink->accurate_seek_audio_pts = static_cast<int64_t>(audioClock * SECOND_TO_MS);
            audioSeekPos = mPlayerLink->seek_pos;
            diff = llabs((int64_t) (audioClock * SECOND_TO_MS) - mPlayerLink->seek_pos);
            if ((audioClock * SECOND_TO_MS < static_cast<double>(mPlayerLink->seek_pos)) ||
                diff < MAX_DEVIATION) {
                if (mPlayerLink->drop_aframe_count == 0) {
                    std::unique_lock<std::mutex> lock(mPlayerLink->accurate_seek_mutex);
                    if (mPlayerLink->accurate_seek_start <= 0 &&
                        (mMetaData->video_index < 0 ||
                         mPlayerLink->vid_accurate_seek_req)) {
                        mPlayerLink->accurate_seek_start = now;
                    }
                }
                mPlayerLink->drop_aframe_count++;
                if (!mPlayerLink->vid_accurate_seek_req &&
                    mMetaData->video_index >= 0 &&
                    audioClock * SECOND_TO_MS > static_cast<double>(mPlayerLink->accurate_seek_video_pts)) {
                    accurateSeekFail = true;
                } else {
                    now = CurrentTimeMs();
                    if ((now - mPlayerLink->accurate_seek_start) <=
                        playerConfig->accurate_seek_timeout) {
                        return true; // drop some old frame when do accurate seek
                    } else {
                        accurateSeekFail = true;
                    }
                }
            } else {
                while (mPlayerLink->vid_accurate_seek_req && !bAbort) {
                    int64_t pts = mPlayerLink->accurate_seek_video_pts;
                    if (pts - mPlayerLink->seek_pos >= 0) {
                        break;
                    } else {
                        usleep(SLEEP_20MS_CONVERT_US);
                    }
                    now = CurrentTimeMs();
                    if ((now - mPlayerLink->accurate_seek_start) >
                        playerConfig->accurate_seek_timeout) {
                        break;
                    }
                    if (audioSeekPos != mPlayerLink->seek_pos) {
                        break;
                    }
                }
                if (audioSeekPos == mPlayerLink->seek_pos) {
                    mPlayerLink->drop_aframe_count = 0;
                    std::unique_lock<std::mutex> lock(mPlayerLink->accurate_seek_mutex);
                    mPlayerLink->aud_accurate_seek_req = false;
                    mPlayerLink->video_accurate_seek_cond.notify_one();
                    if (audioSeekPos == mPlayerLink->seek_pos &&
                        mPlayerLink->vid_accurate_seek_req && !bAbort) {
                        mPlayerLink->audio_accurate_seek_cond.wait_for(
                                lock, std::chrono::milliseconds(playerConfig->accurate_seek_timeout));
                    } else {
                        notifyListener(MSG_ACCURATE_SEEK_COMPLETE, static_cast<int32_t>(audioClock * 1000));
                    }

                    if (audioSeekPos != mPlayerLink->seek_pos && !bAbort) {
                        mPlayerLink->aud_accurate_seek_req = true;
                        return true;
                    }
                }
            }
        } else {
            accurateSeekFail = true;
        }
        if (accurateSeekFail) {
            mPlayerLink->drop_aframe_count = 0;
            std::unique_lock<std::mutex> lock(mPlayerLink->accurate_seek_mutex);
            mPlayerLink->aud_accurate_seek_req = false;
            mPlayerLink->video_accurate_seek_cond.notify_one();
            if (mPlayerLink->vid_accurate_seek_req && !bAbort) {
                mPlayerLink->audio_accurate_seek_cond.wait_for(
                        lock, std::chrono::milliseconds(playerConfig->accurate_seek_timeout));
            } else {
                notifyListener(MSG_ACCURATE_SEEK_COMPLETE,
                               static_cast<int32_t>(audioClock * 1000));
            }
        }
        mPlayerLink->accurate_seek_start = 0;
        accurateSeekFail = false;
    }
    return false;
}

int AudioDecodeHandler::getFrame(std::unique_ptr<FrameBuffer> &buffer) {
    if (!mFrameQueue) {
        return ERROR_PLAYER_INIT_FAIL;
    }
    if (mFrameQueue->size() <= 0) {
        std::unique_lock<std::mutex> lock(mLock);
        if (bEOF) {
            mPlayerLink->audio_dec_finish = true;
            return ERROR_PLAYER_EOF;
        }
    }
    return mFrameQueue->getFrame(buffer, AUDIO_FRAME_WAIT_TIMEOUT_MS);
}

void AudioDecodeHandler::executeTask() {
    while (!bAbort) {
        std::unique_ptr<NextPacket> pkt;
        int ret = readPacketOrBuffering(pkt);
        if (ret != RESULT_OK || !pkt) {
            if (bAbort) {
                continue;
            }
            usleep(SLEEP_20MS_CONVERT_US);
            continue;
        }
        if (pkt->isFlushPacket()) {
            performFlush();
            continue;
        } else if (pkt->isEofPacket()) {
            NEXT_LOGI(TAG, "packet EOF!\n");
            if (mGeneralConfig->playerConfig->get()->enable_accurate_seek) {
                std::unique_lock<std::mutex> lock(mPlayerLink->accurate_seek_mutex);
                mPlayerLink->aud_accurate_seek_req = false;
                mPlayerLink->audio_accurate_seek_cond.notify_all();
            }
            std::unique_lock<std::mutex> lock(mLock);
            bEOF = true;
            mCond.wait(lock, [this] { return bAbort || !bEOF; });
            continue;
        } else if (pkt->getSerial() != mSerial) {
            continue;
        }
        mPlayerLink->audio_dec_finish = false;
        // execute audio decoding
        performDecode(pkt->getPacket());
    }
}

int AudioDecodeHandler::getSerial() {
    std::unique_lock<std::mutex> lock(mLock);
    return mSerial;
}

void AudioDecodeHandler::resetEof() {
    std::unique_lock<std::mutex> lock(mLock);
    bEOF = false;
    mPlayerLink->audio_dec_finish = false;
    if (mFrameQueue) {
        mFrameQueue->flush();
    }
    mCond.notify_one();
}

int AudioDecodeHandler::stop() {
    std::unique_lock<std::mutex> lock(mLock);

    if (mGeneralConfig->playerConfig->get()->enable_accurate_seek) {
        std::unique_lock<std::mutex> seekLock(mPlayerLink->accurate_seek_mutex);
        mPlayerLink->aud_accurate_seek_req = false;
        mPlayerLink->audio_accurate_seek_cond.notify_all();
    }
    bAbort = true;
    mCond.notify_all();
    if (mFrameQueue) {
        mFrameQueue->abort();
    }
    return RESULT_OK;
}

void AudioDecodeHandler::release() {
    NEXT_LOGD(TAG, "release begin\n");
    bAbort = true;
    if (bReleased) {
        NEXT_LOGD(TAG, "Already released!\n");
        return;
    }
    bReleased = true;
    if (mThread.joinable()) {
        mThread.join();
    }
    if (mFrameQueue) {
        mFrameQueue->flush();
    }
    NEXT_LOGD(TAG, "release end\n");
}
