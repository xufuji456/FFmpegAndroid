//
// Created by xu fulong on 2022/9/22.
//

#ifndef FFMPEGANDROID_FFPLAY_DEFINE_H
#define FFMPEGANDROID_FFPLAY_DEFINE_H

#include <mutex>
#include "FFMessageQueue.h"

#ifdef __cplusplus
extern "C" {
#endif
#include <libavformat/avformat.h>
#ifdef __cplusplus
}
#endif

#define PACKET_QUEUE_SIZE 16
#define FRAME_QUEUE_SIZE 16

#define VIDEO_QUEUE_SIZE 5
#define SAMPLE_QUEUE_SIZE 9

#define MAX_QUEUE_SIZE (15 * 1024 * 1024)
#define MIN_FRAMES 25

#define AUDIO_MIN_BUFFER_SIZE 512
#define AUDIO_MAX_CALLBACKS_PER_SEC 30

#define REFRESH_RATE 0.01

#define AV_SYNC_THRESHOLD_MIN 0.04
#define AV_SYNC_THRESHOLD_MAX 0.1
#define AV_SYNC_FRAMEDUP_THRESHOLD 0.1
#define AV_NOSYNC_THRESHOLD 10.0

#define EXTERNAL_CLOCK_MIN_FRAMES 2
#define EXTERNAL_CLOCK_MAX_FRAMES 10
#define EXTERNAL_CLOCK_SPEED_MIN  0.900
#define EXTERNAL_CLOCK_SPEED_MAX  1.010
#define EXTERNAL_CLOCK_SPEED_STEP 0.001

struct AVDictionary {
    int count;
    AVDictionaryEntry *elements;
};

typedef enum {
    AV_SYNC_AUDIO,      // 同步到音频时钟
    AV_SYNC_VIDEO,      // 同步到视频时钟
    AV_SYNC_EXTERNAL,   // 同步到外部时钟
} SyncType;

class PlayerParams {

public:
    std::mutex mutex;

    MessageQueue *messageQueue;
    int64_t videoDuration;

    AVInputFormat *iformat;
    const char *url;
    int64_t offset;
    const char *headers;

    const char *audioCodecName;
    const char *videoCodecName;

    int abortRequest;
    int pauseRequest;
    SyncType syncType;
    int64_t startTime;
    int64_t duration;
    int realTime;
    int infiniteBuffer;
    int audioDisable;
    int videoDisable;
    int displayDisable;

    int fast;
    int genpts;
    int lowres;

    float playbackRate;
    float playbackPitch;

    int seekByBytes;
    int seekRequest;
    int seekFlags;
    int64_t seekPos;
    int64_t seekRel;

    int autoExit;
    int loop;
    int mute;
    int reorderVideoPts;
};

#endif //FFMPEGANDROID_FFPLAY_DEFINE_H
