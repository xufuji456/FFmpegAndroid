//
// Created by xu fulong on 2022/9/7.
//

#ifndef FFMPEGANDROID_FF_AUDIO_RESAMPLE_H
#define FFMPEGANDROID_FF_AUDIO_RESAMPLE_H

#include "ffmpeg_jni_define.h"

#ifdef __cplusplus
extern "C" {
#endif
#include "libavformat/avformat.h"
#include "libavformat/avio.h"

#include "libavcodec/avcodec.h"

#include "libavutil/audio_fifo.h"
#include "libavutil/avassert.h"
#include "libavutil/avstring.h"
#include "libavutil/frame.h"
#include "libavutil/opt.h"

#include "libswresample/swresample.h"
#ifdef __cplusplus
}
#endif

struct AudioResample {
    int64_t pts = 0;

    AVPacket inPacket;
    AVPacket outPacket;
    AVFrame  *inFrame;
    AVFrame  *outFrame;

    SwrContext *resampleCtx;
    AVAudioFifo *fifo = nullptr;

    AVFormatContext *inFormatCtx;
    AVCodecContext  *inCodecCtx;
    AVFormatContext *outFormatCtx;
    AVCodecContext  *outCodecCtx;
};

class FFAudioResample {
private:

    AudioResample *resample;

    int openInputFile(const char *filename);

    int openOutputFile(const char *filename, int sample_rate);

    int decodeAudioFrame(AVFrame *frame, int *data_present, int *finished);

    int initConvertedSamples(uint8_t ***converted_input_samples, int frame_size);

    int decodeAndConvert(int *finished);

    int encodeAudioFrame(AVFrame *frame, int *data_present);

    int encodeAndWrite();
public:

    FFAudioResample();

    ~FFAudioResample();

    int resampling(const char *src_file, const char *dst_file, int sampleRate);

};
#endif //FFMPEGANDROID_FF_AUDIO_RESAMPLE_H
