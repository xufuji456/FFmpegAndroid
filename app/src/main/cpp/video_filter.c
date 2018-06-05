//
// Created by frank on 2018/6/4.
//
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"
#include <android/native_window.h>
#include <android/native_window_jni.h>
#include <stdio.h>
#include <unistd.h>
#include <libavutil/imgutils.h>
#include <android/log.h>
#include <libavfilter/buffersink.h>
#include <libavfilter/buffersrc.h>
#include <libavfilter/avfiltergraph.h>
#include <libavutil/opt.h>

#define TAG "VideoFilter"
#define LOGI(FORMAT,...) __android_log_print(ANDROID_LOG_INFO, TAG, FORMAT, ##__VA_ARGS__);
#define LOGE(FORMAT,...) __android_log_print(ANDROID_LOG_ERROR, TAG, FORMAT, ##__VA_ARGS__);

AVFilterContext *buffersink_ctx;

AVFilterContext *buffersrc_ctx;

AVFilterGraph *filter_graph;

int video_stream_index = -1;

AVFormatContext *pFormatCtx;

AVCodecContext *pCodecCtx;

ANativeWindow_Buffer windowBuffer;

ANativeWindow* nativeWindow;

AVFrame *pFrame;

AVFrame *pFrameRGBA;

AVFrame *filter_frame;

uint8_t *buffer;

struct SwsContext *sws_ctx;

int is_playing;

int again;

int release;

/**
 * colorbalance:
    rs/gs/bs
    Adjust red, green and blue shadows (darkest pixels).
    rm/gm/bm
    Adjust red, green and blue midtones (medium pixels).
    rh/gh/bh
    Adjust red, green and blue highlights (brightest pixels).
 */

/**
 * Draw text
 * Draw a text string or text from a specified file on top of a video, using the libfreetype library.
 * To enable compilation of this filter, you need to configure FFmpeg with --enable-libfreetype.
 * To enable default font fallback and the font option you need to configure FFmpeg with --enable-libfontconfig.
 * To enable the text_shaping option, you need to configure FFmpeg with --enable-libfribidi.
 * Video center: drawtext="fontsize=30:fontfile=FreeSerif.ttf:text='hello world':x=(w-text_w)/2:y=(h-text_h)/2"
 */

//const char *filter_descr = "lutyuv='u=128:v=128'";//黑白
//const char *filter_descr = "hue='h=60:s=-3'";//hue滤镜-->Set the hue to 60 degrees and the saturation to -3
//const char *filter_descr = "vflip";//上下反序
//const char *filter_descr = "hflip";//左右反序
//const char *filter_descr = "rotate=90";//旋转90°
//const char *filter_descr = "colorbalance=bs=0.3";//添加蓝色背景
//const char *filter_descr = "drawbox=x=100:y=100:w=100:h=100:color=pink@0.5'";//绘制矩形
//const char *filter_descr = "drawgrid=w=iw/3:h=ih/3:t=2:c=white@0.5";//九宫格分割
//const char *filter_descr = "edgedetect=low=0.1:high=0.4";//边缘检测
//const char *filter_descr = "lutrgb='r=0:g=0'";//去掉红色、绿色分量，只保留蓝色
//const char *filter_descr = "noise=alls=20:allf=t+u";//添加噪声
//const char *filter_descr = "vignette='PI/4+random(1)*PI/50':eval=frame";//闪烁装饰-->Make a flickering vignetting

//const char *filter_descr = "gblur=sigma=0.5:steps=1:planes=1:sigmaV=1";//高斯模糊
//const char *filter_descr = "drawtext=fontfile='app/src/main/cpp/arial.ttf':fontcolor=green:fontsize=30:text='Hello world'";//绘制文字
//const char *filter_descr = "movie=my_logo.png[wm];[in][wm]overlay=5:5[out]";//添加图片水印

//初始化滤波器
int init_filters(const char *filters_descr) {
    char args[512];
    int ret = 0;
    AVFilter *buffersrc  = avfilter_get_by_name("buffer");
    AVFilter *buffersink = avfilter_get_by_name("buffersink");
    AVFilterInOut *outputs = avfilter_inout_alloc();
    AVFilterInOut *inputs  = avfilter_inout_alloc();
    AVRational time_base = pFormatCtx->streams[video_stream_index]->time_base;
    enum AVPixelFormat pix_fmts[] = { AV_PIX_FMT_YUV420P, AV_PIX_FMT_NONE };

    filter_graph = avfilter_graph_alloc();
    if (!outputs || !inputs || !filter_graph) {
        ret = AVERROR(ENOMEM);
        goto end;
    }

    /* buffer video source: the decoded frames from the decoder will be inserted here. */
    snprintf(args, sizeof(args),
             "video_size=%dx%d:pix_fmt=%d:time_base=%d/%d:pixel_aspect=%d/%d",
             pCodecCtx->width, pCodecCtx->height, pCodecCtx->pix_fmt,
             time_base.num, time_base.den,
             pCodecCtx->sample_aspect_ratio.num, pCodecCtx->sample_aspect_ratio.den);

    ret = avfilter_graph_create_filter(&buffersrc_ctx, buffersrc, "in",
                                       args, NULL, filter_graph);
    if (ret < 0) {
        LOGE("Cannot create buffer source\n");
        goto end;
    }

    /* buffer video sink: to terminate the filter chain. */
    ret = avfilter_graph_create_filter(&buffersink_ctx, buffersink, "out",
                                       NULL, NULL, filter_graph);
    if (ret < 0) {
        LOGE("Cannot create buffer sink\n");
        goto end;
    }

    ret = av_opt_set_int_list(buffersink_ctx, "pix_fmts", pix_fmts,
                              AV_PIX_FMT_NONE, AV_OPT_SEARCH_CHILDREN);
    if (ret < 0) {
        LOGE("Cannot set output pixel format\n");
        goto end;
    }

    /*
     * The buffer source output must be connected to the input pad of
     * the first filter described by filters_descr; since the first
     * filter input label is not specified, it is set to "in" by
     * default.
     */
    outputs->name       = av_strdup("in");
    outputs->filter_ctx = buffersrc_ctx;
    outputs->pad_idx    = 0;
    outputs->next       = NULL;

    /*
     * The buffer sink input must be connected to the output pad of
     * the last filter described by filters_descr; since the last
     * filter output label is not specified, it is set to "out" by
     * default.
     */
    inputs->name       = av_strdup("out");
    inputs->filter_ctx = buffersink_ctx;
    inputs->pad_idx    = 0;
    inputs->next       = NULL;

    if ((ret = avfilter_graph_parse_ptr(filter_graph, filters_descr,
                                        &inputs, &outputs, NULL)) < 0)
        goto end;

    if ((ret = avfilter_graph_config(filter_graph, NULL)) < 0)
        goto end;

    end:
    avfilter_inout_free(&inputs);
    avfilter_inout_free(&outputs);

    return ret;
}

int open_input(JNIEnv * env, const char* file_name, jobject surface){
    LOGI("open file:%s\n", file_name);
    //注册所有组件
    av_register_all();
    //分配上下文
    pFormatCtx = avformat_alloc_context();
    //打开视频文件
    if(avformat_open_input(&pFormatCtx, file_name, NULL, NULL)!=0) {
        LOGE("Couldn't open file:%s\n", file_name);
        return -1;
    }
    //检索多媒体流信息
    if(avformat_find_stream_info(pFormatCtx, NULL)<0) {
        LOGE("Couldn't find stream information.");
        return -1;
    }
    //寻找视频流的第一帧
    int i;
    for (i = 0; i < pFormatCtx->nb_streams; i++) {
        if (pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO
            && video_stream_index < 0) {
            video_stream_index = i;
        }
    }
    if(video_stream_index == -1) {
        LOGE("couldn't find a video stream.");
        return -1;
    }

    //获取codec上下文指针
    pCodecCtx = pFormatCtx->streams[video_stream_index]->codec;
    //寻找视频流的解码器
    AVCodec * pCodec = avcodec_find_decoder(pCodecCtx->codec_id);
    if(pCodec==NULL) {
        LOGE("couldn't find Codec.");
        return -1;
    }
    if(avcodec_open2(pCodecCtx, pCodec, NULL) < 0) {
        LOGE("Couldn't open codec.");
        return -1;
    }
    // 获取native window
    nativeWindow = ANativeWindow_fromSurface(env, surface);

    // 设置native window的buffer大小,可自动拉伸
    ANativeWindow_setBuffersGeometry(nativeWindow,  pCodecCtx->width, pCodecCtx->height, WINDOW_FORMAT_RGBA_8888);
    //申请内存
    pFrame = av_frame_alloc();
    pFrameRGBA = av_frame_alloc();
    if(pFrameRGBA == NULL || pFrame == NULL) {
        LOGE("Couldn't allocate video frame.");
        return -1;
    }
    // buffer中数据用于渲染,且格式为RGBA
    int numBytes=av_image_get_buffer_size(AV_PIX_FMT_RGBA, pCodecCtx->width, pCodecCtx->height, 1);

    buffer=(uint8_t *)av_malloc(numBytes*sizeof(uint8_t));
    av_image_fill_arrays(pFrameRGBA->data, pFrameRGBA->linesize, buffer, AV_PIX_FMT_RGBA,
                         pCodecCtx->width, pCodecCtx->height, 1);
    // 由于解码出来的帧格式不是RGBA的,在渲染之前需要进行格式转换
    sws_ctx = sws_getContext(pCodecCtx->width,
                                                pCodecCtx->height,
                                                pCodecCtx->pix_fmt,
                                                pCodecCtx->width,
                                                pCodecCtx->height,
                                                AV_PIX_FMT_RGBA,
                                                SWS_BILINEAR,
                                                NULL,
                                                NULL,
                                                NULL);

    return 0;
}

JNIEXPORT jint JNICALL Java_com_frank_ffmpeg_VideoPlayer_filter
        (JNIEnv * env, jclass clazz, jstring filePath, jobject surface, jstring filterDescr){

    int ret;
    const char * file_name = (*env)->GetStringUTFChars(env, filePath, JNI_FALSE);
    const char *filter_descr = (*env)->GetStringUTFChars(env, filterDescr, JNI_FALSE);
    //打开输入文件
    if(!is_playing){
        LOGI("open_input...");
        if((ret = open_input(env, file_name, surface) < 0)){
            LOGE("Couldn't allocate video frame.");
            goto end;
        }
        //注册滤波器
        avfilter_register_all();
        filter_frame = av_frame_alloc();
        if(filter_frame == NULL) {
            LOGE("Couldn't allocate filter frame.");
            ret = -1;
            goto end;
        }
    }

    //初始化滤波器
    if ((ret = init_filters(filter_descr)) < 0){
        LOGE("init_filter error, ret=%d\n", ret);
        goto end;
    }

    is_playing = 1;
    int frameFinished;
    AVPacket packet;

    while(av_read_frame(pFormatCtx, &packet)>=0 && !release) {
        //切换滤波器，退出当初播放
        if(again){
            goto again;
        }
        //判断是否为视频流
        if(packet.stream_index == video_stream_index) {
            //对该帧进行解码
            avcodec_decode_video2(pCodecCtx, pFrame, &frameFinished, &packet);

            if (frameFinished) {
                //把解码后视频帧添加到filter_graph
                if (av_buffersrc_add_frame_flags(buffersrc_ctx, pFrame, AV_BUFFERSRC_FLAG_KEEP_REF) < 0) {
                    LOGE("Error while feeding the filter_graph\n");
                    break;
                }
                //把滤波后的视频帧从filter graph取出来
                ret = av_buffersink_get_frame(buffersink_ctx, filter_frame);
                if (ret >= 0){
                    // lock native window
                    ANativeWindow_lock(nativeWindow, &windowBuffer, 0);
                    // 格式转换
                    sws_scale(sws_ctx, (uint8_t const * const *)filter_frame->data,
                              filter_frame->linesize, 0, pCodecCtx->height,
                              pFrameRGBA->data, pFrameRGBA->linesize);
                    // 获取stride
                    uint8_t * dst = windowBuffer.bits;
                    int dstStride = windowBuffer.stride * 4;
                    uint8_t * src = pFrameRGBA->data[0];
                    int srcStride = pFrameRGBA->linesize[0];
                    // 由于window的stride和帧的stride不同,因此需要逐行复制
                    int h;
                    for (h = 0; h < pCodecCtx->height; h++) {
                        memcpy(dst + h * dstStride, src + h * srcStride, (size_t) srcStride);
                    }
                    ANativeWindow_unlockAndPost(nativeWindow);
                }
                av_frame_unref(filter_frame);
            }
            av_frame_unref(pFrame);
            //延迟等待
            usleep((unsigned long) (1000 * 40));
        }
        av_packet_unref(&packet);
    }
    end:
    is_playing = 0;
    //释放内存以及关闭文件
    av_free(buffer);
    av_free(pFrameRGBA);
    av_free(filter_frame);
    av_free(pFrame);
    avcodec_close(pCodecCtx);
    avformat_close_input(&pFormatCtx);
    avfilter_free(buffersrc_ctx);
    avfilter_free(buffersink_ctx);
    avfilter_graph_free(&filter_graph);
    (*env)->ReleaseStringUTFChars(env, filePath, file_name);
    (*env)->ReleaseStringUTFChars(env, filterDescr, filter_descr);
    LOGE("do release...");
    again:
    again = 0;
    LOGE("play again...");
    return ret;
}

JNIEXPORT void JNICALL Java_com_frank_ffmpeg_VideoPlayer_again(JNIEnv * env, jclass clazz) {
    again = 1;
}

JNIEXPORT void JNICALL Java_com_frank_ffmpeg_VideoPlayer_release(JNIEnv * env, jclass clazz) {
    release = 1;
}