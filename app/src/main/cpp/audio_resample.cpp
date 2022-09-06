//
// Created by xu fulong on 2022/7/12.
//

#include <jni.h>

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

#include "ffmpeg_jni_define.h"

#define ALOGE(Format, ...) LOGE("audio_resample", Format, ##__VA_ARGS__)

/* The output bit rate in bit/s */
#define OUTPUT_BIT_RATE 96000
/* The number of output channels */
#define OUTPUT_CHANNELS 2

/* Global timestamp for the audio frames. */
static int64_t pts = 0;

static AVPacket input_packet;

static AVPacket output_packet;

static AVFrame *output_frame;

/**
 * Open an input file and the required decoder.
 *
 */
static int open_input_file(const char *filename,
                           AVFormatContext **input_format_context,
                           AVCodecContext **input_codec_context) {
    int error;
    const AVCodec *input_codec;
    AVCodecContext *avctx = nullptr;
    AVStream *audio_stream = nullptr;

    if ((error = avformat_open_input(input_format_context, filename, nullptr,
                                     nullptr)) < 0) {
        ALOGE("Could not open input file:%s (error:%s)\n", filename, av_err2str(error));
        *input_format_context = nullptr;
        return error;
    }

    if ((error = avformat_find_stream_info(*input_format_context, nullptr)) < 0) {
        ALOGE("Could not open find stream info (error:%s)\n", av_err2str(error));
        goto cleanup;
    }

    for (int i = 0; i < (*input_format_context)->nb_streams; ++i) {
        if ((*input_format_context)->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
            audio_stream = (*input_format_context)->streams[i];
        }
    }
    if (!(input_codec = avcodec_find_decoder(audio_stream->codecpar->codec_id))) {
        ALOGE("Could not find input codec\n");
        goto cleanup;
    }

    avctx = avcodec_alloc_context3(input_codec);
    if (!avctx) {
        ALOGE("Could not allocate a decoding context\n");
        goto cleanup;
    }

    error = avcodec_parameters_to_context(avctx, audio_stream->codecpar);
    if (error < 0) {
        goto cleanup;
    }

    if ((error = avcodec_open2(avctx, input_codec, nullptr)) < 0) {
        ALOGE("Could not open input codec (error:%s)\n", av_err2str(error));
        goto cleanup;
    }
    *input_codec_context = avctx;

    return 0;

cleanup:
    if (avctx)
        avcodec_free_context(&avctx);
    avformat_close_input(input_format_context);
    return -1;
}

/**
 * Open an output file and the required encoder.
 *
 */
static int open_output_file(const char *filename,
                            int sample_rate,
                            AVCodecContext *input_codec_context,
                            AVFormatContext **output_format_context,
                            AVCodecContext **output_codec_context) {
    AVCodecContext *avctx          = nullptr;
    AVIOContext *output_io_context = nullptr;
    AVStream *stream;
    const AVCodec *output_codec;
    int error;

    /* Open the output file to write to it. */
    if ((error = avio_open(&output_io_context, filename,
                           AVIO_FLAG_WRITE)) < 0) {
        ALOGE("Could not open output file:%s (error:%s)\n", filename, av_err2str(error));
        return error;
    }

    /* Create a new format context for the output container format. */
    if (!(*output_format_context = avformat_alloc_context())) {
        ALOGE("Could not allocate output format context\n");
        return AVERROR(ENOMEM);
    }

    /* Associate the output file (pointer) with the container format context. */
    (*output_format_context)->pb = output_io_context;

    /* Guess the desired container format based on the file extension. */
    if (!((*output_format_context)->oformat = av_guess_format(nullptr, filename,
                                                              nullptr))) {
        ALOGE("Could not find output file format\n");
        goto cleanup;
    }

    if (!((*output_format_context)->url = av_strdup(filename))) {
        ALOGE("Could not allocate url.\n");
        error = AVERROR(ENOMEM);
        goto cleanup;
    }

    /* Find the encoder to be used by its name. */
    if (!(output_codec = avcodec_find_encoder(input_codec_context->codec_id))) {
        ALOGE( "Could not find encoder=%s\n", input_codec_context->codec->name);
        goto cleanup;
    }

    /* Create a new audio stream in the output file container. */
    if (!(stream = avformat_new_stream(*output_format_context, nullptr))) {
        ALOGE("Could not create new stream\n");
        error = AVERROR(ENOMEM);
        goto cleanup;
    }

    avctx = avcodec_alloc_context3(output_codec);
    if (!avctx) {
        ALOGE("Could not allocate an encoding context\n");
        error = AVERROR(ENOMEM);
        goto cleanup;
    }

    /* Set the basic encoder parameters.*/
    avctx->channels       = OUTPUT_CHANNELS;
    avctx->channel_layout = av_get_default_channel_layout(OUTPUT_CHANNELS);
    avctx->sample_rate    = sample_rate;
    avctx->sample_fmt     = output_codec->sample_fmts[0];
    avctx->bit_rate       = OUTPUT_BIT_RATE;

    /* Allow the use of the experimental AAC encoder. */
    avctx->strict_std_compliance = FF_COMPLIANCE_EXPERIMENTAL;

    /* Set the sample rate for the container. */
    stream->time_base.den = sample_rate;
    stream->time_base.num = 1;

    /* Some container formats (like MP4) require global headers to be present.
     * Mark the encoder so that it behaves accordingly. */
    if ((*output_format_context)->oformat->flags & AVFMT_GLOBALHEADER)
        avctx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

    /* Open the encoder for the audio stream to use it later. */
    if ((error = avcodec_open2(avctx, output_codec, nullptr)) < 0) {
        ALOGE("Could not open output codec (error:%s)\n", av_err2str(error));
        goto cleanup;
    }

    error = avcodec_parameters_from_context(stream->codecpar, avctx);
    if (error < 0) {
        ALOGE("Could not initialize stream parameters\n");
        goto cleanup;
    }

    *output_codec_context = avctx;
    return 0;

cleanup:
    avcodec_free_context(&avctx);
    avio_closep(&(*output_format_context)->pb);
    avformat_free_context(*output_format_context);
    *output_format_context = nullptr;
    return error < 0 ? error : AVERROR_EXIT;
}

/**
 * Initialize one audio frame for reading from the input file.
 *
 */
static int init_input_frame(AVFrame **frame) {
    if (!(*frame = av_frame_alloc())) {
        ALOGE("Could not allocate input frame\n");
        return AVERROR(ENOMEM);
    }
    return 0;
}

/**
 * Initialize the audio resampler based on the input and output codec settings.
 *
 */
static int init_resampler(AVCodecContext *input_codec_context,
                          AVCodecContext *output_codec_context,
                          SwrContext **resample_context) {
    int error;
    *resample_context = swr_alloc_set_opts(nullptr,
                                           av_get_default_channel_layout(output_codec_context->channels),
                                           output_codec_context->sample_fmt,
                                           output_codec_context->sample_rate,
                                           av_get_default_channel_layout(input_codec_context->channels),
                                           input_codec_context->sample_fmt,
                                           input_codec_context->sample_rate,
                                           0, nullptr);
    if (!*resample_context) {
        ALOGE("Could not allocate resample context\n");
        return AVERROR(ENOMEM);
    }

    /* Open the re-sampler with the specified parameters. */
    if ((error = swr_init(*resample_context)) < 0) {
        ALOGE("Could not open resample context\n");
        swr_free(resample_context);
        return error;
    }
    return 0;
}

/**
 * Initialize a FIFO buffer for the audio samples to be encoded.
 *
 */
static int init_fifo(AVAudioFifo **fifo, AVCodecContext *output_codec_context) {
    if (!(*fifo = av_audio_fifo_alloc(output_codec_context->sample_fmt,
                                      output_codec_context->channels, 1))) {
        ALOGE("Could not allocate FIFO\n");
        return AVERROR(ENOMEM);
    }
    return 0;
}

/**
 * Write the header of the output file container.
 *
 */
static int write_output_file_header(AVFormatContext *output_format_context) {
    int error;
    if ((error = avformat_write_header(output_format_context, nullptr)) < 0) {
        ALOGE("Could not write output file header (error:%s)\n", av_err2str(error));
        return error;
    }
    return 0;
}

/**
 * Decode one audio frame from the input file.
 *
 */
static int decode_audio_frame(AVFrame *frame,
                              AVFormatContext *input_format_context,
                              AVCodecContext *input_codec_context,
                              int *data_present, int *finished) {
    int error;

    if ((error = av_read_frame(input_format_context, &input_packet)) < 0) {
        if (error == AVERROR_EOF)
            *finished = 1;
        else {
            ALOGE("Could not read frame (error:%s)\n", av_err2str(error));
            return error;
        }
    }

    if (input_format_context->streams[input_packet.stream_index]->codecpar->codec_type
        != AVMEDIA_TYPE_AUDIO) {
        error = 0;
        ALOGE("isn't audio packet, skip it...");
        goto cleanup;
    }

    /* Send the audio frame stored in the temporary packet to the decoder.*/
    if ((error = avcodec_send_packet(input_codec_context, &input_packet)) < 0) {
        ALOGE("Could not send packet for decoding (error:%s)\n", av_err2str(error));
        return error;
    }

    /* Receive one frame from the decoder. */
    error = avcodec_receive_frame(input_codec_context, frame);
    if (error == AVERROR(EAGAIN)) {
        error = 0;
        goto cleanup;
    } else if (error == AVERROR_EOF) {
        *finished = 1;
        error = 0;
        goto cleanup;
    } else if (error < 0) {
        ALOGE("Could not decode frame (error:%s)\n", av_err2str(error));
        goto cleanup;
    } else {
        *data_present = 1;
        goto cleanup;
    }

cleanup:
    av_packet_unref(&input_packet);
    return error;
}

/**
 * Initialize a temporary storage for the specified number of audio samples.
 *
 */
static int init_converted_samples(uint8_t ***converted_input_samples,
                                  AVCodecContext *output_codec_context,
                                  int frame_size) {
    int error;
    if (!(*converted_input_samples = (uint8_t **) calloc(output_codec_context->channels,
                                            sizeof(**converted_input_samples)))) {
        ALOGE("Could not allocate converted input sample pointers\n");
        return AVERROR(ENOMEM);
    }

    if ((error = av_samples_alloc(*converted_input_samples, nullptr,
                                  output_codec_context->channels,
                                  frame_size,
                                  output_codec_context->sample_fmt, 0)) < 0) {
        ALOGE("Could not allocate converted input samples (error:%s)\n", av_err2str(error));
        av_freep(&(*converted_input_samples)[0]);
        free(*converted_input_samples);
        return error;
    }
    return 0;
}

/**
 * Convert the input audio samples into the output sample format.
 * The size of which is specified by frame_size.
 *
 */
static int convert_samples(const uint8_t **input_data, const int input_size,
                           uint8_t **converted_data, const int output_size,
                           SwrContext *resample_context) {
    int error;
    if ((error = swr_convert(resample_context, converted_data, output_size,
                             input_data, input_size)) < 0) {
        ALOGE("Could not convert input samples (error:%s)\n", av_err2str(error));
        return error;
    }

    return 0;
}

/**
 * Add converted input audio samples to the FIFO buffer for later processing.
 *
 */
static int add_samples_to_fifo(AVAudioFifo *fifo,
                               uint8_t **converted_input_samples,
                               const int frame_size) {
    int error;
    /* Make the FIFO as large as it needs to be to hold both, the old and the new samples. */
    if ((error = av_audio_fifo_realloc(fifo, av_audio_fifo_size(fifo) + frame_size)) < 0) {
        ALOGE("Could not reallocate FIFO\n");
        return error;
    }

    /* Store the new samples in the FIFO buffer. */
    if (av_audio_fifo_write(fifo, (void **)converted_input_samples,
                            frame_size) < frame_size) {
        ALOGE("Could not write data to FIFO\n");
        return AVERROR_EXIT;
    }
    return 0;
}

/**
 * Read one audio frame from the input file, decode, convert and store
 * it in the FIFO buffer.
 *
 */
static int read_decode_convert_and_store(AVAudioFifo *fifo,
                                         AVFormatContext *input_format_context,
                                         AVCodecContext *input_codec_context,
                                         AVCodecContext *output_codec_context,
                                         SwrContext *resampler_context,
                                         int *finished) {
    AVFrame *input_frame = nullptr;
    uint8_t **converted_dst_samples = nullptr;
    int data_present = 0;
    int ret = AVERROR_EXIT;

    /* Initialize temporary storage for one input frame. */
    if (init_input_frame(&input_frame))
        goto cleanup;
    /* Decode one frame worth of audio samples. */
    if (decode_audio_frame(input_frame, input_format_context,
                           input_codec_context, &data_present, finished))
        goto cleanup;
    if (*finished) {
        ret = 0;
        goto cleanup;
    }
    /* If there is decoded data, convert and store it. */
    if (data_present) {
        int dst_nb_samples = (int) av_rescale_rnd(input_frame->nb_samples, output_codec_context->sample_rate,
                                            input_codec_context->sample_rate, AV_ROUND_UP);

        if (init_converted_samples(&converted_dst_samples, output_codec_context,
                                   dst_nb_samples))
            goto cleanup;

        if (convert_samples((const uint8_t**)input_frame->extended_data,
                            input_frame->nb_samples, converted_dst_samples,
                            dst_nb_samples, resampler_context))
            goto cleanup;

        if (add_samples_to_fifo(fifo, converted_dst_samples,
                                dst_nb_samples))
            goto cleanup;
    }
    ret = 0;

cleanup:
    if (converted_dst_samples) {
        av_freep(&converted_dst_samples[0]);
        free(converted_dst_samples);
    }
    av_frame_free(&input_frame);

    return ret;
}

static int init_output_frame(AVFrame **frame, AVCodecContext *output_codec_context) {
    *frame = av_frame_alloc();
    (*frame)->nb_samples     = output_codec_context->frame_size;
    (*frame)->channel_layout = output_codec_context->channel_layout;
    (*frame)->format         = output_codec_context->sample_fmt;
    (*frame)->sample_rate    = output_codec_context->sample_rate;

    int ret = av_frame_get_buffer(*frame, 0);
    if (ret < 0) {
        ALOGE("Could not allocate output frame samples (error:%s)\n", av_err2str(ret));
    }
    return ret;
}

/**
 * Encode one frame worth of audio to the output file.
 *
 */
static int encode_audio_frame(AVFrame *frame,
                              AVFormatContext *output_format_context,
                              AVCodecContext *output_codec_context,
                              int *data_present) {
    int error;

    /* Set a timestamp based on the sample rate for the container. */
    if (frame) {
        frame->pts = pts;
        pts += frame->nb_samples;
    }

    /* Send frame stored in the temporary packet to the encoder.*/
    error = avcodec_send_frame(output_codec_context, frame);
    if (error == AVERROR_EOF) {
        error = 0;
        goto cleanup;
    } else if (error < 0) {
        ALOGE("Could not send packet for encoding (error:%s)\n", av_err2str(error));
        return error;
    }

    /* Receive one encoded frame from the encoder. */
    error = avcodec_receive_packet(output_codec_context, &output_packet);
    if (error == AVERROR(EAGAIN)) {
        error = 0;
        goto cleanup;
    } else if (error == AVERROR_EOF) {
        error = 0;
        goto cleanup;
    } else if (error < 0) {
        ALOGE("Could not encode frame (error:%s)\n", av_err2str(error));
        goto cleanup;
    } else {
        *data_present = 1;
    }

    /* Write one audio frame from the temporary packet to the output file. */
    if (*data_present &&
        (error = av_write_frame(output_format_context, &output_packet)) < 0) {
        ALOGE("Could not write frame (error:%s)\n", av_err2str(error));
        goto cleanup;
    }

cleanup:
    av_packet_unref(&output_packet);
    return error;
}

/**
 * Load one audio frame from the FIFO buffer, encode and write it to the
 * output file.
 *
 */
static int load_encode_and_write(AVAudioFifo *fifo,
                                 AVFormatContext *output_format_context,
                                 AVCodecContext *output_codec_context) {
    int data_written;
    const int frame_size = FFMIN(av_audio_fifo_size(fifo),
                                 output_codec_context->frame_size);

    output_frame->nb_samples = frame_size;
    if (av_audio_fifo_read(fifo, (void **)output_frame->data, frame_size) < frame_size) {
        ALOGE("Could not read data from FIFO\n");
        return AVERROR_EXIT;
    }

    if (encode_audio_frame(output_frame, output_format_context,
                           output_codec_context, &data_written)) {
        return AVERROR_EXIT;
    }
    return 0;
}

/**
 * Write the trailer of the output file container.
 *
 */
static int write_output_file_trailer(AVFormatContext *output_format_context) {
    int error;
    if ((error = av_write_trailer(output_format_context)) < 0) {
        ALOGE("Could not write output file trailer (error:%s)\n", av_err2str(error));
        return error;
    }
    return 0;
}

int resampling(const char *src_file, const char *dst_file, int sampleRate) {
    int ret = AVERROR_EXIT;
    AVAudioFifo *fifo = nullptr;
    SwrContext *resample_context = nullptr;
    AVCodecContext *input_codec_context = nullptr;
    AVCodecContext *output_codec_context = nullptr;
    AVFormatContext *input_format_context = nullptr;
    AVFormatContext *output_format_context = nullptr;

    /* Open the input file for reading. */
    if (open_input_file(src_file, &input_format_context, &input_codec_context))
        goto cleanup;
    /* Open the output file for writing. */
    if (open_output_file(dst_file, sampleRate, input_codec_context, &output_format_context, &output_codec_context))
        goto cleanup;
    /* Initialize the re-sampler to be able to convert audio sample formats. */
    if (init_resampler(input_codec_context, output_codec_context,
                       &resample_context))
        goto cleanup;
    /* Initialize the FIFO buffer to store audio samples to be encoded. */
    if (init_fifo(&fifo, output_codec_context))
        goto cleanup;
    if (init_output_frame(&output_frame, output_codec_context))
        goto cleanup;
    /* Write the header of the output file container. */
    if (write_output_file_header(output_format_context))
        goto cleanup;

    while (1) {
        const int output_frame_size = output_codec_context->frame_size;
        int finished                = 0;

        while (av_audio_fifo_size(fifo) < output_frame_size) {
            /* Decode one frame, convert sample format and put it into the FIFO buffer. */
            if (read_decode_convert_and_store(fifo, input_format_context,
                                              input_codec_context,
                                              output_codec_context,
                                              resample_context, &finished))
                goto cleanup;

            if (finished)
                break;
        }

        /* If we have enough samples for the encoder, we encode them.*/
        while (av_audio_fifo_size(fifo) >= output_frame_size ||
               (finished && av_audio_fifo_size(fifo) > 0))
            if (load_encode_and_write(fifo, output_format_context, output_codec_context))
                goto cleanup;

        /* encode all the remaining samples. */
        if (finished) {
            int data_written;
            do {
                data_written = 0;
                if (encode_audio_frame(nullptr, output_format_context,
                                       output_codec_context, &data_written))
                    goto cleanup;
            } while (data_written);
            break;
        }
    }

    /* Write the trailer of the output file container. */
    if (write_output_file_trailer(output_format_context))
        goto cleanup;
    ret = 0;

cleanup:
    if (fifo)
        av_audio_fifo_free(fifo);
    swr_free(&resample_context);
    if (output_codec_context)
        avcodec_free_context(&output_codec_context);
    if (output_format_context) {
        avio_closep(&output_format_context->pb);
        avformat_free_context(output_format_context);
    }
    if (input_codec_context)
        avcodec_free_context(&input_codec_context);
    if (input_format_context)
        avformat_close_input(&input_format_context);
    if (output_frame)
        av_frame_free(&output_frame);

    return ret;
}

extern "C"
VIDEO_PLAYER_FUNC(int, audioResample, jstring srcFile, jstring dstFile, int sampleRate) {
    const char *src_file = env->GetStringUTFChars(srcFile, JNI_FALSE);
    const char *dst_file = env->GetStringUTFChars(dstFile, JNI_FALSE);
    int ret = resampling(src_file, dst_file, sampleRate);
    env->ReleaseStringUTFChars(dstFile, dst_file);
    env->ReleaseStringUTFChars(srcFile, src_file);
    return ret;
}