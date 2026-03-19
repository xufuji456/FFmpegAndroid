/**
 * Note: define of next player
 * Date: 2026/3/12
 * Author: frank
 */

#ifndef NEXTPLAYER_DEFINE_H
#define NEXTPLAYER_DEFINE_H

#include "MediaClock.h"
#include "NextSpeedMeter.h"

// thread name
#define THREAD_MEDIA_PARSER "MediaParser"
#define THREAD_VIDEO_DECODE "VideoDecode"
#define THREAD_AUDIO_DECODE "AudioDecode"
#define THREAD_VIDEO_RENDER "VideoRender"
#define THREAD_AUDIO_RENDER "AudioRender"

// decoder type
#define AVCODEC_MODULE_NAME      "avcodec"
#define MEDIACODEC_MODULE_NAME   "MediaCodec"
#define VIDEOTOOLBOX_MODULE_NAME "VideoToolBox"

#define SLEEP_10MS 10
#define SLEEP_20MS 20
#define SLEEP_10MS_CONVERT_US (10 * 1000)
#define SLEEP_20MS_CONVERT_US (20 * 1000)

#define MAX_PKT_QUEUE_DEEP 300
#define MAX_RETRY_COUNT    3
#define MIN_BUFFER_NOTIFY  5
#define AUDIO_CACHE_64K  (64 * 1024)
#define VIDEO_CACHE_256K (256 * 1024)

// render rate
#define REFRESH_RATE 0.01
#define MAX_FRAME_DURATION 10.0
#define FRAME_DROP_THRESHOLD 0.15
#define AV_SYNC_THRESHOLD_MIN 0.04
#define AV_SYNC_THRESHOLD_MAX 0.1

enum PlayerState {
    MP_STATE_IDLE = 0,
    MP_STATE_INITIALIZED,
    MP_STATE_ASYNC_PREPARING,
    MP_STATE_PREPARED,
    MP_STATE_STARTED,
    MP_STATE_PAUSED,
    MP_STATE_STOPPED,
    MP_STATE_COMPLETED,
    MP_STATE_ERROR,
    MP_STATE_END,
    MP_STATE_INTERRUPT,
    MP_STATE_UNKNOWN
};

typedef struct AVCacheStatistic {
    int64_t bytes    = 0;
    int64_t packets  = 0;
    int64_t duration = 0;
} AVCacheStatistic;

// player stat
typedef struct AVStatistic {

    float av_diff         = 0.0; // diff of clock
    float av_delay        = 0.0; // delay of clock
    float render_rate     = 0.0;
    float decode_rate     = 0.0;
    float drop_frame_rate = 0.0;

    int64_t bit_rate        = 0;
    int64_t last_seek_time  = 0;
    int64_t transfer_bytes  = 0;
    int64_t cache_time_pos  = 0;
    int64_t cache_file_pos  = 0;
    int64_t real_file_size  = 0;
    int64_t cache_file_size = 0;
    int64_t last_cache_pos  = 0;
    int64_t real_cache_size = 0;

    int pixel_format        = 0;
    int video_dec_type      = 0;
    int drop_frame_count    = 0;
    int drop_packet_count   = 0;
    int total_packet_count  = 0;
    int decoded_frame_count = 0;
    int continue_drop_frame = 0;

    AVCacheStatistic video_cache;
    AVCacheStatistic audio_cache;

    NetworkSpeedMeter net_speed_meter;

} AVStatistic;


struct PlayerLink {
    AVStatistic stat;
    std::string play_url;
    std::string video_codec_name;
    std::string video_codec_type;
    std::string audio_codec_name;
    std::string audio_codec_type;

    float volume                = 1.0f;
    float play_rate             = 1.0f;

    int error_code              = 0;
    int loop_count              = 0;
    int skip_frame              = 0;
    int nal_length_size         = 0;
    int drop_aframe_count       = 0;
    int drop_vframe_count       = 0;
    int audio_stream_index      = -1;
    int video_stream_index      = -1;

    int64_t seek_pos            = 0;
    int64_t current_position    = 0;
    int64_t playable_duration   = 0;
    int64_t accurate_seek_start = 0;

    bool paused                 = false;
    bool seek_req               = false;
    bool pause_req              = false;
    bool hw_decode              = false;
    bool audio_dec_finish       = false;
    bool video_dec_finish       = false;
    bool is_video_high_fps      = false;
    bool step_to_next_frame     = false;
    bool first_video_rendered   = false;
    bool first_audio_rendered   = false;
    bool aud_accurate_seek_req  = false;
    bool vid_accurate_seek_req  = false;

    std::mutex accurate_seek_mutex;
    AVCLockType av_sync_type = CLOCK_EXTERNAL;
    std::atomic<int> last_video_seek_serial = -1;
    std::atomic<int> last_audio_seek_serial = -1;
    volatile int64_t last_seek_load_start    = 0;
    volatile int64_t accurate_seek_video_pts = 0;
    volatile int64_t accurate_seek_audio_pts = 0;
    std::unique_ptr<NextMediaClock> audio_clock;
    std::unique_ptr<NextMediaClock> video_clock;
    std::unique_ptr<NextMediaClock> external_clock;
    std::condition_variable video_accurate_seek_cond;
    std::condition_variable audio_accurate_seek_cond;

};

static inline int getMasterSyncType(std::shared_ptr<PlayerLink> &pLink) {
    if (!pLink) {
        return CLOCK_EXTERNAL;
    }
    if (pLink->av_sync_type == CLOCK_VIDEO) {
        if (pLink->video_stream_index >= 0) {
            return CLOCK_VIDEO;
        } else {
            return CLOCK_AUDIO;
        }
    } else if (pLink->av_sync_type == CLOCK_AUDIO) {
        if (pLink->audio_stream_index >= 0) {
            return CLOCK_AUDIO;
        } else {
            return CLOCK_EXTERNAL;
        }
    } else {
        return CLOCK_EXTERNAL;
    }
}

static inline double getMasterClock(std::shared_ptr<PlayerLink> &pLink) {
    double clock = NAN;

    if (!pLink) {
        return clock;
    }

    switch (getMasterSyncType(pLink)) {
        case CLOCK_VIDEO:
            clock = pLink->video_clock->GetClock();
            break;
        case CLOCK_AUDIO:
            clock = pLink->audio_clock->GetClock();
            break;
        default:
            clock = pLink->external_clock->GetClock();
            break;
    }
    return clock;
}

static inline int getMasterClockSerial(std::shared_ptr<PlayerLink> &pLink) {
    int serial = -1;

    if (!pLink) {
        return serial;
    }

    switch (getMasterSyncType(pLink)) {
        case CLOCK_VIDEO:
            serial = pLink->video_clock->GetClockSerial();
            break;
        case CLOCK_AUDIO:
            serial = pLink->audio_clock->GetClockSerial();
            break;
        default:
            serial = pLink->external_clock->GetClockSerial();
            break;
    }
    return serial;
}

#endif //NEXTPLAYER_DEFINE_H
