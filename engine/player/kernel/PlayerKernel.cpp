
#include "PlayerKernel.h"

#include <sys/socket.h>
#include <unistd.h>

#include "NextMessage.h"

#ifdef __cplusplus
extern "C" {
#endif
#include "libavutil/avstring.h"
#ifdef __cplusplus
}
#endif

#define TAG "RsPlayerKernel"

static InjectCallback s_inject_callback;

int injectCallback(void *opaque, int type, void *data) {
    if (s_inject_callback)
        return s_inject_callback(opaque, type, data);
    return 0;
}

void globalSetInjectCallback(InjectCallback cb) {
    s_inject_callback = std::move(cb);
}

static int app_func_event(ApplicationContext *h, int message, void *data) {
    if (!h || !h->opaque || !data)
        return 0;

    auto *core = reinterpret_cast<PlayerKernel *>(h->opaque);
    sp<PlayerLink> state = core->mVideoState;
    if (!core->getInjectOpaque() || !state)
        return 0;

    return injectCallback(core->getInjectOpaque(), message, data);
}

PlayerKernel::PlayerKernel(NotifyCallback notify_cb)
        : mNotifyCb(std::move(notify_cb)) {}

PlayerKernel::~PlayerKernel() {
    mVideoRenderHandler.reset();
    mAudioRenderHandler.reset();
    mVideoDecHandler.reset();
    mAudioDecHandler.reset();
    mParseHandler.reset();
    resetConfigs();
}

int PlayerKernel::init() {
    try {
        mVideoState = std::make_shared<PlayerLink>();
        mVideoState->audio_clock = std::make_unique<MediaClock>();
        mVideoState->video_clock = std::make_unique<MediaClock>();
        mVideoState->external_clock = std::make_unique<MediaClock>();
        mGeneralConfig = std::make_shared<GeneralConfig>();
        mGeneralConfig->playerConfig = std::make_shared<NextPlayerConfig>();
        mSwsConfig    = std::make_shared<NextDictionary>();
        mSwrConfig    = std::make_shared<NextDictionary>();
        mCodecConfig  = std::make_shared<NextDictionary>();
        mFormatConfig = std::make_shared<NextDictionary>();
        mPlayerConfig = std::make_shared<NextDictionary>();
        auto notifyCb = std::bind(
                &PlayerKernel::notifyListener, this, std::placeholders::_1,
                std::placeholders::_2, std::placeholders::_3, std::placeholders::_4,
                std::placeholders::_5);
        mParseHandler =
                std::make_shared<MediaParseHandler>(mVideoState, notifyCb, THREAD_MEDIA_PARSER);
        mVideoDecHandler = std::make_shared<VideoDecodeHandler>(
                mParseHandler, mVideoState, notifyCb, THREAD_VIDEO_DECODE);
        mAudioDecHandler = std::make_shared<AudioDecodeHandler>(
                mParseHandler, mVideoState, notifyCb, THREAD_AUDIO_DECODE);
        mAudioRenderHandler = std::make_shared<AudioRenderHandler>(
                mAudioDecHandler, mVideoState, notifyCb, THREAD_AUDIO_RENDER);
        mVideoRenderHandler = std::make_shared<VideoRenderHandler>(
                mVideoDecHandler, mVideoState, notifyCb, THREAD_VIDEO_RENDER);
        mAppCtx = reinterpret_cast<ApplicationContext *>(
                malloc(sizeof(ApplicationContext)));
    } catch (const std::bad_alloc &e) {
        NEXT_LOGE(TAG, "[%s:%d] Exception caught: %s!\n", __FUNCTION__, __LINE__,
                e.what());
        return ERROR_OTHER_OOM;
    } catch (...) {
        NEXT_LOGE(TAG, "[%s:%d] Exception caught!\n", __FUNCTION__, __LINE__);
        return ERROR_OTHER_OOM;
    }

    return RESULT_OK;
}

void PlayerKernel::setDataSource(const std::string &url) {
    std::unique_lock<std::mutex> lock(mLock);
    mUrl = url;
}

int PlayerKernel::prepareAsync() {
    if (mUrl.empty() || !mParseHandler) {
        return ERROR_URL_INVALID;
    }

    performConfigs();
    mParseHandler->open(mUrl);
    return performPrepare();
}

int PlayerKernel::start() {
    std::unique_lock<std::mutex> lock(mLock);
    bPausedByUser = false;
    return performStart();
}

int PlayerKernel::pause() {
    std::unique_lock<std::mutex> lock(mLock);
    bPausedByUser = true;
    return performPause();
}

int PlayerKernel::seekTo(int64_t msec, bool flush) {
    std::unique_lock<std::mutex> lock(mLock);
    int64_t duration = mMetaData ? mMetaData->duration / 1000 : 0;

    if (duration > 0 && msec >= duration) {
        notifyListener(MSG_ON_COMPLETED);
        return RESULT_OK;
    }

    mCompleted.store(false);
    mCurSeekPos.store(msec);
    return performSeek(msec);
}

int PlayerKernel::stop() {
    std::unique_lock<std::mutex> lock(mLock);
    bAbort = true;
    return performStop();
}

int PlayerKernel::getCurrentPosition(int64_t &pos) {
    int64_t startTime = 0;
    if (mMetaData && mMetaData->start_time > 0) {
        startTime = mMetaData->start_time / 1000;
    }
    if (mCompleted.load()) {
        pos = mMetaData->duration / 1000;
        return RESULT_OK;
    } else if (mParseHandler && !mSeeking.load() &&
               getMasterClockSerial(mVideoState) == mParseHandler->getSerial()) {
        double clock = getMasterClock(mVideoState);
        pos = clock > 0.0 ? static_cast<int64_t>(clock * 1000) : 0;
    } else {
        int64_t curSeekPos = mCurSeekPos.load();
        pos = curSeekPos > 0 ? curSeekPos : 0;
        return RESULT_OK;
    }
    if (pos < 0 || pos < startTime) {
        pos = 0;
        return RESULT_OK;
    }
    if (pos > startTime) {
        pos -= startTime;
    }
    mVideoState->current_position = pos;
    return RESULT_OK;
}

int PlayerKernel::getDuration(int64_t &duration) {
    if (!mMetaData) {
        duration = 0;
        return ERROR_PLAYER_NOT_INIT;
    }
    duration = mMetaData->duration / 1000;
    return RESULT_OK;
}

void PlayerKernel::setVolume(const float left, const float right) {
    mAudioRenderHandler->setVolume(left, right);
    mVideoState->volume = (left + right) / 2;
}

void PlayerKernel::notifyListener(int what, int arg1, int arg2, void *obj, int len) {
    std::unique_lock<std::mutex> lock(mNotifyCbLock);
    if (mNotifyCb) {
        switch (what) {
            case MSG_ON_COMPLETED: {
                mCompleted.store(true);
                PlayerConfig *playerConfig = mGeneralConfig->playerConfig->get();
                if (playerConfig->loop != 1 &&
                    (!playerConfig->loop || --playerConfig->loop)) {
                    what = REQUEST_SEEK;
                    arg1 = 0;
                    mVideoState->loop_count++;
                }
                break;
            }
            case MSG_BUFFER_START: {
                performPause();
                break;
            }
            case MSG_BUFFER_END: {
                if (!bPausedByUser) {
                    performStart();
                }
                break;
            }
            case MSG_SEEK_COMPLETE: {
                if (arg1 == mCurSeekPos) {
                    mSeeking.store(false);
                }
                performFlush();
                break;
            }
            case REQUEST_START: {
                if (!bPausedByUser) {
                    performStart();
                }
                return;
            }
            case REQUEST_KERNEL_PAUSE: {
                performPause();
                return;
            }
            case REQUEST_PLAY_SPEED: {
                float playbackRate = (float)arg1 / 100;
                setPlaybackRate(playbackRate);
                return;
            }
            default:
                break;
        }
        mNotifyCb(what, arg1, arg2, obj, len);
    }
}

int PlayerKernel::performConfigs() {
    resetConfigs();
    configInternal();

    if (mParseHandler) {
        mParseHandler->setConfig(mGeneralConfig);
    }
    if (mVideoDecHandler) {
        mVideoDecHandler->setConfig(mGeneralConfig);
    }
    if (mAudioDecHandler) {
        mAudioDecHandler->setConfig(mGeneralConfig);
    }
    if (mVideoRenderHandler) {
        mVideoRenderHandler->setConfig(mGeneralConfig);
    }
    if (mAudioRenderHandler) {
        mAudioRenderHandler->setConfig(mGeneralConfig);
    }
    return RESULT_OK;
}

int PlayerKernel::resetConfigs() {
    mGeneralConfig->playerConfig->reset();
    av_dict_free(&mGeneralConfig->formatConfig);
    av_dict_free(&mGeneralConfig->codecConfig);
    av_dict_free(&mGeneralConfig->swsConfig);
    av_dict_free(&mGeneralConfig->swrConfig);
    return RESULT_OK;
}

int PlayerKernel::configInternal() {
    /// player config
    sp<NextPlayerConfig> playerConfig = mGeneralConfig->playerConfig;
    auto *target_obj = reinterpret_cast<uint8_t *>(playerConfig->get());
    for (const auto & i : AvConfigs) {
        const char *name = i.name;
        int offset = i.offset;
        OptionType type = i.type;
        int64_t default_i64 = i.defaultVal.i64;
        const char *default_str = i.defaultVal.str;
        void *dst = target_obj + offset;

        if (!name || !dst) {
            continue;
        }

        if (type == OPTION_TYPE_INT32) {
            *(reinterpret_cast<int32_t *>(dst)) =
                    static_cast<int32_t>(mPlayerConfig->getInt64(name, default_i64));
        } else if (type == OPTION_TYPE_INT64) {
            *(reinterpret_cast<int64_t *>(dst)) =
                    static_cast<int64_t>(mPlayerConfig->getInt64(name, default_i64));
        } else if (type == OPTION_TYPE_STRING) {
            std::string str = mPlayerConfig->getString(name, (std::string *) default_str);
            if (!str.empty()) {
                *(reinterpret_cast<uint8_t **>(dst)) = reinterpret_cast<uint8_t *>(strdup(str.c_str()));
            }
        }
    }

    /// format config
    for (size_t i = 0; i < mFormatConfig->getSize(); ++i) {
        ValueType type = VALUE_TYPE_UNKNOWN;
        const char *name = mFormatConfig->getEntryNameAt(i, &type);
        if (!name) {
            continue;
        }
        if (type == VALUE_TYPE_INT64) {
            av_dict_set_int(&mGeneralConfig->formatConfig, name, mFormatConfig->getInt64(name, 0), 0);
        } else if (type == VALUE_TYPE_STRING) {
            av_dict_set(&mGeneralConfig->formatConfig, name, mFormatConfig->getString(name, nullptr).c_str(), 0);
        }
    }
    if (!av_dict_get(mGeneralConfig->formatConfig, "scan_all_pmts", nullptr, AV_DICT_MATCH_CASE)) {
        av_dict_set(&mGeneralConfig->formatConfig, "scan_all_pmts", "1", AV_DICT_DONT_OVERWRITE);
    }
    if (av_stristart(mUrl.c_str(), "rtmp", nullptr) || av_stristart(mUrl.c_str(), "rtsp", nullptr)) {
        NEXT_LOGI(TAG, "Remove 'timeout' option for rtmp.\n");
        av_dict_set(&mGeneralConfig->formatConfig, "timeout", nullptr, 0);
    }
//    av_dict_set_intptr(&mGeneralConfig->formatConfig, "application",
//                       (uintptr_t) mAppCtx, 0); // todo: deprecated

    /// codec config
    for (size_t i = 0; i < mCodecConfig->getSize(); ++i) {
        ValueType type = VALUE_TYPE_UNKNOWN;
        const char *name = mCodecConfig->getEntryNameAt(i, &type);
        if (!name) {
            continue;
        }
        if (type == VALUE_TYPE_INT64) {
            av_dict_set_int(&mGeneralConfig->codecConfig, name, mCodecConfig->getInt64(name, 0), 0);
        } else if (type == VALUE_TYPE_STRING) {
            av_dict_set(&mGeneralConfig->codecConfig, name, mCodecConfig->getString(name, nullptr).c_str(), 0);
        }
    }

    /// sws config
    for (size_t i = 0; i < mSwsConfig->getSize(); ++i) {
        ValueType type = VALUE_TYPE_UNKNOWN;
        const char *name = mSwsConfig->getEntryNameAt(i, &type);
        if (!name) {
            continue;
        }
        if (type == VALUE_TYPE_INT64) {
            av_dict_set_int(&mGeneralConfig->swsConfig, name, mSwsConfig->getInt64(name, 0), 0);
        } else if (type == VALUE_TYPE_STRING) {
            av_dict_set(&mGeneralConfig->swsConfig, name, mSwsConfig->getString(name, nullptr).c_str(), 0);
        }
    }

    return RESULT_OK;
}

int PlayerKernel::performStart() {
    if (bAbort) {
        return RESULT_OK;
    }
    mVideoState->pause_req = false;
    mVideoState->paused = false;
    if (mVideoRenderHandler) {
        mVideoRenderHandler->startRender();
    }
    if (mAudioRenderHandler) {
        mAudioRenderHandler->startRender();
    }

    return RESULT_OK;
}

int PlayerKernel::performPrepare() {
    if (bAbort) {
        return RESULT_OK;
    }
    if (mParseHandler) {
        mParseHandler->setPrepareCb(
                std::bind(&PlayerKernel::preparedCallback, this, std::placeholders::_1));
        return mParseHandler->prepareAsync();
    }
    return ERROR_PLAYER_NOT_INIT;
}

int PlayerKernel::performSeek(int64_t msec) {
    if (bAbort) {
        return RESULT_OK;
    }
    if (!mParseHandler) {
        return ERROR_PLAYER_NOT_INIT;
    }
    mSeeking.store(true);
    return mParseHandler->seek(msec);
}

int PlayerKernel::performPause() {
    if (bAbort) {
        return RESULT_OK;
    }
    mVideoState->pause_req = true;
    mVideoState->paused = true;
    if (mVideoRenderHandler) {
        mVideoRenderHandler->pauseRender();
    }
    if (mAudioRenderHandler) {
        mAudioRenderHandler->pauseRender();
    }
    return RESULT_OK;
}

int PlayerKernel::performFlush() {
    if (bAbort) {
        return RESULT_OK;
    }

    if (mVideoDecHandler) {
        mVideoDecHandler->resetEof();
    }
    if (mAudioDecHandler) {
        mAudioDecHandler->resetEof();
    }
    if (mVideoRenderHandler) {
        mVideoRenderHandler->flush();
    }
    if (mAudioRenderHandler) {
        mAudioRenderHandler->flush();
    }

    return RESULT_OK;
}

int PlayerKernel::performStop() {
    if (mAudioRenderHandler) {
        mAudioRenderHandler->stop();
    }
    if (mVideoRenderHandler) {
        mVideoRenderHandler->stop();
    }
    if (mAudioDecHandler) {
        mAudioDecHandler->stop();
    }
    if (mVideoDecHandler) {
        mVideoDecHandler->stop();
    }
    if (mParseHandler) {
        mParseHandler->stop();
    }
    return RESULT_OK;
}

void PlayerKernel::release() {
    NEXT_LOGD(TAG, "%s Start\n", __func__);
    mParseHandler->setPrepareCb(nullptr);
    mParseHandler->release();
    if (mAppCtx) {
        free(mAppCtx);
        mAppCtx = nullptr;
    }

    if (mVideoDecHandler) {
        mVideoDecHandler->release();
    }
    if (mAudioDecHandler) {
        mAudioDecHandler->release();
    }
    if (mVideoRenderHandler) {
        mVideoRenderHandler->release();
    }
    if (mAudioRenderHandler) {
        mAudioRenderHandler->release();
    }
    if (mFd >= 0) {
        close(static_cast<int>(mFd));
        mFd = -1;
    }
    NEXT_LOGD(TAG, "%s end\n", __func__);
}

// cache config
int PlayerKernel::setConfig(int type, const std::string &name, const std::string &value) {
    std::lock_guard<std::mutex> lock(mLock);
    switch (type) {
        case CONFIG_TYPE_FORMAT:
            mFormatConfig->setString(name.c_str(), value);
            break;
        case CONFIG_TYPE_CODEC:
            mCodecConfig->setString(name.c_str(), value);
            break;
        case CONFIG_TYPE_SWR:
            mSwrConfig->setString(name.c_str(), value);
            break;
        case CONFIG_TYPE_SWS:
            mSwsConfig->setString(name.c_str(), value);
            break;
        case CONFIG_TYPE_PLAYER:
            mPlayerConfig->setString(name.c_str(), value);
            break;
        default:
            break;
    }
    return RESULT_OK;
}

int PlayerKernel::setConfig(int type, const std::string &name, int64_t value) {
    std::lock_guard<std::mutex> lock(mLock);
    switch (type) {
        case CONFIG_TYPE_FORMAT:
            mFormatConfig->setInt64(name.c_str(), value);
            break;
        case CONFIG_TYPE_CODEC:
            mCodecConfig->setInt64(name.c_str(), value);
            break;
        case CONFIG_TYPE_SWR:
            mSwrConfig->setInt64(name.c_str(), value);
            break;
        case CONFIG_TYPE_SWS:
            mSwsConfig->setInt64(name.c_str(), value);
            break;
        case CONFIG_TYPE_PLAYER:
            mPlayerConfig->setInt64(name.c_str(), value);
            break;
        default:
            break;
    }
    return RESULT_OK;
}

void PlayerKernel::setPlaybackRate(float rate) {
    if (std::abs(mVideoState->play_rate - rate) < FLT_EPSILON) {
        return;
    }
    if (mAudioRenderHandler) {
        mAudioRenderHandler->setPlaybackRate(rate);
    }
    mVideoState->audio_clock->setSpeed(rate);
    mVideoState->video_clock->setSpeed(rate);
    mVideoState->play_rate = rate;
}

int PlayerKernel::setOption(int key, int64_t value) {
    return RESULT_OK;
}

int PlayerKernel::setOption(int key, const float value) {
    std::unique_lock<std::mutex> lock(mLock);
    switch (key) {
        case OPTION_FLOAT_PLAYBACK_RATE: {
            setPlaybackRate(value);
            break;
        }
        case OPTION_FLOAT_PLAYBACK_VOLUME: {
            setVolume(value, value);
            break;
        }
        default:
            break;
    }
    return RESULT_OK;
}

int64_t PlayerKernel::getOption(int key, int64_t defaultVal) {
    switch (key) {
        case OPTION_INT64_CUR_VIDEO_STREAM:
            if (mMetaData) {
                return mMetaData->video_index;
            }
            break;
        case OPTION_INT64_CUR_AUDIO_STREAM:
            if (mMetaData) {
                return mMetaData->audio_index;
            }
            break;
        case OPTION_INT64_VIDEO_DECODER:
            return mVideoState->stat.video_dec_type;
        case OPTION_INT64_AUDIO_DECODER: {
            if (mAudioDecHandler) {
                return OPTION_STR_DECODER_AVCODEC;
            }
            break;
        }
        case OPTION_INT64_VIDEO_CACHE_TIME:
            return mVideoState->stat.video_cache.duration;
        case OPTION_INT64_AUDIO_CACHE_TIME:
            return mVideoState->stat.audio_cache.duration;
        case OPTION_INT64_VIDEO_CACHE_SIZE:
            return mVideoState->stat.video_cache.bytes;
        case OPTION_INT64_AUDIO_CACHE_SIZE:
            return mVideoState->stat.audio_cache.bytes;
        case OPTION_INT64_VIDEO_CACHE_PKT:
            return mVideoState->stat.video_cache.packets;
        case OPTION_INT64_AUDIO_CACHE_PKT:
            return mVideoState->stat.audio_cache.packets;
        case OPTION_INT64_BIT_RATE:
            return mVideoState->stat.bit_rate;
        case OPTION_INT64_TCP_SPEED:
            return mVideoState->stat.net_speed_meter.getSpeed();
        case OPTION_INT64_LAST_TCP_SPEED:
            return mVideoState->stat.net_speed_meter.getLastSpeed();
        case OPTION_INT64_SEEK_LOAD_TIME:
            return mVideoState->stat.last_seek_time;
        case OPTION_INT64_TRANSFER_BYTES:
            return mVideoState->stat.transfer_bytes;
        case OPTION_INT64_CACHE_SIZE:
            return mVideoState->stat.cache_file_size;
        case OPTION_INT64_CACHE_POSITION:
            return mVideoState->stat.cache_time_pos;
        case OPTION_INT64_CACHE_FILE_POS:
            return mVideoState->stat.cache_file_pos;
        case OPTION_INT64_FILE_SIZE:
            return mVideoState->stat.real_file_size;
        case OPTION_INT64_MAX_BUFFER_SIZE:
            return mGeneralConfig->playerConfig->get()->dcc.max_buffer_size;
        case OPTION_INT64_PIXEL_FORMAT:
            return mVideoState->stat.pixel_format;
        case OPTION_INT64_VIDEO_WIDTH:
            return mVideoState->width;
        case OPTION_INT64_VIDEO_HEIGHT:
            return mVideoState->height;
        case OPTION_INT64_VIDEO_DECODE_RATE:
            return mVideoState->stat.decode_rate;
        case OPTION_INT64_VIDEO_RENDER_RATE:
            return mVideoState->stat.render_rate;
        default:
            NEXT_LOGE(TAG, "unknown option: %d", key);
            break;
    }
    return defaultVal;
}

float PlayerKernel::getOption(int key, const float defaultVal) {
    switch (key) {
        case OPTION_FLOAT_AV_DELAY:
            return mVideoState->stat.av_delay;
        case OPTION_FLOAT_AV_DIFF:
            return mVideoState->stat.av_diff;
        case OPTION_FLOAT_DROP_PACKET_RATE:
            return mVideoState->stat.total_packet_count == 0
                   ? defaultVal
                   : static_cast<float>(mVideoState->stat.drop_packet_count) /
                           static_cast<float>(mVideoState->stat.total_packet_count);
        case OPTION_FLOAT_DROP_FRAME_RATE:
            return mVideoState->stat.drop_frame_rate;
        case OPTION_FLOAT_PLAYBACK_RATE:
            return mVideoState->play_rate;
        case OPTION_FLOAT_PLAYBACK_VOLUME:
            return mVideoState->volume;
        case OPTION_FLOAT_VIDEO_FRAME_RATE:
            if (mMetaData && mMetaData->video_index >= 0) {
                TrackInfo track_info = mMetaData->track_info[mMetaData->video_index];
                if (track_info.fps_den > 0) {
                    return static_cast<float>(track_info.fps_num) / static_cast<float>(track_info.fps_den);
                }
            }
            break;
        default:
            break;
    }
    return defaultVal;
}

void PlayerKernel::setNotifyCb(NotifyCallback cb) {
    std::unique_lock<std::mutex> lock(mNotifyCbLock);
    mNotifyCb = std::move(cb);
}

void *PlayerKernel::setInjectOpaque(void *opaque) {
    void *prev_weak_thiz = mInjectOpaque.load();
    mInjectOpaque.store(opaque);
    if (!opaque) {
        return prev_weak_thiz;
    }

    memset(mAppCtx, 0, sizeof(ApplicationContext));
    mAppCtx->app_func_event = app_func_event;
    mAppCtx->opaque = reinterpret_cast<void *>(this);
    return prev_weak_thiz;
}

void *PlayerKernel::getInjectOpaque() { return mInjectOpaque.load(); }

sp<NextDictionary> PlayerKernel::getConfig(int type) {
    std::unique_lock<std::mutex> lock(mLock);
    sp<NextDictionary> ret;
    switch (type) {
        case CONFIG_TYPE_FORMAT:
            return mFormatConfig;
        case CONFIG_TYPE_CODEC:
            return mCodecConfig;
        case CONFIG_TYPE_SWR:
            return mSwrConfig;
        case CONFIG_TYPE_SWS:
            return mSwsConfig;
        case CONFIG_TYPE_PLAYER:
            return mPlayerConfig;
        default:
            break;
    }
    return ret;
}

int PlayerKernel::getVideoCodecInfo(std::string &videoInfo) const {
    videoInfo = mVideoState->video_codec_name + ", " + mVideoState->video_codec_type;
    return RESULT_OK;
}

int PlayerKernel::getAudioCodecInfo(std::string &audioInfo) const {
    audioInfo = mVideoState->audio_codec_name + ", " + mVideoState->audio_codec_type;
    return RESULT_OK;
}

void PlayerKernel::preparedCallback(sp<MetaData> &metadata) {
    PlayerConfig *playerConfig = mGeneralConfig->playerConfig->get();
    if (!playerConfig->start_on_prepared) {
        performPause();
    }
    if (metadata) {
        mMetaData = metadata;
        mVideoState->audio_stream_index = mMetaData->audio_index;
        mVideoState->video_stream_index = mMetaData->video_index;
        mVideoState->stat.bit_rate = mMetaData->bit_rate;
        prepareStream(metadata);
        if (mVideoState->video_stream_index >= 0) {
            int video_idx = mVideoState->video_stream_index;
            notifyListener(MSG_VIDEO_SIZE_CHANGED,
                           metadata->track_info[video_idx].width,
                           metadata->track_info[video_idx].height);
            notifyListener(MSG_SAR_CHANGED,
                           metadata->track_info[video_idx].sar_num,
                           metadata->track_info[video_idx].sar_den);
        }
        if (playerConfig->start_on_prepared) {
            performStart();
        }
    } else {
        NEXT_LOGE(TAG, "Fail to find stream info!\n");
        notifyListener(MSG_ON_ERROR, ERROR_PARSE_STREAM_OPEN);
    }
    notifyListener(MSG_ON_PREPARED);

    if (playerConfig->seek_at_start > 0) {
        seekTo(playerConfig->seek_at_start, true);
    }
}

void PlayerKernel::prepareStream(sp<MetaData> &metadata) {
    int ret = RESULT_OK;
    PlayerConfig *playerConfig = mGeneralConfig->playerConfig->get();
    if (bAbort || !metadata || !playerConfig)
        return;
    if (mVideoState->audio_stream_index < 0 && mVideoState->video_stream_index < 0) {
        NEXT_LOGE(TAG, "No available stream!\n");
        notifyListener(MSG_ON_ERROR, ERROR_PARSE_FIND_STREAM);
        return;
    }

    if (mVideoState->audio_stream_index >= 0 && !playerConfig->audio_disable && mAudioDecHandler) {
        ret = mAudioDecHandler->prepare(metadata);
        if (ret == RESULT_OK && mAudioRenderHandler) {
            ret = mAudioRenderHandler->prepare(metadata);
            if (ret == RESULT_OK) {
                mVideoState->av_sync_type = CLOCK_AUDIO;
                NEXT_LOGI(TAG, "prepare audio stream success\n");
            }
        }
        if (ret != RESULT_OK) {
            NEXT_LOGE(TAG, "%s audio failed\n", __func__);
            mVideoState->audio_stream_index = -1;
            mAudioDecHandler->release();
            mAudioRenderHandler->release();
            mAudioDecHandler.reset();
            mAudioDecHandler.reset();
            mAudioRenderHandler.reset();
            mAudioRenderHandler.reset();
        }
    } else {
        NEXT_LOGE(TAG, "Don't have audio stream\n");
        mAudioDecHandler.reset();
        mAudioDecHandler.reset();
        mAudioRenderHandler.reset();
        mAudioRenderHandler.reset();
    }

    if (mVideoState->video_stream_index >= 0 && !playerConfig->video_disable && mVideoDecHandler) {
        TrackInfo track_info = metadata->track_info[mVideoState->video_stream_index];
        parseExtraData(track_info);
        ret = mVideoDecHandler->init(metadata);
        if (ret == RESULT_OK && mVideoRenderHandler) {
            ret = mVideoRenderHandler->prepare(metadata);
            if (ret == RESULT_OK) {
                if (mVideoState->av_sync_type != CLOCK_AUDIO) {
                    mVideoState->av_sync_type = CLOCK_VIDEO;
                }
            }
            mVideoState->width  = track_info.width;
            mVideoState->height = track_info.height;
        }
        if (ret != RESULT_OK) {
            NEXT_LOGE(TAG, "%s video failed\n", __func__);
            mVideoState->video_stream_index = -1;
            mVideoDecHandler->release();
            mVideoRenderHandler->release();
            mVideoDecHandler.reset();
            mVideoDecHandler.reset();
            mVideoRenderHandler.reset();
            mVideoRenderHandler.reset();
        }
    } else {
        NEXT_LOGE(TAG, "Stream not have Video\n");
        mVideoDecHandler.reset();
        mVideoDecHandler.reset();
        mVideoRenderHandler.reset();
        mVideoRenderHandler.reset();
    }
    notifyListener(MSG_COMPONENT_OPEN);
}

void PlayerKernel::parseExtraData(TrackInfo &info) const {
    if (info.codec_id == AV_CODEC_ID_H264 && info.extra_data_size > 3 &&
        info.extra_data && info.extra_data[0]) {
        mVideoState->nal_length_size = (info.extra_data[4] & 0x03) + 1;
    } else if (info.codec_id == AV_CODEC_ID_H265 && info.extra_data &&
               (info.extra_data[0] || info.extra_data[1] ||
                info.extra_data[2] > 1)) {
        if (info.extra_data_size > 21) {
            mVideoState->nal_length_size = (info.extra_data[21] & 0x03) + 1;
        }
    }
}

#if defined(__ANDROID__) || defined(__HARMONY__)

int PlayerKernel::setVideoSurface(ANativeWindow *surface) {
    if (!mVideoDecHandler || !mVideoRenderHandler) {
        return ERROR_PLAYER_NOT_INIT;
    }

    mVideoRenderHandler->setVideoSurface(surface);

    if (mVideoDecHandler->setNativeSurface(surface) == ERROR_RENDER_VIDEO_SUR) {
        int64_t pos = 0;
        getCurrentPosition(pos);
        if (surface && pos >= 0 &&
            mVideoState->stat.video_dec_type == OPTION_STR_DECODER_MEDIACODEC) {
            seekTo(pos);
        }
    }
    return RESULT_OK;
}

#endif

#if defined(__APPLE__)
UIView *CRedCore::initWithFrame(int type, CGRect cgrect) {
  if (!mVideoRenderHandler) {
    return nullptr;
  }
  return mVideoRenderHandler->initWithFrame(type, cgrect);
}
#endif
