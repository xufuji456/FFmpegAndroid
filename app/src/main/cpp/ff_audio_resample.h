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

class FFAudioResample {
private:
    int64_t pts = 0;

    AVPacket input_packet;

    AVPacket output_packet;

    AVFrame *input_frame;

    AVFrame *output_frame;

    int openInputFile(const char *filename,
                    AVFormatContext **input_format_context,
                    AVCodecContext **input_codec_context);

    int openOutputFile(const char *filename,
                         int sample_rate,
                         AVCodecContext *input_codec_context,
                         AVFormatContext **output_format_context,
                         AVCodecContext **output_codec_context);

    int initResample(AVCodecContext *input_codec_context,
                      AVCodecContext *output_codec_context,
                      SwrContext **resample_context);

    int decodeAudioFrame(AVFrame *frame,
                           AVFormatContext *input_format_context,
                           AVCodecContext *input_codec_context,
                           int *data_present, int *finished);

    int initConvertedSamples(uint8_t ***converted_input_samples,
                               AVCodecContext *output_codec_context, int frame_size);

    int decodeAndConvert(AVAudioFifo *fifo,
                                      AVFormatContext *input_format_context,
                                      AVCodecContext *input_codec_context,
                                      AVCodecContext *output_codec_context,
                                      SwrContext *resample_context,
                                      int *finished);

    int encodeAudioFrame(AVFrame *frame,
                           AVFormatContext *output_format_context,
                           AVCodecContext *output_codec_context,
                           int *data_present);

    int encodeAndWrite(AVAudioFifo *fifo,
                              AVFormatContext *output_format_context,
                              AVCodecContext *output_codec_context);
public:

    int resampling(const char *src_file, const char *dst_file, int sampleRate);

};
#endif //FFMPEGANDROID_FF_AUDIO_RESAMPLE_H
