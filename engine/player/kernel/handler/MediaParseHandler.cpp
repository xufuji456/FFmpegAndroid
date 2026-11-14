/**
 * Note: Handler of Media Parser
 * Date: 2026/6/9
 * Author: frank
 */

#include "MediaParseHandler.h"

#include <unistd.h>

#include "CommonUtil.h"
#include "NextErrorCode.h"
#include "NextLog.h"
#include "NextMessage.h"
#include "NalUnitParser.h"

#define TAG "MediaParseHandler"

MediaParseHandler::MediaParseHandler(const sp<PlayerLink> &pLink,
                                     NotifyCallback notifyCb, const char *threadName)
        : BaseThread(threadName),
          mPlayerLink(pLink),
          mNotifyCb(std::move(notifyCb)) {}

MediaParseHandler::~MediaParseHandler() {
    NEXT_LOGD(TAG, "MediaParseHandler destructor\n");
}

int MediaParseHandler::open(std::string &url) {
    std::unique_lock<std::mutex> lock(mLock);
    mUrl = std::move(url);
    return RESULT_OK;
}

void MediaParseHandler::setPrepareCb(PrepareCallBack callBack) {
    mPrepareCb = std::move(callBack);
}

void MediaParseHandler::setConfig(const sp<GeneralConfig> &config) {
    std::unique_lock<std::mutex> lock(mLock);
    mGeneralConfig = config;
}

int MediaParseHandler::setMetaData() {
    std::unique_lock<std::mutex> lock(mLock);
    for (auto & it : mMetaData->track_info) {
        if (it.stream_type == AVMEDIA_TYPE_AUDIO && mMetaData->audio_index == -1) {
            mMetaData->audio_index = it.stream_index;
            mPktQueueMap[AVMEDIA_TYPE_AUDIO] = std::make_shared<NextPacketQueue>(AVMEDIA_TYPE_AUDIO);
        } else if (it.stream_type == AVMEDIA_TYPE_VIDEO && mMetaData->video_index == -1) {
            mMetaData->video_index = it.stream_index;
            mPktQueueMap[AVMEDIA_TYPE_VIDEO] = std::make_shared<NextPacketQueue>(AVMEDIA_TYPE_VIDEO);
        }
    }
    lock.unlock();

    if (mPrepareCb) {
        mPrepareCb(mMetaData);
    }
    return RESULT_OK;
}

int MediaParseHandler::prepareAsync() {
    this->start();
    return RESULT_OK;
}

int MediaParseHandler::seek(int64_t msec) {
    std::unique_lock<std::mutex> lock(mLock);
    if (!mPlayerLink->seek_req) {
        mPlayerLink->seek_pos = msec * 1000;
        mPlayerLink->seek_req = true;
        mCond.notify_one();
    }
    return RESULT_OK;
}

bool MediaParseHandler::frontIsFlush(int streamType) {
    sp<NextPacketQueue> queue = getQueueByStreamType(streamType);
    if (!queue) {
        return false;
    }
    return queue->isFlushPacket();
}

int MediaParseHandler::performFlush() {
    ++mSerial;
    if (mPktQueueMap.empty())
        return RESULT_OK;
    for (auto & it : mPktQueueMap) {
        it.second->flush();
    }
    updateCacheStatistic();
    return RESULT_OK;
}

// Decide whether the buffer has reached to resume play
bool MediaParseHandler::isBufferFinish() {
    return (!mPlayerLink->seek_req &&
             (mPlayerLink->stat.audio_cache.bytes + mPlayerLink->stat.video_cache.bytes > mMaxBufferSize) &&
             (mPlayerLink->stat.audio_cache.bytes > AUDIO_CACHE_64K || mMetaData->audio_index < 0) &&
             (mPlayerLink->stat.video_cache.bytes > VIDEO_CACHE_256K || mMetaData->video_index < 0)) ||
            ((mPlayerLink->stat.audio_cache.packets >= DEFAULT_MIN_FRAMES || mMetaData->audio_index < 0) &&
             (mPlayerLink->stat.video_cache.packets >= DEFAULT_MIN_FRAMES || mMetaData->video_index < 0));
}

// Update buffer state
void MediaParseHandler::toggleBuffering(bool buffering) {
    std::unique_lock<std::mutex> lock(mLock);
    // 1=seek 2=network 3=decode
    int bufferType = -1;
    bool bSeekBuffering{false};

    if (buffering && !bBuffering && !bEOF && !mPlayerLink->pause_req) {
        NEXT_LOGI(TAG, "toggleBuffering begin...\n");
        bBuffering = true;
        mBufferingPercent = 0;
        if (mPlayerLink->seek_req ||
            mPlayerLink->last_audio_seek_serial >= 0 ||
            mPlayerLink->last_video_seek_serial >= 0) {
            bSeekBuffering = true;
            bufferType = 1;
        }
        notifyListener(MSG_BUFFER_START, bufferType);
    } else if (!buffering && bBuffering) {
        NEXT_LOGI(TAG, "toggleBuffering end...\n");
        bBuffering = false;
        if (bSeekBuffering || mPlayerLink->last_audio_seek_serial >= 0 ||
            mPlayerLink->last_video_seek_serial >= 0) {
            bSeekBuffering = false;
            bufferType = 1;
        }
        notifyListener(MSG_BUFFER_END, bufferType);
    }
}

int MediaParseHandler::getSerial() {
    std::unique_lock<std::mutex> lock(mLock);
    return mSerial;
}

int MediaParseHandler::getPacket(std::unique_ptr<NextPacket> &pkt, int streamType, bool block) {
    sp<NextPacketQueue> queue = getQueueByStreamType(streamType);
    if (!queue) {
        return ERROR_PLAYER_INIT_FAIL;
    }
    int ret = queue->getPacket(pkt, block);
    updateCacheStatistic();
    return ret;
}

void MediaParseHandler::notifyListener(int what, int arg1, int arg2) {
    mNotifyCb(what, arg1, arg2, nullptr, 0);
}

sp<NextPacketQueue> MediaParseHandler::getQueueByStreamType(int streamType) {
    auto it = mPktQueueMap.find(streamType);
    return it != mPktQueueMap.end() ? it->second : nullptr;
}

// 1=flush 2=eof
int MediaParseHandler::putPacketByType(PacketOpType type) {
    if (mPktQueueMap.empty()) {
        NEXT_LOGE(TAG, "PktQueue not init!\n");
        return ERROR_PLAYER_INIT_FAIL;
    }
    for (auto & it : mPktQueueMap) {
        std::unique_ptr<NextPacket> pkt(new NextPacket(type));
        it.second->putPacket(pkt);
    }
    return RESULT_OK;
}

// Parser thread looping
void MediaParseHandler::executeTask() {
    int ret                     = 0;
    int errorType               = ERROR_OTHER_UNKNOWN;
    bool completed              = false;
    int connectRetryCount       = MAX_RETRY_COUNT;
    int64_t bufferCheckTime     = 0;
    int64_t prevBufferCheckTime = 0;

    FFmpegOption opt{nullptr};
    AVPacket *pkt = av_packet_alloc();
    PlayerConfig *playerConfig = mGeneralConfig->playerConfig->get();

    NEXT_LOGI(TAG, "Parse thread Start\n");

    mMaxBufferSize = playerConfig->dcc.max_buffer_size > 0
                     ? playerConfig->dcc.max_buffer_size
                     : mMaxBufferSize;

    do {
        mExtractor = std::make_shared<NextExtractor>(mNotifyCb);
        mMetaData  = std::make_shared<MetaData>();
        if (!mExtractor || !mMetaData) {
            NEXT_LOGE(TAG, "Create extractor failed!\n");
            notifyListener(MSG_ON_ERROR, ERROR_OTHER_OOM);
            return;
        }
        av_dict_copy(&opt.format_opts, mGeneralConfig->formatConfig, 0);
        av_dict_copy(&opt.codec_opts, mGeneralConfig->codecConfig, 0);
        NEXT_LOGD(TAG, "Open url = %s\n", mUrl.c_str());

        ret = mExtractor->open(mUrl, opt, mMetaData);

        av_dict_free(&opt.format_opts);
        av_dict_free(&opt.codec_opts);
    } while (ret < 0 && !bAbort && connectRetryCount-- > 0);

    if (ret != RESULT_OK) {
        NEXT_LOGE(TAG, "Open extractor failed!\n");
        mExtractor->close();
        if ((errorType = getErrorType(ret)) == ERROR_OTHER_UNKNOWN) {
            notifyListener(MSG_ON_ERROR, ERROR_PARSE_OPEN, ret);
        } else {
            notifyListener(MSG_ON_ERROR, errorType, ret);
        }
        return;
    } else {
        setMetaData();
    }

    while (!bAbort) {
        if (mPlayerLink->seek_req) {
            bEOF = false;
            toggleBuffering(true);
            notifyListener(MSG_BUFFER_UPDATE, 0, 0);
            // TODO: seek flag
            ret = mExtractor->seek(mPlayerLink->seek_pos, 0, 0);
            if (ret < 0) {
                NEXT_LOGI(TAG, "Error while seeking, ret=%d", ret);
            } else {
                performFlush();
                putPacketByType(PKT_OP_TYPE_FLUSH);
                mPlayerLink->last_video_seek_serial = mSerial.load();
                mPlayerLink->last_audio_seek_serial = mSerial.load();
                mPlayerLink->last_seek_load_start   = CurrentTimeUs();
            }
            mPlayerLink->seek_req = false;
            playerConfig->dcc.current_high_water_mark_in_ms = playerConfig->dcc.first_high_water_mark_in_ms;
            if (playerConfig->enable_accurate_seek) {
                mPlayerLink->drop_aframe_count = 0;
                std::unique_lock<std::mutex> lock(mPlayerLink->accurate_seek_mutex);
                if (mMetaData->video_index >= 0) {
                    if (mPlayerLink->skip_frame < AVDISCARD_NONREF) {
                        mPlayerLink->skip_frame = std::max(
                                mPlayerLink->skip_frame, static_cast<int>(AVDISCARD_NONREF));
                    }
                    mPlayerLink->vid_accurate_seek_req = true;
                }
                if (mMetaData->audio_index >= 0) {
                    mPlayerLink->aud_accurate_seek_req = true;
                }
                mPlayerLink->audio_accurate_seek_cond.notify_one();
                mPlayerLink->video_accurate_seek_cond.notify_one();
            }
            notifyListener(MSG_SEEK_COMPLETE, static_cast<int>(mPlayerLink->seek_pos) / 1000);
            completed = false;
            toggleBuffering(true);
            continue;
        }

        updateCacheStatistic();

        if (isBufferFinish()) {
            toggleBuffering(false);
            usleep(SLEEP_10MS_CONVERT_US);
            continue;
        }
        if ((!mPlayerLink->paused || completed) &&
            (mMetaData->audio_index < 0 || mPlayerLink->audio_dec_finish) &&
            (mMetaData->video_index < 0 || mPlayerLink->video_dec_finish)) {
            if (completed) {
                std::unique_lock<std::mutex> lock(mLock);
                while (!bAbort && !mPlayerLink->seek_req) {
                    mCond.wait_for(lock, std::chrono::milliseconds(100));
                }
                if (!bAbort) {
                    continue;
                }
            } else {
                completed = true;
                toggleBuffering(false);
                NEXT_LOGE(TAG, "Completed, error %d\n", mPlayerLink->error_code);
                if (mPlayerLink->error_code != 0) {
                    notifyListener(REQUEST_KERNEL_PAUSE);
                    notifyListener(MSG_ON_ERROR, ERROR_PARSE_READ_FRAME, mPlayerLink->error_code);
                } else {
                    notifyListener(MSG_ON_COMPLETED);
                }
            }
        }

        ret = mExtractor->readPacket(pkt);

        if (ret != RESULT_OK || !pkt) {
            if (ret == AVERROR_EOF || ret == AVERROR_EXIT || mExtractor->getError()) {
                if (!bEOF) {
                    NEXT_LOGI(TAG, "Read EOF!\n");
                    bEOF = true;
                    putPacketByType(PKT_OP_TYPE_EOF);
                }

                toggleBuffering(false);

                if (mExtractor->getError()) {
                    mPlayerLink->error_code = mExtractor->getError();
                }
                if (ret == AVERROR_EXIT) {
                    mPlayerLink->error_code = AVERROR_EXIT;
                }

                std::unique_lock<std::mutex> lock(mLock);
                if (!bAbort && !mPlayerLink->seek_req) {
                    mCond.wait_for(lock, std::chrono::milliseconds(100));
                }
                continue;
            } else {
                NEXT_LOGE(TAG, "Read frame error: %d, %s\n", ret, av_err2str(ret));
                if ((errorType = getErrorType(ret)) == ERROR_OTHER_UNKNOWN) {
                    notifyListener(MSG_ON_ERROR, ERROR_PARSE_READ_FRAME, ret);
                } else {
                    notifyListener(MSG_ON_ERROR, errorType, ret);
                }
                continue;
            }
        } else {
            mPlayerLink->error_code = 0;
            bEOF = false;
        }
        if (pkt->stream_index == mMetaData->audio_index) {
            std::unique_ptr<NextPacket> flushPkt(new NextPacket(pkt, mSerial));
            sp<NextPacketQueue> queue = getQueueByStreamType(AVMEDIA_TYPE_AUDIO);
            if (queue) {
                queue->putPacket(flushPkt);
            }
        } else if (pkt->stream_index == mMetaData->video_index && !checkDropNonRefFrame(pkt)) {
            std::unique_ptr<NextPacket> flushPkt(new NextPacket(pkt, mSerial));
            sp<NextPacketQueue> queue = getQueueByStreamType(AVMEDIA_TYPE_VIDEO);
            if (queue) {
                queue->putPacket(flushPkt);
            }
        }

        updateCacheStatistic();
        bufferCheckTime = CurrentTimeMs();
        if ((!mPlayerLink->first_video_rendered && mMetaData->video_index >= 0) ||
            (!mPlayerLink->first_audio_rendered && mMetaData->audio_index >= 0)) {
            prevBufferCheckTime = bufferCheckTime;
            playerConfig->dcc.current_high_water_mark_in_ms = playerConfig->dcc.first_high_water_mark_in_ms;
            checkBuffering();
        } else {
            if (std::abs(bufferCheckTime - prevBufferCheckTime) > BUFFERING_CHECK_PERIOD) {
                prevBufferCheckTime = bufferCheckTime;
                checkBuffering();
            }
        }
        av_packet_unref(pkt);
    }
    if (pkt) {
        av_packet_free(&pkt);
    }
    if (mExtractor) {
        mExtractor->close();
    }
}

int MediaParseHandler::stop() {
    std::unique_lock<std::mutex> lock(mLock);
    int ret = RESULT_OK;
    bAbort = true;
    if (mExtractor) {
        mExtractor->setInterrupt();
    }
    // wakeup decode threads blocked on reading packets
    for (auto & it : mPktQueueMap) {
        it.second->abort();
    }
    updateCacheStatistic();
    mCond.notify_all();
    return ret;
}

void MediaParseHandler::release() {
    NEXT_LOGD(TAG, "%s release Start\n", __func__ );
    if (bReleased.load()) {
        NEXT_LOGD(TAG, "already released\n");
        return;
    }
    bReleased.store(true);
    bAbort = true;

    if (mThread.joinable()) {
        mThread.join();
    }
    for (auto & it : mPktQueueMap) {
        it.second->release();
    }
    NEXT_LOGD(TAG, "%s release end\n", __func__ );
}

// mapping from ffmpeg error code
int MediaParseHandler::getErrorType(int errorCode) {
    switch (errorCode) {
        case AVERROR_HTTP_BAD_REQUEST:
        case AVERROR_HTTP_UNAUTHORIZED:
            return ERROR_NET_HTTP401;
        case AVERROR_HTTP_FORBIDDEN:
            return ERROR_NET_HTTP403;
        case AVERROR_HTTP_NOT_FOUND:
            return ERROR_NET_HTTP404;
        case AVERROR_HTTP_OTHER_4XX:
        case AVERROR_HTTP_SERVER_ERROR:
            return ERROR_NET_SERVER_INNER;
        case AVERROR_INVALIDDATA:
            return ERROR_PARSE_INVALID_DATA;
        default:
            return ERROR_OTHER_UNKNOWN;
    }
}

void MediaParseHandler::checkBuffering() {
    PlayerConfig *playerConfig = mGeneralConfig->playerConfig->get();
    if (!playerConfig->packet_buffering) {
        return;
    }
    if (!bBuffering || bEOF) {
        return;
    }

    int bufSizePercent       = -1;
    int bufTimePercent       = -1;
    int highWaterMarkInMs    = playerConfig->dcc.current_high_water_mark_in_ms;
    int highWaterMarkInBytes = playerConfig->dcc.high_water_mark_in_bytes;
    bool needStartBuffering  = false;
    TrackInfo audioTrackInfo, videoTrackInfo;

    if (mMetaData->audio_index >= 0) {
        audioTrackInfo = mMetaData->track_info[mMetaData->audio_index];
    }
    if (mMetaData->video_index >= 0) {
        videoTrackInfo = mMetaData->track_info[mMetaData->video_index];
    }

    if (highWaterMarkInMs > 0) {
        int64_t cachedDurationInMs  = -1;
        int64_t audioCachedDuration = -1;
        int64_t videoCachedDuration = -1;

        if (audioTrackInfo.time_base_num > 0 && audioTrackInfo.time_base_den > 0) {
            audioCachedDuration = mPlayerLink->stat.audio_cache.duration;
        }

        if (videoTrackInfo.time_base_num > 0 && videoTrackInfo.time_base_den > 0) {
            videoCachedDuration = mPlayerLink->stat.video_cache.duration;
        }

        if (videoCachedDuration > 0 && audioCachedDuration > 0) {
            cachedDurationInMs = std::min(videoCachedDuration, audioCachedDuration);
        } else if (videoCachedDuration > 0) {
            cachedDurationInMs = static_cast<int>(videoCachedDuration);
        } else if (audioCachedDuration > 0) {
            cachedDurationInMs = static_cast<int>(audioCachedDuration);
        }

        if (cachedDurationInMs >= 0) {
            mPlayerLink->playable_duration = mPlayerLink->current_position + cachedDurationInMs;
            bufTimePercent = static_cast<int>(
                    av_rescale(cachedDurationInMs, 1005, highWaterMarkInMs * 10));
        }
    }

    int64_t cachedSize = mPlayerLink->stat.audio_cache.bytes + mPlayerLink->stat.video_cache.bytes;
    if (highWaterMarkInBytes > 0) {
        bufSizePercent = static_cast<int>(av_rescale(cachedSize, 1005, highWaterMarkInBytes * 10));
    }

    int bufPercent = -1;
    // buffer time first
    if (bufTimePercent >= 0) {
        if (bufTimePercent >= 100)
            needStartBuffering = true;
        bufPercent = bufTimePercent;
    } else {
        if (bufSizePercent >= 100)
            needStartBuffering = true;
        bufPercent = bufSizePercent;
    }

    if (bufTimePercent >= 0 && bufSizePercent >= 0) {
        bufPercent = std::min(bufTimePercent, bufSizePercent);
    }
    if (bufPercent) {
        if (bufPercent - mBufferingPercent >= MIN_BUFFER_NOTIFY) {
            notifyListener(MSG_BUFFER_UPDATE, bufPercent, 0);
            mBufferingPercent = bufPercent;
        }
    }

    if (needStartBuffering) {
        if (highWaterMarkInMs < playerConfig->dcc.next_high_water_mark_in_ms) {
            highWaterMarkInMs = playerConfig->dcc.next_high_water_mark_in_ms;
        } else {
            highWaterMarkInMs *= 2;
        }

        if (highWaterMarkInMs > playerConfig->dcc.last_high_water_mark_in_ms)
            highWaterMarkInMs = playerConfig->dcc.last_high_water_mark_in_ms;

        playerConfig->dcc.current_high_water_mark_in_ms = highWaterMarkInMs;

        if ((mPlayerLink->stat.audio_cache.packets >= MIN_MIN_FRAMES ||
             mMetaData->audio_index < 0) &&
            (mPlayerLink->stat.video_cache.packets >= MIN_MIN_FRAMES ||
             mMetaData->video_index < 0)) {
            toggleBuffering(false);
        }
    }
}

void MediaParseHandler::updateCacheStatistic() {
    sp<NextPacketQueue> videoQueue = getQueueByStreamType(AVMEDIA_TYPE_VIDEO);
    if (videoQueue) {
        if (mMetaData) {
            if (mMetaData->video_index >= 0) {
                auto trackInfo = mMetaData->track_info[mMetaData->video_index];
                AVRational tb =
                        (AVRational) {trackInfo.time_base_num, trackInfo.time_base_den};
                mPlayerLink->stat.video_cache.duration = videoQueue->duration() * av_q2d(tb) * 1000;
            }
        }
        mPlayerLink->stat.video_cache.bytes   = videoQueue->byteCount();
        mPlayerLink->stat.video_cache.packets = videoQueue->packetCount();
    }
    sp<NextPacketQueue> audioQueue = getQueueByStreamType(AVMEDIA_TYPE_AUDIO);
    if (audioQueue) {
        if (mMetaData && mMetaData->audio_index >= 0) {
            auto trackInfo = mMetaData->track_info[mMetaData->audio_index];
            AVRational tb = (AVRational) {trackInfo.time_base_num, trackInfo.time_base_den};
            mPlayerLink->stat.audio_cache.duration = audioQueue->duration() * av_q2d(tb) * 1000;
        }
        mPlayerLink->stat.audio_cache.bytes   = audioQueue->byteCount();
        mPlayerLink->stat.audio_cache.packets = audioQueue->packetCount();
    }
}

// Accurate seek：check if non reference frames need drop
bool MediaParseHandler::checkDropNonRefFrame(AVPacket *pkt) {
    PlayerConfig *playerConfig = mGeneralConfig->playerConfig->get();
    if (!mMetaData || mMetaData->video_index < 0) {
        return false;
    }
    TrackInfo trackInfo = mMetaData->track_info[mMetaData->video_index];
    AVRational tb = (AVRational) {trackInfo.time_base_num, trackInfo.time_base_den};
    if (playerConfig->enable_accurate_seek &&
        mPlayerLink->vid_accurate_seek_req && !mPlayerLink->seek_req &&
        !mPlayerLink->is_video_high_fps &&
        mPlayerLink->skip_frame >= AVDISCARD_NONREF) {
        const AVRational timebase = {1, AV_TIME_BASE};
        int64_t pts = (pkt->pts == AV_NOPTS_VALUE) ? 0 : av_rescale_q(pkt->pts, tb, timebase);
        int64_t dts = (pkt->dts == AV_NOPTS_VALUE) ? 0 : av_rescale_q(pkt->dts, tb, timebase);
        if (pts > mPlayerLink->seek_pos || dts > mPlayerLink->seek_pos) {
            mPlayerLink->skip_frame =
                    std::min(mPlayerLink->skip_frame, static_cast<int>(AVDISCARD_DEFAULT));
        }
    }
    mPlayerLink->stat.total_packet_count++;
    if (mPlayerLink->nal_length_size &&
        pkt->size > mPlayerLink->nal_length_size + 1 &&
        mPlayerLink->skip_frame >= AVDISCARD_NONREF) {
        int ref_idc       = 0;
        int nuh_layer_id  = 0;
        int nal_unit_type = 0;
        if (trackInfo.codec_id == AV_CODEC_ID_H265) {
            nal_unit_type = NALUnitParser::get_hevc_nal_unit_type(pkt->data);
            nuh_layer_id  = NALUnitParser::get_hevc_nuh_layer_id(pkt->data);
            if (NALUnitParser::is_hevc_no_ref(nal_unit_type) || nuh_layer_id > 0) {
                mPlayerLink->stat.drop_packet_count++;
                return true;
            }
        } else if (trackInfo.codec_id == AV_CODEC_ID_H264) {
            ref_idc       = NALUnitParser::get_h264_ref_idc(pkt->data);
            nal_unit_type = NALUnitParser::get_h264_nal_unit_type(pkt->data);
            if (ref_idc == 0 && nal_unit_type != NAL_SEI) {
                if (pkt->flags & AV_PKT_FLAG_KEY) {
                    mPlayerLink->skip_frame = std::min(mPlayerLink->skip_frame,
                                                       static_cast<int>(AVDISCARD_DEFAULT));
                    return false;
                }
                mPlayerLink->stat.drop_packet_count++;
                return true;
            }
        }
    }
    return false;
}
