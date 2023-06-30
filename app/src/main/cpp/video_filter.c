//
// Created by frank on 2018/6/4.
//

#include <stdio.h>
#include <unistd.h>
#include <stdbool.h>

#include <android/native_window.h>
#include <android/native_window_jni.h>

#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"
#include "libavutil/imgutils.h"
#include "libavutil/opt.h"
#include "libavutil/time.h"
#include "libavfilter/buffersink.h"
#include "libavfilter/buffersrc.h"
#include "libavfilter/avfilter.h"
#include "libswresample/swresample.h"
#include "ffmpeg_jni_define.h"

#define TAG "VideoFilter"
#define MAX_AUDIO_FRAME_SIZE (48000 * 4)

typedef struct VideoFilterContext {
    AVFormatContext *format_ctx;
    AVCodecContext  *audio_codec_ctx;
    AVCodecContext  *video_codec_ctx;

    uint8_t *buffer;
    AVFrame *frame_src;
    AVFrame *frame_rgb;
    uint8_t *out_buffer;

    ANativeWindow *native_window;

    SwrContext *audio_swr_ctx;
    struct SwsContext *sws_ctx;
    enum AVSampleFormat out_sample_fmt;

    int video_stream_index;
    int audio_stream_index;

    int out_channel_nb;
    int64_t start_time;
} VideoFilterContext;

int pos;
int again;
int release;
bool enable_audio;
jobject audio_track;
jmethodID audio_track_write_mid;

const char* filters[] = {
        "lutyuv='u=128:v=128'",
        "eq=brightness=0.1", // -1.0 to 1.0 (default 0)
        "eq=saturation=1.5", // 0.0 to 3.0 (default 1)
        "eq=contrast=1.2",   // -1000.0 to 1000.0 (default 1)
        "unsharp",
        "edgedetect=low=0.1:high=0.4",
        "stereo3d=sbsl:arcd", // arbg
        "drawgrid=w=iw/3:h=ih/3:t=2:c=white@0.5",
        "hflip",
        "colorbalance=bs=0.3",
        "gblur=sigma=2:steps=1:planes=1:sigmaV=1",
        "rotate=180*PI/180"
};

int init_filters(const char *filters_descr, AVRational time_base, AVCodecContext *codecCtx,
        AVFilterGraph **graph, AVFilterContext **src, AVFilterContext **sink) {
    char args[512];
    int ret;
    AVFilterContext *buffersrc_ctx;
    AVFilterContext *buffersink_ctx;
    const AVFilter *buffersrc = avfilter_get_by_name("buffer");
    const AVFilter *buffersink = avfilter_get_by_name("buffersink");
    AVFilterInOut *outputs = avfilter_inout_alloc();
    AVFilterInOut *inputs = avfilter_inout_alloc();
    enum AVPixelFormat pix_fmts[] = {AV_PIX_FMT_YUV420P, AV_PIX_FMT_NONE};

    AVFilterGraph *filter_graph = avfilter_graph_alloc();
    if (!outputs || !inputs || !filter_graph) {
        ret = AVERROR(ENOMEM);
        goto end;
    }
    /* buffer video source: the decoded frames from the decoder will be inserted here. */
    snprintf(args, sizeof(args),
             "video_size=%dx%d:pix_fmt=%d:time_base=%d/%d:pixel_aspect=%d/%d",
             codecCtx->width, codecCtx->height, codecCtx->pix_fmt,
             time_base.num, time_base.den,
             codecCtx->sample_aspect_ratio.num, codecCtx->sample_aspect_ratio.den);
    ret = avfilter_graph_create_filter(&buffersrc_ctx, buffersrc, "in",
                                       args, NULL, filter_graph);
    if (ret < 0) {
        LOGE(TAG, "Cannot create buffer source\n");
        goto end;
    }
    /* buffer video sink: to terminate the filter chain. */
    ret = avfilter_graph_create_filter(&buffersink_ctx, buffersink, "out",
                                       NULL, NULL, filter_graph);
    if (ret < 0) {
        LOGE(TAG, "Cannot create buffer sink\n");
        goto end;
    }
    ret = av_opt_set_int_list(buffersink_ctx, "pix_fmts", pix_fmts,
                              AV_PIX_FMT_NONE, AV_OPT_SEARCH_CHILDREN);
    if (ret < 0) {
        LOGE(TAG, "Cannot set output pixel format\n");
        goto end;
    }

    outputs->name = av_strdup("in");
    outputs->filter_ctx = buffersrc_ctx;
    outputs->pad_idx = 0;
    outputs->next = NULL;
    inputs->name = av_strdup("out");
    inputs->filter_ctx = buffersink_ctx;
    inputs->pad_idx = 0;
    inputs->next = NULL;

    if ((ret = avfilter_graph_parse_ptr(filter_graph, filters_descr,
                                        &inputs, &outputs, NULL)) < 0)
        goto end;
    if ((ret = avfilter_graph_config(filter_graph, NULL)) < 0)
        goto end;

    *graph = filter_graph;
    *src   = buffersrc_ctx;
    *sink  = buffersink_ctx;
end:
    avfilter_inout_free(&inputs);
    avfilter_inout_free(&outputs);
    return ret;
}

//init player
int open_input(JNIEnv *env, const char *file_name, jobject surface, VideoFilterContext *s) {
    LOGI(TAG, "open file:%s\n", file_name);
    s->format_ctx = avformat_alloc_context();
    if (avformat_open_input(&s->format_ctx, file_name, NULL, NULL) != 0) {
        LOGE(TAG, "Couldn't open file:%s\n", file_name);
        return -1;
    }
    if (avformat_find_stream_info(s->format_ctx, NULL) < 0) {
        LOGE(TAG, "Couldn't find stream information.");
        return -1;
    }

    for (int i = 0; i < s->format_ctx->nb_streams; i++) {
        if (s->format_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            s->video_stream_index = i;
            break;
        }
    }
    if (s->video_stream_index == -1) {
        LOGE(TAG, "couldn't find a video stream.");
        return -1;
    }

    AVCodecParameters *videoCodec = s->format_ctx->streams[s->video_stream_index]->codecpar;
    AVCodec *pCodec = avcodec_find_decoder(videoCodec->codec_id);
    if (pCodec == NULL) {
        LOGE(TAG, "couldn't find Codec.");
        return -1;
    }
    s->video_codec_ctx = avcodec_alloc_context3(pCodec);
    avcodec_parameters_to_context(s->video_codec_ctx, videoCodec);
    if (avcodec_open2(s->video_codec_ctx, pCodec, NULL) < 0) {
        LOGE(TAG, "Couldn't open codec.");
        return -1;
    }

    s->native_window = ANativeWindow_fromSurface(env, surface);
    if (!s->native_window) {
        LOGE(TAG, "nativeWindow is null...");
        return -1;
    }
    ANativeWindow_setBuffersGeometry(s->native_window, s->video_codec_ctx->width,
                                     s->video_codec_ctx->height,WINDOW_FORMAT_RGBA_8888);
    s->frame_src = av_frame_alloc();
    s->frame_rgb = av_frame_alloc();
    if (s->frame_rgb == NULL || s->frame_src == NULL) {
        LOGE(TAG, "Couldn't allocate video frame.");
        return -1;
    }
    int numBytes = av_image_get_buffer_size(AV_PIX_FMT_RGBA, s->video_codec_ctx->width,
                                            s->video_codec_ctx->height,1);

    s->buffer = (uint8_t *) av_malloc(numBytes * sizeof(uint8_t));
    av_image_fill_arrays(s->frame_rgb->data, s->frame_rgb->linesize, s->buffer, AV_PIX_FMT_RGBA,
                         s->video_codec_ctx->width, s->video_codec_ctx->height, 1);
    s->sws_ctx = sws_getContext(s->video_codec_ctx->width,
                                s->video_codec_ctx->height,
                                s->video_codec_ctx->pix_fmt,
                                s->video_codec_ctx->width,
                                s->video_codec_ctx->height,
                             AV_PIX_FMT_RGBA,
                             SWS_BILINEAR,
                             NULL,
                             NULL,
                             NULL);

    return 0;
}

int init_audio(JNIEnv *env, jclass jthiz, VideoFilterContext *s) {
    for (int i = 0; i < s->format_ctx->nb_streams; i++) {
        if (s->format_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
            s->audio_stream_index = i;
            break;
        }
    }

    AVCodecParameters *audioCodec = s->format_ctx->streams[s->audio_stream_index]->codecpar;
    AVCodec *codec = avcodec_find_decoder(audioCodec->codec_id);
    if (codec == NULL) {
        LOGE(TAG, "could not find audio decoder");
        return -1;
    }
    s->audio_codec_ctx = avcodec_alloc_context3(codec);
    avcodec_parameters_to_context(s->audio_codec_ctx, audioCodec);
    if (avcodec_open2(s->audio_codec_ctx, codec, NULL) < 0) {
        LOGE(TAG, "could not open audio decoder");
        return -1;
    }

    s->audio_swr_ctx       = swr_alloc();
    s->out_sample_fmt      = AV_SAMPLE_FMT_S16;
    int in_sample_rate     = s->audio_codec_ctx->sample_rate;
    int out_sample_rate    = in_sample_rate;
    uint64_t in_ch_layout  = s->audio_codec_ctx->channel_layout;
    uint64_t out_ch_layout = AV_CH_LAYOUT_STEREO;
    enum AVSampleFormat in_sample_fmt = s->audio_codec_ctx->sample_fmt;

    swr_alloc_set_opts(s->audio_swr_ctx,
                       (int64_t)out_ch_layout, s->out_sample_fmt, out_sample_rate,
                       (int64_t)in_ch_layout, in_sample_fmt, in_sample_rate,
                       0, NULL);
    swr_init(s->audio_swr_ctx);
    s->out_channel_nb = av_get_channel_layout_nb_channels(out_ch_layout);

    jclass player_class = (*env)->GetObjectClass(env, jthiz);
    if (!player_class) {
        LOGE(TAG, "player_class not found...");
        return -1;
    }
    jmethodID audio_track_method = (*env)->GetMethodID(env, player_class, "createAudioTrack",
                                                       "(II)Landroid/media/AudioTrack;");
    if (!audio_track_method) {
        LOGE(TAG, "audio_track_method not found...");
        return -1;
    }
    audio_track = (*env)->CallObjectMethod(env, jthiz, audio_track_method, out_sample_rate,
                                           s->out_channel_nb);
    jclass audio_track_class = (*env)->GetObjectClass(env, audio_track);
    jmethodID audio_track_play_mid = (*env)->GetMethodID(env, audio_track_class, "play", "()V");
    (*env)->CallVoidMethod(env, audio_track, audio_track_play_mid);

    audio_track_write_mid = (*env)->GetMethodID(env, audio_track_class, "write", "([BII)I");
    s->out_buffer = (uint8_t *) av_malloc(MAX_AUDIO_FRAME_SIZE);
    return 0;
}

int play_audio(JNIEnv *env, AVPacket *packet, AVFrame *frame, VideoFilterContext *s) {
    int ret = avcodec_send_packet(s->audio_codec_ctx, packet);
    if (ret < 0)
        return ret;

    while (ret >= 0) {
        ret = avcodec_receive_frame(s->audio_codec_ctx, frame);
        if (ret == AVERROR_EOF || ret == AVERROR(EAGAIN))
            break;
        else if (ret < 0) {
            LOGE(TAG, "decode error=%s", av_err2str(ret));
            break;
        }

        swr_convert(s->audio_swr_ctx, &s->out_buffer, MAX_AUDIO_FRAME_SIZE,
                    (const uint8_t **) frame->data, frame->nb_samples);
        int out_buffer_size = av_samples_get_buffer_size(NULL, s->out_channel_nb,
                                                         frame->nb_samples, s->out_sample_fmt, 1);

        jbyteArray audio_sample_array = (*env)->NewByteArray(env, out_buffer_size);
        jbyte *sample_byte_array = (*env)->GetByteArrayElements(env, audio_sample_array, NULL);
        memcpy(sample_byte_array, s->out_buffer, (size_t) out_buffer_size);
        (*env)->ReleaseByteArrayElements(env, audio_sample_array, sample_byte_array, 0);
        (*env)->CallIntMethod(env, audio_track, audio_track_write_mid,
                              audio_sample_array, 0, out_buffer_size);
        (*env)->DeleteLocalRef(env, audio_sample_array);

    }
    return ret;
}

int render_video(AVFilterContext *buffersrc_ctx, AVFilterContext *buffersink_ctx, AVFrame *filter_frame,
                 VideoFilterContext *s) {
    int ret = av_buffersrc_add_frame_flags(buffersrc_ctx, s->frame_src,
                                           AV_BUFFERSRC_FLAG_KEEP_REF);
    if (ret < 0) {
        LOGE(TAG, "Error while feeding the filter_graph\n");
        return ret;
    }
    //take frame from filter graph
    ret = av_buffersink_get_frame(buffersink_ctx, filter_frame);
    if (ret >= 0) {
        // lock native window
        ANativeWindow_Buffer windowBuffer;
        ANativeWindow_lock(s->native_window, &windowBuffer, NULL);
        // convert
        sws_scale(s->sws_ctx, (uint8_t const *const *) filter_frame->data,
                  filter_frame->linesize, 0, s->video_codec_ctx->height,
                  s->frame_rgb->data, s->frame_rgb->linesize);
        uint8_t *dst  = windowBuffer.bits;
        int dstStride = windowBuffer.stride * 4;
        uint8_t *src  = s->frame_rgb->data[0];
        int srcStride = s->frame_rgb->linesize[0];
        for (int h = 0; h < s->video_codec_ctx->height; h++) {
            memcpy(dst + h * dstStride, src + h * srcStride, (size_t) srcStride);
        }
        ANativeWindow_unlockAndPost(s->native_window);
    }
    int64_t pts = filter_frame->pts;
    av_frame_unref(filter_frame);

    AVRational time_base = s->format_ctx->streams[s->video_stream_index]->time_base;
    double timestamp = (double)pts * av_q2d(time_base);
    int64_t master_clock = av_gettime_relative() - s->start_time;
    int64_t diff = (int64_t)(timestamp * 1000 * 1000) - master_clock; // microseconds
    if (diff > 0) {
        usleep(diff);
    }
    return ret;
}

VIDEO_PLAYER_FUNC(jint, filter, jstring filePath, jobject surface, jint position) {
    int ret;
    pos = position;
    AVFilterGraph *filter_graph;
    AVFilterContext *buffersrc_ctx;
    AVFilterContext *buffersink_ctx;
    AVPacket *packet      = av_packet_alloc();
    AVFrame *filter_frame = av_frame_alloc();
    const char *file_name = (*env)->GetStringUTFChars(env, filePath, JNI_FALSE);
    VideoFilterContext *s = malloc(sizeof (VideoFilterContext));

    if ((ret = open_input(env, file_name, surface, s)) < 0) {
        LOGE(TAG, "Couldn't allocate video frame.");
        goto end;
    }

    //init audio decoder
    if ((ret = init_audio(env, thiz, s)) < 0) {
        LOGE(TAG, "Couldn't init_audio.");
        goto end;
    }

    //init filter
    AVRational time_base = s->format_ctx->streams[s->video_stream_index]->time_base;
    if ((ret = init_filters(filters[pos], time_base, s->video_codec_ctx,
            &filter_graph, &buffersrc_ctx, &buffersink_ctx)) < 0) {
        LOGE(TAG, "init_filter error, ret=%d\n", ret);
        goto end;
    }
    s->start_time = av_gettime_relative();

    while (av_read_frame(s->format_ctx, packet) >= 0 && !release) {
        //switch filter
        if (again) {
            again = 0;
            avfilter_graph_free(&filter_graph);
            if ((ret = init_filters(filters[pos], time_base, s->video_codec_ctx,
                    &filter_graph, &buffersrc_ctx, &buffersink_ctx)) < 0) {
                LOGE(TAG, "init_filter error, ret=%d\n", ret);
                goto end;
            }
            LOGE(TAG, "play again,filter_descr=_=%s", filters[pos]);
        }
        //is video stream or not
        if (packet->stream_index == s->video_stream_index) {
            ret = avcodec_send_packet(s->video_codec_ctx, packet);
            if (ret < 0)
                goto end;

            while (ret >= 0) {
                ret = avcodec_receive_frame(s->video_codec_ctx, s->frame_src);
                if (ret == AVERROR_EOF || ret == AVERROR(EAGAIN))
                    break;
                else if (ret < 0) {
                    LOGE(TAG, "decode error=%s", av_err2str(ret));
                    goto end;
                }
                ret = render_video(buffersrc_ctx, buffersink_ctx, filter_frame, s);
            }
        } else if (packet->stream_index == s->audio_stream_index) {//audio stream
            if (enable_audio) {
                play_audio(env, packet, s->frame_src, s);
            }
        }
        av_packet_unref(packet);
    }
end:
    av_free(s->buffer);
    av_free(s->out_buffer);
    sws_freeContext(s->sws_ctx);
    swr_free(&s->audio_swr_ctx);
    avfilter_graph_free(&filter_graph);
    avcodec_free_context(&s->video_codec_ctx);
    avcodec_free_context(&s->audio_codec_ctx);
    avformat_close_input(&s->format_ctx);
    av_frame_free(&s->frame_rgb);
    av_frame_free(&filter_frame);
    av_frame_free(&s->frame_src);
    av_packet_free(&packet);

    audio_track = NULL;
    audio_track_write_mid = NULL;
    ANativeWindow_release(s->native_window);
    (*env)->ReleaseStringUTFChars(env, filePath, file_name);
    again = 0;
    release = 0;
    free(s);
    LOGE(TAG, "video release...");
    return ret;
}

VIDEO_PLAYER_FUNC(void, again, jint position) {
    again = 1;
    pos = position;
}

VIDEO_PLAYER_FUNC(void, release) {
    release = 1;
}

VIDEO_PLAYER_FUNC(void, playAudio, jboolean play) {
    enable_audio = play;
}