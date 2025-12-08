#ifndef FFMPEG_VIDEO_DECODER_H
#define FFMPEG_VIDEO_DECODER_H

#include "decode/common/VideoCodecInfo.h"
#include "decode/VideoDecoder.h"

#ifdef __cplusplus
extern "C" {
#endif
#include "libavcodec/avcodec.h"
#ifdef __cplusplus
}
#endif

class FFmpegVideoDecoder : public VideoDecoder {
public:
    explicit FFmpegVideoDecoder(int codecId);

    ~FFmpegVideoDecoder() override;

    int Init(const MetaData *metadata) override;

    void SetDecodeCallback(VideoDecodeCallback *callback) override;

    int Decode(const AVPacket *pkt) override;

    int Flush() override;

    int SetVideoFormat(const MetaData *metadata) override;

    int Release() override;

private:
    bool bFlushState = false;
    AVCodecContext *mCodecContext = nullptr;

};

#endif
