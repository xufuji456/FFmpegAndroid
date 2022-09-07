//
// Created by xu fulong on 2022/7/12.
//

#include "ff_audio_resample.h"

#define ALOGE(Format, ...) LOGE("audio_resample", Format, ##__VA_ARGS__)


int FFAudioResample::openInputFile(const char *filename,
                           AVFormatContext **input_format_context,
                           AVCodecContext **input_codec_context) {
    int ret;
    const AVCodec *input_codec;
    AVStream *audio_stream = nullptr;

    if ((ret = avformat_open_input(input_format_context, filename, nullptr,nullptr)) < 0) {
        ALOGE("Could not open input file:%s\n", av_err2str(ret));
        return ret;
    }
    avformat_find_stream_info(*input_format_context, nullptr);

    for (int i = 0; i < (*input_format_context)->nb_streams; ++i) {
        if ((*input_format_context)->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
            audio_stream = (*input_format_context)->streams[i];
        }
    }
    if (!(input_codec = avcodec_find_decoder(audio_stream->codecpar->codec_id))) {
        ALOGE("Could not find input codec:%s\n", avcodec_get_name(audio_stream->codecpar->codec_id));
        return -1;
    }

    *input_codec_context = avcodec_alloc_context3(input_codec);
    avcodec_parameters_to_context(*input_codec_context, audio_stream->codecpar);

    if ((ret = avcodec_open2(*input_codec_context, input_codec, nullptr)) < 0) {
        ALOGE("Could not open input codec (error:%s)\n", av_err2str(ret));
    }

    return 0;
}

int FFAudioResample::openOutputFile(const char *filename,
                            int sample_rate,
                            AVCodecContext *input_codec_context,
                            AVFormatContext **output_format_context,
                            AVCodecContext **output_codec_context) {
    AVCodecContext *avctx;
    AVIOContext *output_io_context = nullptr;
    const AVCodec *output_codec;
    int ret;

    if ((ret = avio_open(&output_io_context, filename, AVIO_FLAG_WRITE)) < 0) {
        ALOGE("Could not open output file:%s\n", av_err2str(ret));
        return ret;
    }

    *output_format_context            = avformat_alloc_context();
    (*output_format_context)->pb      = output_io_context;
    (*output_format_context)->url     = av_strdup(filename);
    (*output_format_context)->oformat = av_guess_format(nullptr, filename,nullptr);
    if (!(*output_format_context)->oformat) {
        ALOGE("Could not find output file format\n");
        return -1;
    }

    /* Find the encoder to be used by its name. */
    if (!(output_codec = avcodec_find_encoder(input_codec_context->codec_id))) {
        ALOGE( "Could not find encoder=%s\n", input_codec_context->codec->name);
        return -1;
    }

    /* Create a new audio stream in the output file container. */
    AVStream *stream = avformat_new_stream(*output_format_context, nullptr);

    avctx = avcodec_alloc_context3(output_codec);

    /* Set the basic encoder parameters.*/
    avctx->channels       = input_codec_context->channels;
    avctx->channel_layout = av_get_default_channel_layout(input_codec_context->channels);
    avctx->sample_rate    = sample_rate;
    avctx->sample_fmt     = output_codec->sample_fmts[0];

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
    if ((ret = avcodec_open2(avctx, output_codec, nullptr)) < 0) {
        ALOGE("Could not open output codec (error:%s)\n", av_err2str(ret));
        return ret;
    }

    avcodec_parameters_from_context(stream->codecpar, avctx);
    *output_codec_context = avctx;
    return 0;
}

int FFAudioResample::initResample(AVCodecContext *input_codec_context,
                          AVCodecContext *output_codec_context,
                          SwrContext **resample_context) {
    *resample_context = swr_alloc_set_opts(nullptr,
                                           av_get_default_channel_layout(output_codec_context->channels),
                                           output_codec_context->sample_fmt,
                                           output_codec_context->sample_rate,
                                           av_get_default_channel_layout(input_codec_context->channels),
                                           input_codec_context->sample_fmt,
                                           input_codec_context->sample_rate,
                                           0, nullptr);
    return swr_init(*resample_context);
}

int FFAudioResample::decodeAudioFrame(AVFrame *frame,
                              AVFormatContext *input_format_context,
                              AVCodecContext *input_codec_context,
                              int *data_present, int *finished) {
    int ret;

    if ((ret = av_read_frame(input_format_context, &input_packet)) < 0) {
        if (ret == AVERROR_EOF)
            *finished = 1;
        else {
            ALOGE("Could not read frame (error:%s)\n", av_err2str(ret));
            return ret;
        }
    }
    if (input_format_context->streams[input_packet.stream_index]->codecpar->codec_type != AVMEDIA_TYPE_AUDIO) {
        ret = 0;
        ALOGE("isn't audio packet, skip it...");
        goto cleanup;
    }
    /* Send the audio frame stored in the temporary packet to the decoder.*/
    if ((ret = avcodec_send_packet(input_codec_context, &input_packet)) < 0) {
        ALOGE("Could not send packet for decoding (error:%s)\n", av_err2str(ret));
        return ret;
    }
    /* Receive one frame from the decoder. */
    ret = avcodec_receive_frame(input_codec_context, frame);
    if (ret == AVERROR(EAGAIN)) {
        ret = 0;
        goto cleanup;
    } else if (ret == AVERROR_EOF) {
        *finished = 1;
        ret = 0;
        goto cleanup;
    } else if (ret < 0) {
        ALOGE("Could not decode frame (error:%s)\n", av_err2str(ret));
        goto cleanup;
    } else {
        *data_present = 1;
        goto cleanup;
    }

cleanup:
    av_packet_unref(&input_packet);
    return ret;
}

int FFAudioResample::initConvertedSamples(uint8_t ***converted_input_samples,
                                  AVCodecContext *output_codec_context, int frame_size) {
    int ret;
    if (!(*converted_input_samples = (uint8_t **) calloc(output_codec_context->channels,
                                            sizeof(**converted_input_samples)))) {
        ALOGE("Could not allocate converted input sample pointers\n");
        return AVERROR(ENOMEM);
    }

    if ((ret = av_samples_alloc(*converted_input_samples, nullptr,
                                  output_codec_context->channels,
                                  frame_size,
                                  output_codec_context->sample_fmt, 0)) < 0) {
        ALOGE("Could not allocate converted input samples (error:%s)\n", av_err2str(ret));
        av_freep(&(*converted_input_samples)[0]);
        free(*converted_input_samples);
        return ret;
    }
    return 0;
}

/**
 * Read one audio frame from the input file, decode, convert and store
 * it in the FIFO buffer.
 *
 */
int FFAudioResample::decodeAndConvert(AVAudioFifo *fifo,
                                         AVFormatContext *input_format_context,
                                         AVCodecContext *input_codec_context,
                                         AVCodecContext *output_codec_context,
                                         SwrContext *resample_context,
                                         int *finished) {
    uint8_t **converted_dst_samples = nullptr;
    int data_present = 0;
    int ret = AVERROR_EXIT;

    /* Decode one frame worth of audio samples. */
    if (decodeAudioFrame(input_frame, input_format_context,
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

        if (initConvertedSamples(&converted_dst_samples, output_codec_context, dst_nb_samples))
            goto cleanup;

        ret = swr_convert(resample_context, converted_dst_samples, dst_nb_samples,
                          (const uint8_t**)input_frame->extended_data, input_frame->nb_samples);
        if (ret < 0) {
            ALOGE("Could not convert input samples (error:%s)\n", av_err2str(ret));
            goto cleanup;
        }

        if (av_audio_fifo_write(fifo, (void **)converted_dst_samples, dst_nb_samples) < dst_nb_samples)
            goto cleanup;
    }
    ret = 0;

cleanup:
    if (converted_dst_samples) {
        av_freep(&converted_dst_samples[0]);
        free(converted_dst_samples);
    }

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

int FFAudioResample::encodeAudioFrame(AVFrame *frame,
                              AVFormatContext *output_format_context,
                              AVCodecContext *output_codec_context,
                              int *data_present) {
    int ret;

    /* Set a timestamp based on the sample rate for the container. */
    if (frame) {
        frame->pts = pts;
        pts += frame->nb_samples;
    }

    ret = avcodec_send_frame(output_codec_context, frame);
    if (ret == AVERROR_EOF) {
        ret = 0;
        goto cleanup;
    } else if (ret < 0) {
        ALOGE("Could not send packet for encoding (error:%s)\n", av_err2str(ret));
        return ret;
    }

    ret = avcodec_receive_packet(output_codec_context, &output_packet);
    if (ret == AVERROR(EAGAIN)) {
        ret = 0;
        goto cleanup;
    } else if (ret == AVERROR_EOF) {
        ret = 0;
        goto cleanup;
    } else if (ret < 0) {
        ALOGE("Could not encode frame (error:%s)\n", av_err2str(ret));
        goto cleanup;
    } else {
        *data_present = 1;
    }

    /* Write one audio frame from the temporary packet to the output file. */
    if (*data_present &&
        (ret = av_write_frame(output_format_context, &output_packet)) < 0) {
        ALOGE("Could not write frame (error:%s)\n", av_err2str(ret));
    }

cleanup:
    av_packet_unref(&output_packet);
    return ret;
}

/**
 * Load one audio frame from the FIFO buffer, encode and write it to the
 * output file.
 *
 */
int FFAudioResample::encodeAndWrite(AVAudioFifo *fifo,
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

    if (encodeAudioFrame(output_frame, output_format_context,
                           output_codec_context, &data_written)) {
        return AVERROR_EXIT;
    }
    return 0;
}

int FFAudioResample::resampling(const char *src_file, const char *dst_file, int sampleRate) {
    int ret = AVERROR_EXIT;
    AVAudioFifo *fifo = nullptr;
    SwrContext *resample_context = nullptr;
    AVCodecContext *input_codec_context = nullptr;
    AVCodecContext *output_codec_context = nullptr;
    AVFormatContext *input_format_context = nullptr;
    AVFormatContext *output_format_context = nullptr;

    /* Open the input file for reading. */
    if (openInputFile(src_file, &input_format_context, &input_codec_context))
        goto cleanup;
    /* Open the output file for writing. */
    if (openOutputFile(dst_file, sampleRate, input_codec_context, &output_format_context, &output_codec_context))
        goto cleanup;
    /* Initialize the re-sampler to be able to convert audio sample formats. */
    if (initResample(input_codec_context, output_codec_context,
                       &resample_context))
        goto cleanup;
    /* Initialize the FIFO buffer to store audio samples to be encoded. */
    fifo = av_audio_fifo_alloc(output_codec_context->sample_fmt, output_codec_context->channels, 1024 * 10);
    input_frame = av_frame_alloc();
    if (init_output_frame(&output_frame, output_codec_context))
        goto cleanup;
    /* Write the header of the output file container. */
    if ((ret = avformat_write_header(output_format_context, nullptr)) < 0) {
        ALOGE("write header error=%s", av_err2str(ret));
    }

    while (1) {
        const int output_frame_size = output_codec_context->frame_size;
        int finished                = 0;

        while (av_audio_fifo_size(fifo) < output_frame_size) {
            /* Decode one frame, convert sample format and put it into the FIFO buffer. */
            if (decodeAndConvert(fifo, input_format_context,
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
            if (encodeAndWrite(fifo, output_format_context, output_codec_context))
                goto cleanup;

        /* encode all the remaining samples. */
        if (finished) {
            int data_written;
            do {
                data_written = 0;
                if (encodeAudioFrame(nullptr, output_format_context,
                                       output_codec_context, &data_written))
                    goto cleanup;
            } while (data_written);
            break;
        }
    }

    /* Write the trailer of the output file container. */
    if (av_write_trailer(output_format_context)) {
        ALOGE("write trailer error...");
    }
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
    if (input_frame)
        av_frame_free(&input_frame);

    return ret;
}