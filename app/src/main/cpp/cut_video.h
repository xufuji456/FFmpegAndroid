//
// Created by xu fulong on 2022/5/13.
//

#ifndef CUT_VIDEO_H
#define CUT_VIDEO_H

#ifdef __cplusplus
extern "C" {
#endif
#include "libavformat/avformat.h"
#ifdef __cplusplus
}
#endif

class CutVideo {
private:

    int64_t m_startTime = 15;
    int64_t m_duration  = 10;

    int64_t *dts_start_offset;
    int64_t *pts_start_offset;

    AVFormatContext *ofmt_ctx = nullptr;

    AVPacket* copy_packet(AVFormatContext *ifmt_ctx, AVPacket *packet);

    int write_internal(AVFormatContext *ifmt_ctx, AVPacket *packet);

public:

    int open_output_file(AVFormatContext *ifmt_ctx, const char *filename);

    void setParam(int64_t start_time, int64_t duration);

    void write_output_file(AVFormatContext *ifmt_ctx, AVPacket *packet);

    void close_output_file();
};

#endif //CUT_VIDEO_H
