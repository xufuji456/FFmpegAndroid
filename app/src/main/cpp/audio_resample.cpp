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
    AVCodec *codec = is_encoder ? avcodec_find_encoder(fmt_ctx->audio_codec_id)
            : avcodec_find_decoder(fmt_ctx->audio_codec_id);
    if (!codec) {
        ALOGE("can't found codec id=%d\n", fmt_ctx->audio_codec_id);
        return -1;
    }
    AVCodecContext *codec_ctx = avcodec_alloc_context3(codec);
    if (!codec_ctx) {
        ALOGE("avcodec_alloc_context3 fail!\n");
        return -2;
    }
    int ret = avcodec_open2(codec_ctx, codec, nullptr);
    *avcodec_ctx = codec_ctx;
    return ret;
}

int init_audio_decoder(AVFormatContext *fmt_ctx, AVCodecContext **avcodec_ctx) {
    return init_audio_codec(fmt_ctx, avcodec_ctx, false);
}

int init_audio_encoder(AVFormatContext *fmt_ctx, AVCodecContext **avcodec_ctx) {
    return init_audio_codec(fmt_ctx, avcodec_ctx, true);
}

int init_audio_muxer(AVFormatContext **ofmt_ctx, const char* filename) {
    int ret;
    AVFormatContext *fmt_ctx = *ofmt_ctx;
    avformat_alloc_output_context2(&fmt_ctx, nullptr, nullptr, filename);
    av_dump_format(fmt_ctx, 0, filename, 1);
    if (!(fmt_ctx->oformat->flags & AVFMT_NOFILE)) {
        ret = avio_open(&fmt_ctx->pb, filename, AVIO_FLAG_WRITE);
        if (ret < 0) {
            ALOGE("Could not open output file %s\n", filename);
            return ret;
        }
    }
    /* init muxer, write output file header */
    ret = avformat_write_header(fmt_ctx, nullptr);
    if (ret < 0) {
        ALOGE("Error occurred when opening output file\n");
    }
    *ofmt_ctx = fmt_ctx;
    return ret;
}

int resampling(const char *src_filename, const char *dst_filename, int dst_rate)
{
    int src_rate = 0;
    int src_nb_samples = 0;
    int64_t src_ch_layout = AV_CH_LAYOUT_STEREO;
    enum AVSampleFormat src_sample_fmt = AV_SAMPLE_FMT_S16;

    int dst_bufsize;
    int dst_linesize;
    int dst_nb_channels;
    uint8_t **dst_data = nullptr;
    int dst_nb_samples, max_dst_nb_samples;
    int64_t dst_ch_layout = AV_CH_LAYOUT_SURROUND;
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

    /* create resample context */
    swr_ctx = swr_alloc();
    if (!swr_ctx) {
        ALOGE("Could not allocate resample context:%s\n", strerror(errno));
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
        ALOGE("Failed to initialize the resampling context:%s\n", strerror(errno));
        goto end;
    }

    /* compute the number of converted samples: buffering is avoided
     * ensuring that the output buffer will contain at least all the
     * converted input samples */
    max_dst_nb_samples = dst_nb_samples =
            (int) av_rescale_rnd(src_nb_samples, dst_rate, src_rate, AV_ROUND_UP);

    /* buffer is going to be directly written to a raw-audio file, no alignment */
    dst_nb_channels = av_get_channel_layout_nb_channels(dst_ch_layout);
    ret = av_samples_alloc_array_and_samples(&dst_data, &dst_linesize, dst_nb_channels,
                                             dst_nb_samples, dst_sample_fmt, 0);
    if (ret < 0) {
        ALOGE("Could not allocate destination samples:%s\n", strerror(errno));
        goto end;
    }

    ret = init_audio_muxer(&oformat_ctx, dst_filename);
    if (ret < 0) {
        goto end;
    }
    init_audio_decoder(iformat_ctx, &icodec_ctx);
    init_audio_encoder(oformat_ctx, &ocodec_ctx);

    while (av_read_frame(iformat_ctx, &packet) >= 0) {
        /* compute destination number of samples */
        dst_nb_samples = (int) av_rescale_rnd(swr_get_delay(swr_ctx, src_rate) +
                                        src_nb_samples, dst_rate, src_rate, AV_ROUND_UP);
        if (dst_nb_samples > max_dst_nb_samples) {
            av_freep(&dst_data[0]);
            ret = av_samples_alloc(dst_data, &dst_linesize, dst_nb_channels,
                                   dst_nb_samples, dst_sample_fmt, 1);
            if (ret < 0)
                break;
            max_dst_nb_samples = dst_nb_samples;
        }

        ret = avcodec_decode_audio4(icodec_ctx, frame, &got_frame_ptr, &packet);
        if (ret < 0) {
            ALOGE("decode audio error:%d\n", ret);
            continue;
        }

        /* convert to destination format */
        ret = swr_convert(swr_ctx, dst_data, dst_nb_samples, (const uint8_t **)frame->data, frame->nb_samples);
        if (ret < 0) {
            ALOGE("Error while converting:%s\n", strerror(errno));
            goto end;
        }
        dst_bufsize = av_samples_get_buffer_size(&dst_linesize, dst_nb_channels,
                                                 ret, dst_sample_fmt, 1);
        if (dst_bufsize < 0) {
            ALOGE("Could not get sample buffer size:%s\n", strerror(errno));
            goto end;
        }

        ret = avcodec_encode_audio2(ocodec_ctx, opacket, frame, &got_packet_ptr);
        if (ret < 0) {
            ALOGE("encode audio error=%d\n", ret);
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

    if (dst_data)
        av_freep(&dst_data[0]);
    av_freep(&dst_data);

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