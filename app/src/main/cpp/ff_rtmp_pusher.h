//
// Created by xu fulong on 2022/9/9.
//

#ifndef FF_RTMP_PUSHER_H
#define FF_RTMP_PUSHER_H

#include "ffmpeg_jni_define.h"

#ifdef __cplusplus
extern "C" {
#endif
#include "libavformat/avformat.h"
#include "libavcodec/avcodec.h"
#include "libavutil/time.h"
#ifdef __cplusplus
}
#endif

class FFRtmpPusher {
private:
    AVFormatContext *inFormatCtx;
    AVFormatContext *outFormatCtx;

    AVPacket packet;
    int video_index = -1;
    int audio_index = -1;

public:

    int open(const char *inputPath, const char *outputPath);

    int push();

    void close();

};

#endif //FF_RTMP_PUSHER_H
