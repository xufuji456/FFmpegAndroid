/**
 * Note: Handler of Video Decoder
 * Date: 2026/6/5
 * Author: frank
 */

#include "VideoDecodeHandler.h"

#include <unistd.h>

#include "common/NextConfig.h"
#include "CommonUtil.h"
#include "decode/VideoDecoderFactory.h"
#include "NextLog.h"
#include "NextMessage.h"

#define TAG "ViDecodeHandler"

VideoDecodeHandler::VideoDecodeHandler(sp<MediaParseHandler> &mediaParser,
                                       const sp<PlayerLink> &pLink,
                                       NotifyCallback notifyCb, const char *threadName)
                                       : BaseThread(threadName),
                                         mPlayerLink(pLink),
                                         mMediaParser(mediaParser),
                                         mNotifyCb(std::move(notifyCb)) {
    mLastSerial    = -1;
    bIdrIdentified = true;
}

VideoDecodeHandler::~VideoDecodeHandler() {
    mBuffer.reset();
    mMetaData.reset();
    mPendingPkt.reset();
    mFrameQueue.reset();
    mVideoDecoder.reset();
#if defined(__ANDROID__)
    ANativeWindow_release(mCurNativeWindow);
#endif
}

int VideoDecodeHandler::init(sp<MetaData> &metadata) {
    int ret = RESULT_OK;
    size_t queueSize = VIDEO_PICTURE_QUEUE_SIZE_DEFAULT;
    mMetaData = metadata;
    PlayerConfig *playerConfig = mGeneralConfig->playerConfig->get();
    if (mMetaData && mMetaData->video_index >= 0) {
        mWidth  = mMetaData->track_info[mMetaData->video_index].width;
        mHeight = mMetaData->track_info[mMetaData->video_index].height;
    }
    mFrameQueue = std::make_shared<FrameQueue>(queueSize);
    if (!mFrameQueue) {
        NEXT_LOGE(TAG, "Video frame queue create error\n");
        return ERROR_PLAYER_INIT_FAIL;
    }
    ret = initInternal();
    if (ret != RESULT_OK) {
        playerConfig->enable_vtb     = false;
        playerConfig->enable_ndkvdec = false;
        ret = initInternal();
        if (ret != RESULT_OK) {
            notifyListener(MSG_ON_ERROR, ERROR_PARSE_STREAM_OPEN);
            return ret;
        }
    }

    this->start();

    return ret;
}

int VideoDecodeHandler::initInternal() {
    if (!mMetaData) {
        NEXT_LOGE(TAG, "initInternal, null metadata\n");
        return ERROR_PLAYER_INIT_FAIL;
    }

    std::string codecType;
    int codecId                = mMetaData->track_info[mMetaData->video_index].codec_id;
    VideoCodecType type        = VideoCodecType::DECODE_TYPE_UNKNOWN;
    std::string codecName      = avcodec_get_name((AVCodecID) codecId);
    PlayerConfig *playerConfig = mGeneralConfig->playerConfig->get();

    if (playerConfig) {
        if (playerConfig->enable_vtb) {
            mPlayerLink->hw_decode = true;
            type = VideoCodecType::DECODE_TYPE_IOS;
            mPlayerLink->stat.video_dec_type = OPTION_STR_DECODER_VTB;
            codecType = VIDEOTOOLBOX_MODULE_NAME;
        }
#if defined(__ANDROID__)
        else if (playerConfig->enable_ndkvdec) {
            mPlayerLink->hw_decode = true;
            type = VideoCodecType::DECODE_TYPE_ANDROID;
            mPlayerLink->stat.video_dec_type = OPTION_STR_DECODER_MEDIACODEC;
            codecType = MEDIACODEC_MODULE_NAME;
            if (!mCurNativeWindow) {
                NEXT_LOGE(TAG, "init decoder with null surface\n");
            }
        }
#elif defined(__HARMONY__)
            else if (playerConfig->enable_harmony_vdec) {
              type = reddecoder::VideoCodecType::DECODE_TYPE_HARMONY;
              mPlayerLink->stat.video_dec_type = OPTION_STR_DECODER_HARMONY;
              codecType = HARMONY_VIDEO_DECODER_MODULE_NAME;
            }
#endif
        else {
            type = VideoCodecType::DECODE_TYPE_SOFTWARE;
            mPlayerLink->stat.video_dec_type = OPTION_STR_DECODER_AVCODEC;
            codecType = AVCODEC_MODULE_NAME;
        }
    }

    mVideoDecoder = VideoDecoderFactory::createVideoDecoder(type, codecId);
    if (!mVideoDecoder) {
        NEXT_LOGE(TAG, "Video decoder create error\n");
        notifyListener(MSG_ON_ERROR, ERROR_DECODE_VIDEO_DEC, ERROR_DECODE_VIDEO_OPEN);
        return ERROR_DECODE_VIDEO_OPEN;
    }

    int err = mVideoDecoder->init(nullptr);
    if (err != RESULT_OK) {
        NEXT_LOGE(TAG, "Video decoder init error %d\n", err);
        return ERROR_DECODE_VIDEO_OPEN;
    }

    mVideoDecoder->setDecodeCallback(this);

    if (mPlayerLink->stat.video_dec_type != OPTION_STR_DECODER_MEDIACODEC) {
        resetDecoderFormat();
    }

    if (mMetaData->track_info[mMetaData->video_index].pixel_fmt != AV_PIX_FMT_NONE) {
        mPlayerLink->stat.pixel_format = mMetaData->track_info[mMetaData->video_index].pixel_fmt;
    }

    mPlayerLink->video_codec_name = codecName;
    mPlayerLink->video_codec_type = codecType;
    NEXT_LOGI(TAG, "Video decoder created, name=%s, type=%s\n", codecName.c_str(), codecType.c_str());

    notifyListener(MSG_VIDEO_DECODER_OPEN, static_cast<int>(type), -1);

    return RESULT_OK;
}

int VideoDecodeHandler::performDecode(AVPacket *pkt) {
    int ret = RESULT_OK;
    if (!mVideoDecoder)
        return ERROR_DECODE_VIDEO_OPEN;

    // TODO: ios refresh decoder session
    if (bRefreshSession) {
        pkt->flags |= DecodeFlag::DECODE_FLAG_NO_OUT_FRAME;
    }
    ret = mVideoDecoder->decode(pkt);

    if (ret == ERROR_DECODE_SESSION) {
        bRefreshSession = true;
    } else if (ret != RESULT_OK) {
        mDecodeErrorCount++;
        if (mPlayerLink->stat.video_dec_type != OPTION_STR_DECODER_AVCODEC) {
            notifyListener(MSG_ON_ERROR, ERROR_DECODE_VIDEO_DEC, ret);
        }
    }
    return ret;
}

int VideoDecodeHandler::onDecodedFrame(std::unique_ptr<MixedBuffer> decodedFrame) {

    PlayerConfig *playerConfig = mGeneralConfig->playerConfig->get();
    mPlayerLink->stat.decode_rate = mSpeedMeter.add();
    mDecodeErrorCount = 0;

    if (bAbort) {
        return RESULT_OK;
    }
    if (!mFrameQueue) {
        return ERROR_DECODE_NOT_INIT;
    }
    if (bDecoderRecovery) {
        return RESULT_OK;
    }
    if (frontIsFlush()) {
        return RESULT_OK;
    }

    auto meta = decodedFrame->getVideoFrameMetadata();
    std::unique_ptr<FrameBuffer> buffer(new FrameBuffer());
    if (!buffer) {
        return ERROR_OTHER_OOM;
    }

    auto trackInfo = mMetaData->track_info[mMetaData->video_index];
    AVRational tb = (AVRational) {trackInfo.time_base_num, trackInfo.time_base_den};
    buffer->width   = meta->width;
    buffer->height  = meta->height;
    buffer->data    = decodedFrame->obtainData();
    buffer->pts     = meta->pts * av_q2d(tb) * 1000; // ms
    buffer->serial  = mSerial;
    buffer->yBuffer = meta->buffer_y;
    buffer->uBuffer = meta->buffer_u;
    buffer->vBuffer = meta->buffer_v;
    buffer->yStride = meta->stride_y;
    buffer->uStride = meta->stride_u;
    buffer->vStride = meta->stride_v;
    buffer->pixel_format = PIXEL_FORMAT_UNKNOWN;

    switch (meta->pixel_format) {
        case VideoPixelFormat::PIXEL_FORMAT_VIDEOTOOLBOX:
            buffer->pixel_format = PIXEL_FORMAT_VIDEOTOOLBOX;
            if (meta->buffer_context) {
                auto bufferContext = new VideoToolBufferContext();
                bufferContext->buffer =
                        ((VideoToolBufferContext *) meta->buffer_context)->buffer;
                buffer->opaque = bufferContext;
                delete (VideoToolBufferContext *) meta->buffer_context;
            }
            break;
        case VideoPixelFormat::PIXEL_FORMAT_MEDIACODEC:
            buffer->pixel_format = PIXEL_FORMAT_MEDIACODEC;
#if defined(__ANDROID__)
            if (meta->buffer_context) {
                auto bufferContext = new MediaCodecBufferContext();
                auto opaqueContext = ((MediaCodecBufferContext *) meta->buffer_context);
                bufferContext->buffer_index   = opaqueContext->buffer_index;
                bufferContext->decoder        = opaqueContext->decoder;
                bufferContext->decoder_serial = opaqueContext->decoder_serial;
                bufferContext->media_codec    = opaqueContext->media_codec;
                bufferContext->opaque         =
                        reinterpret_cast<void *>(opaqueContext->release_output_buffer);

                bufferContext->release_output_buffer =
                        [](MediaCodecBufferContext *ctx, bool render) -> void {
                            MediaCodecBufferContext mediacodecCtx{};
                            mediacodecCtx.buffer_index   = ctx->buffer_index;
                            mediacodecCtx.decoder        = ctx->decoder;
                            mediacodecCtx.media_codec    = ctx->media_codec;
                            mediacodecCtx.decoder_serial = ctx->decoder_serial;
                            auto *releaseBuffer =
                            (void (*)(MediaCodecBufferContext *context, bool render)) ctx->opaque;
                            releaseBuffer(&mediacodecCtx, render);
                        };
                buffer->opaque = bufferContext;
                delete (MediaCodecBufferContext *) meta->buffer_context;
            }
#endif
            break;
        case VideoPixelFormat::PIXEL_FORMAT_HARMONY:
            break;
        case VideoPixelFormat::PIXEL_FORMAT_YUV420P:
            buffer->pixel_format = PIXEL_FORMAT_YUV420P;
        case VideoPixelFormat::PIXEL_FORMAT_YUVJ420P:
            if (buffer->pixel_format == PIXEL_FORMAT_UNKNOWN) {
                buffer->pixel_format = PIXEL_FORMAT_YUVJ420P;
            }
        case VideoPixelFormat::PIXEL_FORMAT_YUV420P10LE:
            if (buffer->pixel_format == PIXEL_FORMAT_UNKNOWN) {
                buffer->pixel_format = PIXEL_FORMAT_YUV420P10LE;
            }
            if (meta->buffer_context) {
                auto bufferContext = new FFmpegBufferContext();
                auto opaqueContext = ((FFmpegBufferContext *) meta->buffer_context);
                bufferContext->av_frame = opaqueContext->av_frame;
                bufferContext->opaque   =
                        reinterpret_cast<void *>(opaqueContext->release_frame);

                bufferContext->release_frame =
                        [](FFmpegBufferContext *ctx) -> void {
                            FFmpegBufferContext ffmpegContext{};
                            ffmpegContext.av_frame = ctx->av_frame;
                            auto *releaseBuffer =
                            (void (*)(FFmpegBufferContext *context)) ctx->opaque;
                            releaseBuffer(&ffmpegContext);
                        };
                buffer->opaque = bufferContext;
                delete (FFmpegBufferContext *) meta->buffer_context;
            }
            break;
        default:
            NEXT_LOGE(TAG, "unsupported frame format=%d\n", meta->pixel_format);
            break;
    }

    if ((playerConfig->framedrop > 0) ||
        (playerConfig->framedrop &&
         getMasterSyncType(mPlayerLink) != CLOCK_VIDEO)) {
        mPlayerLink->stat.decoded_frame_count++;
        double pts  = static_cast<double>(buffer->pts) / 1000;
        double diff = pts - getMasterClock(mPlayerLink);
        if (!isnan(diff) && std::abs(diff) < AV_NO_SYNC_THRESHOLD && diff < 0 &&
            mPlayerLink->stat.video_cache.packets > 0 &&
            mSerial == getMasterClockSerial(mPlayerLink) &&
            mPlayerLink->first_video_rendered) {
            mPlayerLink->stat.continue_drop_frame++;
            if (mPlayerLink->stat.continue_drop_frame >
                playerConfig->framedrop) {
                mPlayerLink->stat.continue_drop_frame = 0;
            } else {
                mPlayerLink->stat.drop_frame_count++;
                mPlayerLink->stat.drop_frame_rate =
                        static_cast<float>(mPlayerLink->stat.drop_frame_count) /
                        static_cast<float>(mPlayerLink->stat.decoded_frame_count);
                NEXT_LOGW(TAG, "drop frame early pts=%lf, diff=%lf\n", pts, diff);
                return RESULT_OK;
            }
        }
    }

    if (mWidth != buffer->width || mHeight != buffer->height) {
        mWidth  = buffer->width;
        mHeight = buffer->height;
        notifyListener(MSG_VIDEO_SIZE_CHANGED, meta->width, meta->height);
    }

    if (!bFirstFrameDecoded) {
        bFirstFrameDecoded = true;
        notifyListener(MSG_VIDEO_DECODE_START);
    }
    mFrameQueue->putFrame(buffer);
    return RESULT_OK;
}

void VideoDecodeHandler::onDecodeError(int error, int errorCode) {
    if (bAbort) {
        return;
    }
    switch (error) {
        case RESULT_OK:
        case ERROR_PLAYER_EOF:
        case ERROR_PLAYER_TRY_AGAIN:
            break;
        case ERROR_DECODE_SESSION:
            bRefreshSession = true;
            break;
        default:
            mDecodeErrorCount++;
            if (mPlayerLink->stat.video_dec_type != OPTION_STR_DECODER_AVCODEC) {
                notifyListener(MSG_ON_ERROR, ERROR_DECODE_VIDEO_DEC,
                               -(static_cast<int32_t>(error)));
            }
            break;
    }
}

void VideoDecodeHandler::setConfig(const sp<GeneralConfig> &config) {
    std::lock_guard<std::mutex> lock(mLock);
    mGeneralConfig = config;
}

#if defined(__ANDROID__)
int VideoDecodeHandler::setNativeSurface(ANativeWindow *surface) {
    if (!surface) {
        return ERROR_RENDER_VIDEO_INIT;
    }
    if (surface && mCurNativeWindow && surface == mCurNativeWindow) {
        NEXT_LOGD(TAG, "new surface is same %p\n", surface);
        return RESULT_OK;
    }
    mCurNativeWindow = surface;
    mSurfaceUpdated.store(true);

    return RESULT_OK;
}
#endif

void VideoDecodeHandler::notifyListener(int what, int arg1, int arg2) {
    mNotifyCb(what, arg1, arg2, nullptr, 0);
}

int VideoDecodeHandler::readPacketOrBuffering(std::unique_ptr<NextPacket> &pkt) {
    if (!mMediaParser)
        return ERROR_PARSE_NOT_INIT;
    int ret = mMediaParser->getPacket(pkt, AVMEDIA_TYPE_VIDEO, false);
    if (ret == ERROR_PLAYER_TRY_AGAIN && !bEOF) {
        if (mPlayerLink->first_video_rendered) {
            mMediaParser->toggleBuffering(true);
        }
        ret = mMediaParser->getPacket(pkt, AVMEDIA_TYPE_VIDEO, true);
    }
    return ret;
}

int VideoDecodeHandler::getFrame(std::unique_ptr<FrameBuffer> &buffer) {
    if (!mFrameQueue) {
        return ERROR_PLAYER_INIT_FAIL;
    }
    if (mFrameQueue->size() <= 0) {
        std::unique_lock<std::mutex> lock(mLock);
        if (mLastSerial == mSerial) {
            mPlayerLink->video_dec_finish = true;
            return ERROR_PLAYER_EOF;
        }
    }
    return mFrameQueue->getFrame(buffer, QUEUE_WAIT_TIMEOUT);
}

int VideoDecodeHandler::performFlush() {
    std::lock_guard<std::mutex> lock(mLock);
    mSerial++;
    bEOF = false;
    int ret = RESULT_OK;
    mLastSerial = -1;
    if (mFrameQueue) {
        mFrameQueue->flush();
    }
    if (mVideoDecoder &&
        (mInputPacketCount > 0 ||
         mPlayerLink->stat.video_dec_type != OPTION_STR_DECODER_MEDIACODEC)) {
        mVideoDecoder->flush();
    }
    mInputPacketCount = 0;
    if (mPlayerLink->pause_req) {
        mPlayerLink->step_to_next_frame = true;
    }
    return ret;
}

bool VideoDecodeHandler::frontIsFlush() {
    if (!mMediaParser)
        return false;
    return mMediaParser->frontIsFlush(AVMEDIA_TYPE_VIDEO);
}

void VideoDecodeHandler::resetEof() {
    std::unique_lock<std::mutex> lock(mLock);
    bEOF = false;
    mLastSerial = -1;
    mPlayerLink->video_dec_finish = false;
    if (mFrameQueue) {
        mFrameQueue->flush();
    }
    mCond.notify_one();
}

int VideoDecodeHandler::resetDecoderFormat() {
    int ret = RESULT_OK;
    if (!mVideoDecoder) {
        return ERROR_DECODE_VIDEO_OPEN;
    }

    if (!mMetaData) {
        return ERROR_PLAYER_TRY_AGAIN;
    }

    mVideoDecoder->setVideoFormat(mMetaData.get());
    return ret;
}

int VideoDecodeHandler::resetDecoder() {
    if (mPlayerLink->stat.video_dec_type == OPTION_STR_DECODER_MEDIACODEC && mFrameQueue) {
        mFrameQueue->flush();
    }
    return resetDecoderFormat();
}

void VideoDecodeHandler::decodeLastCacheGop() {
    bDecoderRecovery = true;
    if ((!mPktQueue.empty()) &&
            mPktQueue.front()->isKeyOrIdrPacket(bIdrIdentified,
                                                "hevc" == mPlayerLink->video_codec_name)) {
        NEXT_LOGD(TAG, "decodeLastCacheGop begin\n");
        while (!mPktQueue.empty()) {
            if (bAbort || frontIsFlush()) {
                break;
            }
            auto cache_pkt = mPktQueue.front();
            mPktQueue.pop();
            performDecode(cache_pkt->getPacket());
        }
        NEXT_LOGD(TAG, "decodeLastCacheGop end\n");
    }
    bDecoderRecovery = false;
}

int VideoDecodeHandler::getSerial() {
    std::unique_lock<std::mutex> lock(mLock);
    return mSerial;
}

int VideoDecodeHandler::getQueueSize() {
    return mFrameQueue ? mFrameQueue->size() : 0;
}

// decode thread
void VideoDecodeHandler::executeTask() {
    PlayerConfig *playerConfig = mGeneralConfig->playerConfig->get();
    if (mPlayerLink->stat.video_dec_type == OPTION_STR_DECODER_MEDIACODEC &&
        playerConfig->mediacodec_auto_rotate) {
        notifyListener(MSG_ROTATION_CHANGED, 0);
    } else {
        notifyListener(MSG_ROTATION_CHANGED,
                       mMetaData->track_info[mMetaData->video_index].rotation);
    }
    while (!bAbort) {
#if defined(__ANDROID__)
        // waiting for surface
        if (mPlayerLink->stat.video_dec_type == OPTION_STR_DECODER_MEDIACODEC) {
            if (mSurfaceUpdated.load()) {
                mPendingPkt.reset();
                mInputPacketCount = 0;
                if (mFrameQueue) {
                    mFrameQueue->flush();
                }
                if (mVideoDecoder) {
                    HardWareContext *hwContext = new AndroidHardWareContext(mCurNativeWindow);
                    mVideoDecoder->updateHardwareContext(hwContext);
                }
                if (mCurNativeWindow) {
                    resetDecoder();
                }
                mSurfaceUpdated.store(false);
            }
            if (!mCurNativeWindow && !bAbort) {
                std::this_thread::sleep_for(std::chrono::milliseconds(SLEEP_10MS));
                NEXT_LOGD(TAG, "waiting for surface, sleep 10ms\n");
                continue;
            }
        }
#endif

        std::unique_ptr<NextPacket> pkt;
        int32_t ret = RESULT_OK;
        if (mPendingPkt && !frontIsFlush()) {
            pkt = std::move(mPendingPkt);
        } else {
            ret = readPacketOrBuffering(pkt);
        }
        if (ret != RESULT_OK || !pkt) {
            if (bAbort) {
                continue;
            }
            usleep(SLEEP_20MS_CONVERT_US);
            continue;
        }
        if (pkt->isFlushPacket()) {
            performFlush();
            mPendingPkt.reset();
            if (mPlayerLink->stat.video_dec_type == OPTION_STR_DECODER_VTB &&
                mDecodeErrorCount >= playerConfig->vtb_max_error_count) {
                resetDecoder();
            }
            continue;
        } else if (pkt->isEofPacket()) {
            // TODO: EOF process
            NEXT_LOGI(TAG, "packet EOF!\n");
            bEOF = true;
//            while (!bAbort && !frontIsFlush() && mVideoDecoder &&
//                   (mInputPacketCount > 0 &&
//                    mVideoDecoder->GetDelayedFrame() == RESULT_OK)) {
//            }
            if (frontIsFlush()) {
                continue;
            }
            if (playerConfig->enable_accurate_seek) {
                std::unique_lock<std::mutex> lck(mPlayerLink->accurate_seek_mutex);
                mPlayerLink->vid_accurate_seek_req = false;
                mPlayerLink->video_accurate_seek_cond.notify_all();
            }
            std::unique_lock<std::mutex> lock(mLock);
            mLastSerial = mSerial;
            while (!bAbort && !frontIsFlush()) {
                mCond.wait_for(lock, std::chrono::milliseconds(SLEEP_10MS));
            }
            continue;
        } else if (pkt->getSerial() != mSerial) {
            NEXT_LOGI(TAG, "packet serial=%d, current serial=%d\n", pkt->getSerial(), mSerial.load());
            continue;
        }

        bEOF = false;
        mLastSerial = -1;
        mPlayerLink->video_dec_finish = false;

        if (mPlayerLink->stat.video_dec_type == OPTION_STR_DECODER_VTB && !mPendingPkt) {
            if (pkt->isKeyPacket() || mPktQueue.size() >= MAX_PKT_QUEUE_DEEP) {
                bIdrIdentified = false;
                while (!mPktQueue.empty()) {
                    mPktQueue.pop();
                }
            }
            std::shared_ptr<NextPacket> newPkt(new NextPacket(pkt->getPacket(), pkt->getSerial()));
            mPktQueue.push(newPkt);
        }

        if (bRefreshSession &&
            mPlayerLink->stat.video_dec_type == OPTION_STR_DECODER_VTB) {
            resetDecoder();
            decodeLastCacheGop();
            bRefreshSession = false;
        }
        ret = performDecode(pkt->getPacket());
        if (ret == ERROR_PLAYER_TRY_AGAIN) {
            mPendingPkt = std::move(pkt);
            continue;
        } else {
            mPendingPkt.reset();
            mBuffer.reset();
        }
        if (!bRefreshSession &&
            mPlayerLink->stat.video_dec_type == OPTION_STR_DECODER_VTB &&
            mDecodeErrorCount > playerConfig->vtb_max_error_count) {
            playerConfig->enable_vtb = 0;
            ret = initInternal();
            if (ret != RESULT_OK) {
                break;
            }
            decodeLastCacheGop();
        }
        mInputPacketCount++;
        if (!bFirstPacketReceived) {
            bFirstPacketReceived = true;
            notifyListener(MSG_VIDEO_FIRST_PACKET);
        }
    }
}

int VideoDecodeHandler::stop() {
    std::unique_lock<std::mutex> lock(mLock);

    bAbort = true;
    if (mGeneralConfig->playerConfig->get()->enable_accurate_seek) {
        std::unique_lock<std::mutex> seekLock(mPlayerLink->accurate_seek_mutex);
        mPlayerLink->vid_accurate_seek_req = false;
        mPlayerLink->audio_accurate_seek_cond.notify_all();
        mPlayerLink->video_accurate_seek_cond.notify_all();
    }
    mCond.notify_all();
    if (mFrameQueue) {
        mFrameQueue->abort();
    }
    return RESULT_OK;
}

void VideoDecodeHandler::release() {
    NEXT_LOGD(TAG, "release begin\n");
    if (bReleased.load()) {
        NEXT_LOGD(TAG, "Already released\n");
        return;
    }
    bReleased.store(true);

    if (mThread.joinable()) {
        mThread.join();
    }
    if (mFrameQueue) {
        mFrameQueue->flush();
    }
    while (!mPktQueue.empty()) {
        mPktQueue.pop();
    }
    NEXT_LOGD(TAG, "release end\n");
}
