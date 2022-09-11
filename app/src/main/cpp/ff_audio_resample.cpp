//
// Created by xu fulong on 2022/7/12.
//

#include "ff_audio_resample.h"

#define ALOGE(Format, ...) LOGE("audio_resample", Format, ##__VA_ARGS__)


FFAudioResample::FFAudioResample() {
    resample = new AudioResample();
}

FFAudioResample::~FFAudioResample() {
    delete resample;
}

int FFAudioResample::openInputFile(const char *filename) {
    int ret;
    const AVCodec *input_codec;
    AVStream *audio_stream = nullptr;

    if ((ret = avformat_open_input(&resample->inFormatCtx, filename, nullptr,nullptr)) < 0) {
        ALOGE("Could not open input file:%s\n", av_err2str(ret));
        return ret;
    }
    avformat_find_stream_info(resample->inFormatCtx, nullptr);

    for (int i = 0; i < resample->inFormatCtx->nb_streams; ++i) {
        if (resample->inFormatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
            audio_stream = resample->inFormatCtx->streams[i];
        }
    }
    if (!(input_codec = avcodec_find_decoder(audio_stream->codecpar->codec_id))) {
        ALOGE("Could not find input codec:%s\n", avcodec_get_name(audio_stream->codecpar->codec_id));
        return -1;
    }

    resample->inCodecCtx = avcodec_alloc_context3(input_codec);
    avcodec_parameters_to_context(resample->inCodecCtx, audio_stream->codecpar);

    if ((ret = avcodec_open2(resample->inCodecCtx, input_codec, nullptr)) < 0) {
        ALOGE("Could not open input codec (error:%s)\n", av_err2str(ret));
    }
    resample->inFrame = av_frame_alloc();

    return 0;
}

int FFAudioResample::openOutputFile(const char *filename, int sample_rate) {
    AVIOContext *output_io_context = nullptr;
    const AVCodec *output_codec;
    int ret;

    if ((ret = avio_open(&output_io_context, filename, AVIO_FLAG_WRITE)) < 0) {
        ALOGE("Could not open output file:%s\n", av_err2str(ret));
        return ret;
    }

    resample->outFormatCtx          = avformat_alloc_context();
    resample->outFormatCtx->pb      = output_io_context;
    resample->outFormatCtx->url     = av_strdup(filename);
    resample->outFormatCtx->oformat = av_guess_format(nullptr, filename,nullptr);
    if (!(resample->outFormatCtx->oformat)) {
        ALOGE("Could not find output file format\n");
        return -1;
    }

    /* Find the encoder to be used by its name. */
    if (!(output_codec = avcodec_find_encoder(resample->inCodecCtx->codec_id))) {
        ALOGE( "Could not find encoder=%s\n", resample->inCodecCtx->codec->name);
        return -1;
    }

    /* Create a new audio stream in the output file container. */
    AVStream *stream = avformat_new_stream(resample->outFormatCtx, nullptr);

    resample->outCodecCtx = avcodec_alloc_context3(output_codec);

    /* Set the basic encoder parameters.*/
    resample->outCodecCtx->channels       = resample->inCodecCtx->channels;
    resample->outCodecCtx->channel_layout = av_get_default_channel_layout(resample->inCodecCtx->channels);
    resample->outCodecCtx->sample_rate    = sample_rate;
    resample->outCodecCtx->sample_fmt     = output_codec->sample_fmts[0];

    /* Allow the use of the experimental AAC encoder. */
    resample->outCodecCtx->strict_std_compliance = FF_COMPLIANCE_EXPERIMENTAL;

    /* Set the sample rate for the container. */
    stream->time_base.den = sample_rate;
    stream->time_base.num = 1;

    /* Some container formats (like MP4) require global headers to be present.
     * Mark the encoder so that it behaves accordingly. */
    if (resample->outFormatCtx->oformat->flags & AVFMT_GLOBALHEADER)
        resample->outCodecCtx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

    /* Open the encoder for the audio stream to use it later. */
    if ((ret = avcodec_open2(resample->outCodecCtx, output_codec, nullptr)) < 0) {
        ALOGE("Could not open output codec (error:%s)\n", av_err2str(ret));
        return ret;
    }

    avcodec_parameters_from_context(stream->codecpar, resample->outCodecCtx);
    return 0;
}

static int initResample(AudioResample **pResample) {
    AudioResample *ar = *pResample;
    SwrContext *context = swr_alloc_set_opts(nullptr,
                                           av_get_default_channel_layout(ar->outCodecCtx->channels),
                                           ar->outCodecCtx->sample_fmt,
                                           ar->outCodecCtx->sample_rate,
                                           av_get_default_channel_layout(ar->inCodecCtx->channels),
                                           ar->inCodecCtx->sample_fmt,
                                           ar->inCodecCtx->sample_rate,
                                           0, nullptr);
    int ret = swr_init(context);
    ar->resampleCtx = context;
    *pResample = ar;
    return ret;
}

int FFAudioResample::decodeAudioFrame(AVFrame *frame, int *data_present, int *finished) {
    int ret;

    if ((ret = av_read_frame(resample->inFormatCtx, &resample->inPacket)) < 0) {
        if (ret == AVERROR_EOF)
            *finished = 1;
        else {
            ALOGE("Could not read frame (error:%s)\n", av_err2str(ret));
            return ret;
        }
    }
    if (resample->inFormatCtx->streams[resample->inPacket.stream_index]->codecpar->codec_type
        != AVMEDIA_TYPE_AUDIO) {
        ret = 0;
        ALOGE("isn't audio packet, skip it...");
        goto cleanup;
    }
    /* Send the audio frame stored in the temporary packet to the decoder.*/
    if ((ret = avcodec_send_packet(resample->inCodecCtx, &resample->inPacket)) < 0) {
        ALOGE("Could not send packet for decoding (error:%s)\n", av_err2str(ret));
        return ret;
    }
    /* Receive one frame from the decoder. */
    ret = avcodec_receive_frame(resample->inCodecCtx, frame);
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
    av_packet_unref(&resample->inPacket);
    return ret;
}

int FFAudioResample::initConvertedSamples(uint8_t ***converted_input_samples, int frame_size) {
    int ret;
    if (!(*converted_input_samples = (uint8_t **) calloc(resample->outCodecCtx->channels,
                                            sizeof(**converted_input_samples)))) {
        ALOGE("Could not allocate converted input sample pointers\n");
        return AVERROR(ENOMEM);
    }

    if ((ret = av_samples_alloc(*converted_input_samples, nullptr,
                                  resample->outCodecCtx->channels,
                                  frame_size,
                                  resample->outCodecCtx->sample_fmt, 0)) < 0) {
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
int FFAudioResample::decodeAndConvert(int *finished) {
    uint8_t **converted_dst_samples = nullptr;
    int data_present = 0;
    int ret = AVERROR_EXIT;

    /* Decode one frame worth of audio samples. */
    if (decodeAudioFrame(resample->inFrame, &data_present, finished))
        goto cleanup;
    if (*finished) {
        ret = 0;
        goto cleanup;
    }
    /* If there is decoded data, convert and store it. */
    if (data_present) {
        int dst_nb_samples = (int) av_rescale_rnd(resample->inFrame->nb_samples, resample->outCodecCtx->sample_rate,
                                            resample->inCodecCtx->sample_rate, AV_ROUND_UP);

        if (initConvertedSamples(&converted_dst_samples, dst_nb_samples))
            goto cleanup;

        ret = swr_convert(resample->resampleCtx, converted_dst_samples, dst_nb_samples,
                          (const uint8_t**)resample->inFrame->extended_data, resample->inFrame->nb_samples);
        if (ret < 0) {
            ALOGE("Could not convert input samples (error:%s)\n", av_err2str(ret));
            goto cleanup;
        }

        if (av_audio_fifo_write(resample->fifo, (void **)converted_dst_samples, dst_nb_samples) < dst_nb_samples)
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

static int initOutputFrame(AudioResample **pResample) {
    AudioResample *ar = *pResample;

    AVFrame *frame        = av_frame_alloc();
    frame->format         = ar->outCodecCtx->sample_fmt;
    frame->nb_samples     = ar->outCodecCtx->frame_size;
    frame->sample_rate    = ar->outCodecCtx->sample_rate;
    frame->channel_layout = ar->outCodecCtx->channel_layout;

    int ret = av_frame_get_buffer(frame, 0);
    ar->outFrame = frame;
    *pResample   = ar;
    return ret;
}

int FFAudioResample::encodeAudioFrame(AVFrame *frame, int *data_present) {
    int ret;

    /* Set a timestamp based on the sample rate for the container. */
    if (frame) {
        frame->pts = resample->pts;
        resample->pts += frame->nb_samples;
    }

    ret = avcodec_send_frame(resample->outCodecCtx, frame);
    if (ret == AVERROR_EOF) {
        ret = 0;
        goto cleanup;
    } else if (ret < 0) {
        ALOGE("Could not send packet for encoding (error:%s)\n", av_err2str(ret));
        return ret;
    }

    ret = avcodec_receive_packet(resample->outCodecCtx, &resample->outPacket);
    if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
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
        (ret = av_write_frame(resample->outFormatCtx, &resample->outPacket)) < 0) {
        ALOGE("Could not write frame (error:%s)\n", av_err2str(ret));
    }

cleanup:
    av_packet_unref(&resample->outPacket);
    return ret;
}

/**
 * Load one audio frame from the FIFO buffer, encode and write it to the
 * output file.
 *
 */
int FFAudioResample::encodeAndWrite() {
    int data_written;
    const int frame_size = FFMIN(av_audio_fifo_size(resample->fifo),
                                 resample->outCodecCtx->frame_size);

    resample->outFrame->nb_samples = frame_size;
    if (av_audio_fifo_read(resample->fifo, (void **)resample->outFrame->data, frame_size) < frame_size) {
        ALOGE("Could not read data from FIFO\n");
        return AVERROR_EXIT;
    }

    if (encodeAudioFrame(resample->outFrame, &data_written)) {
        return AVERROR_EXIT;
    }
    return 0;
}

int FFAudioResample::resampling(const char *src_file, const char *dst_file, int sampleRate) {
    int ret = AVERROR_EXIT;

    /* Open the input file for reading. */
    if (openInputFile(src_file))
        goto cleanup;
    /* Open the output file for writing. */
    if (openOutputFile(dst_file, sampleRate))
        goto cleanup;
    /* Initialize the re-sampler to be able to convert audio sample formats. */
    if (initResample(&resample))
        goto cleanup;
    /* Initialize the FIFO buffer to store audio samples to be encoded. */
    resample->fifo = av_audio_fifo_alloc(resample->outCodecCtx->sample_fmt,
                                         resample->outCodecCtx->channels, 1024 * 10);
    if (initOutputFrame(&resample))
        goto cleanup;
    /* Write the header of the output file container. */
    if ((ret = avformat_write_header(resample->outFormatCtx, nullptr)) < 0) {
        ALOGE("write header error=%s", av_err2str(ret));
    }

    while (true) {
        int finished = 0;
        const int output_frame_size = resample->outCodecCtx->frame_size;

        while (av_audio_fifo_size(resample->fifo) < output_frame_size) {
            /* Decode one frame, convert sample format and put it into the FIFO buffer. */
            if (decodeAndConvert(&finished))
                goto cleanup;

            if (finished)
                break;
        }

        /* If we have enough samples for the encoder, we encode them.*/
        while (av_audio_fifo_size(resample->fifo) >= output_frame_size ||
               (finished && av_audio_fifo_size(resample->fifo) > 0))
            if (encodeAndWrite())
                goto cleanup;

        /* encode all the remaining samples. */
        if (finished) {
            int data_written;
            do {
                data_written = 0;
                if (encodeAudioFrame(nullptr, &data_written))
                    goto cleanup;
            } while (data_written);
            break;
        }
    }

    /* Write the trailer of the output file container. */
    if (av_write_trailer(resample->outFormatCtx)) {
        ALOGE("write trailer error...");
    }
    ret = 0;

cleanup:
    if (resample->fifo)
        av_audio_fifo_free(resample->fifo);
    swr_free(&(resample->resampleCtx));
    if (resample->outCodecCtx)
        avcodec_free_context(&(resample->outCodecCtx));
    if (resample->outFormatCtx) {
        avio_closep(&(resample->outFormatCtx->pb));
        avformat_free_context(resample->outFormatCtx);
    }
    if (resample->inCodecCtx)
        avcodec_free_context(&(resample->inCodecCtx));
    if (resample->inFormatCtx)
        avformat_close_input(&(resample->inFormatCtx));
    if (resample->inFrame)
        av_frame_free(&(resample->inFrame));
    if (resample->outFrame)
        av_frame_free(&(resample->outFrame));

    return ret;
}