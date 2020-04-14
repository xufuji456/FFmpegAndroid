//
// Created by frank on 2018/2/2.
//

#include <jni.h>
#include <string>
#include <android/log.h>
#include "ffmpeg_jni_define.h"

#define TAG "FFmpegPusher"

#ifdef __cplusplus
extern "C" {
#endif
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavfilter/avfilter.h>
#include <libavutil/time.h>

PUSHER_FUNC(jint, pushStream, jstring filePath, jstring liveUrl) {

    AVOutputFormat *output_format = NULL;
    AVFormatContext *in_format = NULL, *out_format = NULL;
    AVPacket packet;
    const char *file_path, *live_url;
    int video_index = -1;
    int ret = 0, i;
    int frame_index = 0;
    int64_t start_time = 0;

    file_path = env->GetStringUTFChars(filePath, NULL);
    live_url = env->GetStringUTFChars(liveUrl, NULL);

    LOGE(TAG, "file_path=%s", file_path);
    LOGE(TAG, "live_url=%s", live_url);

    av_register_all();
    avformat_network_init();
    if ((ret = avformat_open_input(&in_format, file_path, 0, 0)) < 0) {
        LOGE(TAG, "could not open input file...");
        goto end;
    }
    if ((ret = avformat_find_stream_info(in_format, 0)) < 0) {
        LOGE(TAG, "could not find stream info...");
        goto end;
    }
    for (i = 0; i < in_format->nb_streams; i++) {
        if (in_format->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO) {
            video_index = i;
            break;
        }
    }
    av_dump_format(in_format, 0, file_path, 0);
    avformat_alloc_output_context2(&out_format, NULL, "flv", live_url);
    if (!out_format) {
        LOGE(TAG, "could not alloc output context...");
        ret = AVERROR_UNKNOWN;
        goto end;
    }
    //Create output stream according to input stream
    for (i = 0; i < in_format->nb_streams; i++) {
        AVStream *in_stream = in_format->streams[i];
        AVStream *out_stream = avformat_new_stream(out_format, in_stream->codec->codec);
        if (!out_stream) {
            LOGE(TAG, "could not alloc output stream...");
            ret = AVERROR_UNKNOWN;
            goto end;
        }
        //Copy context
        ret = avcodec_copy_context(out_stream->codec, in_stream->codec);
        if (ret < 0) {
            LOGE(TAG, "could not copy context...");
            goto end;
        }
        out_stream->codec->codec_tag = 0;
        if (out_format->oformat->flags & AVFMT_GLOBALHEADER) {
            out_stream->codec->flags |= CODEC_FLAG_GLOBAL_HEADER;
        }
    }
    output_format = out_format->oformat;
    if (!(output_format->flags & AVFMT_NOFILE)) {
        ret = avio_open(&out_format->pb, live_url, AVIO_FLAG_WRITE);
        if (ret < 0) {
            LOGE(TAG, "could not open output url '%s'", live_url);
            goto end;
        }
    }
    //Write header
    ret = avformat_write_header(out_format, NULL);
    if (ret < 0) {
        LOGE(TAG, "could not write header...");
        goto end;
    }
    start_time = av_gettime();
    while (1) {
        AVStream *in_stream, *out_stream;
        ret = av_read_frame(in_format, &packet);
        if (ret < 0) {
            break;
        }
        //计算帧间隔，参考时钟/采样率
        if (packet.pts == AV_NOPTS_VALUE) {
            AVRational time_base = in_format->streams[video_index]->time_base;
            int64_t cal_duration = (int64_t) (AV_TIME_BASE /
                                              av_q2d(in_format->streams[video_index]->r_frame_rate));
            packet.pts = (int64_t) ((frame_index * cal_duration) /
                                    (av_q2d(time_base) * AV_TIME_BASE));
            packet.dts = packet.pts;
            packet.duration = (int64_t) (cal_duration / (av_q2d(time_base) * AV_TIME_BASE));
        }
        if (packet.stream_index == video_index) {
            AVRational time_base = in_format->streams[video_index]->time_base;
            AVRational time_base_q = {1, AV_TIME_BASE};
            int64_t pts_time = av_rescale_q(packet.dts, time_base, time_base_q);
            int64_t now_time = av_gettime() - start_time;
            //sleep to keep waiting
            if (pts_time > now_time) {
                av_usleep((unsigned int) (pts_time - now_time));
            }
        }
        in_stream = in_format->streams[packet.stream_index];
        out_stream = out_format->streams[packet.stream_index];

        //pts to dts
        packet.pts = av_rescale_q_rnd(packet.pts, in_stream->time_base, out_stream->time_base,
                                      (AVRounding) (AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
        packet.dts = av_rescale_q_rnd(packet.dts, in_stream->time_base, out_stream->time_base,
                                      (AVRounding) (AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
        packet.duration = av_rescale_q(packet.duration, in_stream->time_base,
                                       out_stream->time_base);
        packet.pos = -1;

        if (packet.stream_index == video_index) {
            frame_index++;
            LOGI(TAG, "write frame = %d", frame_index);
        }
        //Write a frame
        ret = av_interleaved_write_frame(out_format, &packet);
        if (ret < 0) {
            LOGE(TAG, "could not write frame...");
            break;
        }
        //Release packet
        av_packet_unref(&packet);
    }
    //Write tail
    av_write_trailer(out_format);

    end:
    avformat_close_input(&in_format);
    if (out_format && !(out_format->flags & AVFMT_NOFILE)) {
        avio_close(out_format->pb);
    }
    avformat_free_context(in_format);
    avformat_free_context(out_format);
    env->ReleaseStringUTFChars(filePath, file_path);
    env->ReleaseStringUTFChars(liveUrl, live_url);
    if (ret < 0 && ret != AVERROR_EOF) {
        return -1;
    }
    return 0;
}

#ifdef __cplusplus
}
#endif