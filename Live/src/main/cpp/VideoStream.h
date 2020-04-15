
#ifndef VIDEOSTREAM_H
#define VIDEOSTREAM_H

#include <inttypes.h>
#include <pthread.h>
#include "include/rtmp/rtmp.h"
#include "include/x264/x264.h"

class VideoStream {
    typedef void (*VideoCallback)(RTMPPacket *packet);

public:
    VideoStream();

    ~VideoStream();

    void setVideoEncInfo(int width, int height, int fps, int bitrate);

    void encodeData(int8_t *data);

    void encodeDataNew(int8_t *y_plane, int8_t *u_plane, int8_t *v_plane);

    void setVideoCallback(VideoCallback videoCallback);

private:
    pthread_mutex_t mutex;
    int mWidth;
    int mHeight;
    int mFps;
    int mBitrate;
    x264_t *videoCodec = 0;
    x264_picture_t *pic_in = 0;

    int ySize;
    int uvSize;
    VideoCallback videoCallback;

    void sendSpsPps(uint8_t *sps, uint8_t *pps, int sps_len, int pps_len);

    void sendFrame(int type, uint8_t *payload, int i_payload);
};

#endif
