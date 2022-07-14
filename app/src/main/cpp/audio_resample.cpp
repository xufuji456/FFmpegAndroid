//
// Created by xu fulong on 2022/7/12.
//

#include <jni.h>

#ifdef __cplusplus
extern "C" {
#endif
#include "libavformat/avformat.h"
#include <libavutil/opt.h>
#include <libavutil/channel_layout.h>
#include <libavutil/samplefmt.h>
#include <libswresample/swresample.h>
#ifdef __cplusplus
}
#endif

#include "ffmpeg_jni_define.h"

#define ALOGE(Format, ...) LOGE("audio_resample", Format, ##__VA_ARGS__)

static void log_error(const char *functionName, int errorNumber) {
    int buffer_len = 1024;
    char *buffer = new char [buffer_len];
    av_strerror(errorNumber, buffer, buffer_len);
    ALOGE("%s: %s", functionName, buffer);
    delete []buffer;
}

static int get_format_from_sample_fmt(const char **fmt, enum AVSampleFormat sample_fmt)
{
    *fmt = nullptr;

    struct sample_fmt_entry {
        enum AVSampleFormat sample_fmt; const char *fmt_be, *fmt_le;
    } sample_fmt_entries[] = {
            { AV_SAMPLE_FMT_U8,  "u8",    "u8"    },
            { AV_SAMPLE_FMT_S16, "s16be", "s16le" },
            { AV_SAMPLE_FMT_S32, "s32be", "s32le" },
            { AV_SAMPLE_FMT_FLT, "f32be", "f32le" },
            { AV_SAMPLE_FMT_DBL, "f64be", "f64le" },
    };

    for (int i = 0; i < FF_ARRAY_ELEMS(sample_fmt_entries); i++) {
        struct sample_fmt_entry *entry = &sample_fmt_entries[i];
        if (sample_fmt == entry->sample_fmt) {
            *fmt = AV_NE(entry->fmt_be, entry->fmt_le);
            return 0;
        }
    }

    ALOGE("Sample format %s not supported as output format, msg=%s\n",
            av_get_sample_fmt_name(sample_fmt), strerror(errno));
    return AVERROR(EINVAL);
}

int init_audio_codec(AVFormatContext *fmt_ctx, AVCodecContext **avcodec_ctx, bool is_encoder) {
    AVCodecContext *codec_ctx = nullptr;
    for (int i = 0; i < fmt_ctx->nb_streams; ++i) {
        if (fmt_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
            codec_ctx = fmt_ctx->streams[i]->codec;
        }
    }
    AVCodec *codec = is_encoder ? avcodec_find_encoder(codec_ctx->codec_id)
            : avcodec_find_decoder(codec_ctx->codec_id);
    if (!codec) {
        ALOGE("can't found codec id=%d\n", codec_ctx->codec_id);
        return -1;
    }
    int ret = avcodec_open2(codec_ctx, codec, nullptr);
    if (ret < 0)
        ALOGE("avcodec_open2 fail:%d", ret);
    *avcodec_ctx = codec_ctx;
    return ret;
}

int init_audio_decoder(AVFormatContext *fmt_ctx, AVCodecContext **avcodec_ctx) {
    return init_audio_codec(fmt_ctx, avcodec_ctx, false);
}

int init_audio_encoder(AVFormatContext *fmt_ctx, AVCodecContext **avcodec_ctx) {
    return init_audio_codec(fmt_ctx, avcodec_ctx, true);
}

int init_audio_muxer(AVFormatContext *ifmt_ctx, AVFormatContext **ofmt_ctx, const char* filename,
                     int sample_rate, int channels, int64_t channel_layout, AVSampleFormat sampleFormat) {
    int ret;
    AVFormatContext *fmt_ctx = *ofmt_ctx;
    avformat_alloc_output_context2(&fmt_ctx, nullptr, nullptr, filename);
    if (!(fmt_ctx->oformat->flags & AVFMT_NOFILE)) {
        ret = avio_open(&fmt_ctx->pb, filename, AVIO_FLAG_WRITE);
        if (ret < 0) {
            ALOGE("Could not open output file %s\n", filename);
            return ret;
        }
    }
    av_dump_format(fmt_ctx, 0, filename, 1);

    for (int i = 0; i < ifmt_ctx->nb_streams; i++) {
        AVStream* in_stream = ifmt_ctx->streams[i];
        AVCodecParameters* codecpar = in_stream->codecpar;
        AVCodec* encoder = avcodec_find_encoder(codecpar->codec_id);
        AVStream* out_stream = avformat_new_stream(fmt_ctx, encoder);
        avcodec_parameters_copy(out_stream->codecpar, codecpar);

        if(out_stream->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
            out_stream->codecpar->channels = channels;
            out_stream->codecpar->sample_rate = sample_rate;
            out_stream->codecpar->channel_layout = channel_layout;
            out_stream->codec->sample_fmt = sampleFormat;
            avcodec_parameters_to_context(out_stream->codec, out_stream->codecpar);
            out_stream->time_base = in_stream->codec->time_base;
            out_stream->duration = in_stream->duration;
            break;
        }
    }

    ret = avformat_write_header(fmt_ctx, nullptr);
    if (ret < 0) {
        log_error("Error occurred when opening output file", ret);
    }
    *ofmt_ctx = fmt_ctx;
    return ret;
}

void init_out_frame(AVFrame **frame, AVCodecContext *codec_ctx, int nb_samples) {
    AVFrame *out_frame = *frame;
    av_frame_free(&out_frame);
    out_frame = av_frame_alloc();
    out_frame->nb_samples = nb_samples;
    out_frame->format = codec_ctx->sample_fmt;
    out_frame->sample_rate = codec_ctx->sample_rate;
    out_frame->channel_layout = codec_ctx->channel_layout;
    av_frame_get_buffer(out_frame,0);
    av_frame_make_writable(out_frame);
    *frame = out_frame;
}

int resampling(const char *src_filename, const char *dst_filename, int dst_rate)
{
    int src_rate;
    int64_t src_ch_layout;
    enum AVSampleFormat src_sample_fmt;

    int dst_bufsize;
    int dst_linesize;
    int dst_nb_channels;
    int dst_nb_samples, max_dst_nb_samples;
    int64_t dst_ch_layout = AV_CH_LAYOUT_STEREO;
    enum AVSampleFormat dst_sample_fmt = AV_SAMPLE_FMT_S16;

    int ret;
    const char *fmt;
    AVPacket packet;
    AVPacket *opacket;
    AVFrame *frame;
    int got_frame_ptr;
    int got_packet_ptr;
    struct SwrContext *swr_ctx;
    AVFormatContext *iformat_ctx = nullptr;
    AVFormatContext *oformat_ctx = nullptr;
    AVCodecContext  *icodec_ctx  = nullptr;
    AVCodecContext  *ocodec_ctx  = nullptr;

    ret = avformat_open_input(&iformat_ctx, src_filename, nullptr, nullptr);
    if (ret < 0) {
        ALOGE("open input fail, path=%s, ret=%d", src_filename, ret);
        goto end;
    }
    avformat_find_stream_info(iformat_ctx, nullptr);
    frame = av_frame_alloc();
    opacket = av_packet_alloc();
    init_audio_decoder(iformat_ctx, &icodec_ctx);
    src_rate       = icodec_ctx->sample_rate;
    src_ch_layout  = (int64_t) icodec_ctx->channel_layout;
    src_sample_fmt = icodec_ctx->sample_fmt;

    /* create resample context */
    swr_ctx = swr_alloc();
    if (!swr_ctx) {
        ALOGE("Could not allocate resample context...\n");
        ret = AVERROR(ENOMEM);
        goto end;
    }

    /* set options */
    av_opt_set_int(swr_ctx, "in_channel_layout",    src_ch_layout, 0);
    av_opt_set_int(swr_ctx, "in_sample_rate",       src_rate, 0);
    av_opt_set_sample_fmt(swr_ctx, "in_sample_fmt", src_sample_fmt, 0);

    av_opt_set_int(swr_ctx, "out_channel_layout",    dst_ch_layout, 0);
    av_opt_set_int(swr_ctx, "out_sample_rate",       dst_rate, 0);
    av_opt_set_sample_fmt(swr_ctx, "out_sample_fmt", dst_sample_fmt, 0);

    /* initialize the resampling context */
    if ((ret = swr_init(swr_ctx)) < 0) {
        log_error("Failed to initialize the resampling context", ret);
        goto end;
    }

    dst_nb_samples = (int) av_rescale_rnd(src_rate, dst_rate, src_rate, AV_ROUND_UP);
    max_dst_nb_samples = dst_nb_samples;
    dst_nb_channels = av_get_channel_layout_nb_channels(dst_ch_layout);

    ret = init_audio_muxer(iformat_ctx, &oformat_ctx, dst_filename, dst_rate, dst_nb_channels, dst_ch_layout, dst_sample_fmt);
    if (ret < 0) {
        goto end;
    }
    init_audio_encoder(oformat_ctx, &ocodec_ctx);

    while (av_read_frame(iformat_ctx, &packet) >= 0) {

        ret = avcodec_decode_audio4(icodec_ctx, frame, &got_frame_ptr, &packet);
        if (ret < 0) {
            ALOGE("decode audio error:%d\n", ret);
            continue;
        }
        ALOGE("decode succ, pts=%ld\n", frame->pts);

        /* compute destination number of samples */
        dst_nb_samples = (int) av_rescale_rnd(swr_get_delay(swr_ctx, src_rate) +
                                        frame->nb_samples, dst_rate, src_rate, AV_ROUND_UP);
        if (dst_nb_samples > max_dst_nb_samples) {
            init_out_frame(&frame, ocodec_ctx, dst_nb_samples);
            max_dst_nb_samples = dst_nb_samples;
        }

        /* convert to destination format */
        int samples_result = swr_convert(swr_ctx, frame->data, dst_nb_samples, (const uint8_t **)frame->data, frame->nb_samples);
        if (samples_result < 0) {
            ALOGE("Error while converting...");
            goto end;
        }
        dst_bufsize = av_samples_get_buffer_size(&dst_linesize, dst_nb_channels,
                                                 samples_result, dst_sample_fmt, 1);
        if (dst_bufsize < 0) {
            ALOGE("Could not get sample buffer size...");
            goto end;
        }
        ALOGE("resample size=%d", dst_bufsize);

        ret = avcodec_encode_audio2(ocodec_ctx, opacket, frame, &got_packet_ptr);
        if (ret < 0) {
            log_error("encode audio error", ret);
            continue;
        }

        AVStream *in_stream = iformat_ctx->streams[0];
        AVStream *out_stream = oformat_ctx->streams[0];
        opacket->pts = av_rescale_q_rnd(packet.pts, in_stream->time_base, out_stream->time_base,
                                    static_cast<AVRounding>(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
        opacket->dts = av_rescale_q_rnd(packet.dts, in_stream->time_base, out_stream->time_base,
                                    static_cast<AVRounding>(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
        opacket->duration = av_rescale_q(packet.duration, in_stream->time_base, out_stream->time_base);
        opacket->pos = -1;

        av_interleaved_write_frame(oformat_ctx, opacket);

        av_packet_unref(opacket);
        av_packet_unref(&packet);
    }

    if ((ret = get_format_from_sample_fmt(&fmt, dst_sample_fmt)) < 0)
        goto end;
    ALOGE("Resampling succeeded. Play the output file with the command:\n"
                    "ffplay -f %s -channel_layout %" PRId64 " -channels %d -ar %d %s\n",
            fmt, dst_ch_layout, dst_nb_channels, dst_rate, dst_filename);

end:

    av_packet_free(&opacket);
    av_frame_free(&frame);

    swr_free(&swr_ctx);
    avformat_close_input(&iformat_ctx);

    if (oformat_ctx) {
        av_write_trailer(oformat_ctx);
        if (!(oformat_ctx->oformat->flags & AVFMT_NOFILE)) {
            avio_close(oformat_ctx->pb);
        }
        avformat_free_context(oformat_ctx);
    }
    return ret;
}

#ifdef __cplusplus
extern "C" {
#endif
VIDEO_PLAYER_FUNC(int, audioResample, jstring srcFile, jstring dstFile, int sampleRate) {
    const char *src_file = env->GetStringUTFChars(srcFile, JNI_FALSE);
    const char *dst_file = env->GetStringUTFChars(dstFile, JNI_FALSE);
    int ret = resampling(src_file, dst_file, sampleRate);
    env->ReleaseStringUTFChars(dstFile, dst_file);
    env->ReleaseStringUTFChars(srcFile, src_file);
    return ret;
}
#ifdef __cplusplus
}
#endif