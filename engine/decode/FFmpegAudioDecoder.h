#ifndef FFMPEG_AUDIO_DECODER_H
#define FFMPEG_AUDIO_DECODER_H

#include "AudioDecoder.h"

extern "C" {
#include "libavcodec/avcodec.h"
}

class FFmpegAudioDecoder : public AudioDecoder {
public:
    FFmpegAudioDecoder();

    ~FFmpegAudioDecoder() override;

    int Init(AudioCodecConfig &config) override;

    int Decode(const AVPacket *pkt) override;

    int Flush() override;

    int Release() override;

    void SetDecodeCallback(AudioDecodeCallback *callback) override;

private:
    AVCodecContext *mCodecContext;

};

#endif
