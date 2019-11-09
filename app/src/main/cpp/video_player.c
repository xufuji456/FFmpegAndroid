//
// Created by frank on 2018/2/1.
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
#include "ffmpeg_jni_define.h"

#define TAG "VideoPlayer"

//播放倍率
float play_rate = 1;
//视频总时长
long duration = 0;

VIDEO_PLAYER_FUNC(jint, play, jstring filePath, jobject surface){

    const char * file_name = (*env)->GetStringUTFChars(env, filePath, JNI_FALSE);
    LOGE(TAG, "open file:%s\n", file_name);
    //注册所有组件
    av_register_all();
    //分配上下文
    AVFormatContext * pFormatCtx = avformat_alloc_context();
    //打开视频文件
    if(avformat_open_input(&pFormatCtx, file_name, NULL, NULL)!=0) {
        LOGE(TAG, "Couldn't open file:%s\n", file_name);
        return -1;
    }
    //检索多媒体流信息
    if(avformat_find_stream_info(pFormatCtx, NULL)<0) {
        LOGE(TAG, "Couldn't find stream information.");
        return -1;
    }
    //寻找视频流的第一帧
    int videoStream = -1, i;
    for (i = 0; i < pFormatCtx->nb_streams; i++) {
        if (pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO
           && videoStream < 0) {
            videoStream = i;
        }
    }
    if(videoStream==-1) {
        LOGE(TAG, "couldn't find a video stream.");
        return -1;
    }

    //获取视频总时长
    if (pFormatCtx->duration != AV_NOPTS_VALUE) {
        duration  = (long) (pFormatCtx->duration / AV_TIME_BASE);
        LOGE(TAG, "duration==%ld", duration);
    }

    //获取codec上下文指针
    AVCodecContext  * pCodecCtx = pFormatCtx->streams[videoStream]->codec;
    //寻找视频流的解码器
    AVCodec * pCodec = avcodec_find_decoder(pCodecCtx->codec_id);
    if(pCodec==NULL) {
        LOGE(TAG, "couldn't find Codec.");
        return -1;
    }
    if(avcodec_open2(pCodecCtx, pCodec, NULL) < 0) {
        LOGE(TAG, "Couldn't open codec.");
        return -1;
    }
    // 获取native window
    ANativeWindow* nativeWindow = ANativeWindow_fromSurface(env, surface);
    // 获取视频宽高
    int videoWidth = pCodecCtx->width;
    int videoHeight = pCodecCtx->height;
    // 设置native window的buffer大小,可自动拉伸
    ANativeWindow_setBuffersGeometry(nativeWindow,  videoWidth, videoHeight, WINDOW_FORMAT_RGBA_8888);
    ANativeWindow_Buffer windowBuffer;
    if(avcodec_open2(pCodecCtx, pCodec, NULL)<0) {
        LOGE(TAG, "Couldn't open codec.");
        return -1;
    }
    //申请内存
    AVFrame * pFrame = av_frame_alloc();
    AVFrame * pFrameRGBA = av_frame_alloc();
    if(pFrameRGBA == NULL || pFrame == NULL) {
        LOGE(TAG, "Couldn't allocate video frame.");
        return -1;
    }
    // buffer中数据用于渲染,且格式为RGBA
    int numBytes=av_image_get_buffer_size(AV_PIX_FMT_RGBA, pCodecCtx->width, pCodecCtx->height, 1);

    uint8_t * buffer=(uint8_t *)av_malloc(numBytes*sizeof(uint8_t));
    av_image_fill_arrays(pFrameRGBA->data, pFrameRGBA->linesize, buffer, AV_PIX_FMT_RGBA,
                         pCodecCtx->width, pCodecCtx->height, 1);

    // 由于解码出来的帧格式不是RGBA的,在渲染之前需要进行格式转换
    struct SwsContext *sws_ctx = sws_getContext(pCodecCtx->width,
                             pCodecCtx->height,
                             pCodecCtx->pix_fmt,
                             pCodecCtx->width,
                             pCodecCtx->height,
                             AV_PIX_FMT_RGBA,
                             SWS_BILINEAR,
                             NULL,
                             NULL,
                             NULL);

    int frameFinished;
    AVPacket packet;

    while(av_read_frame(pFormatCtx, &packet)>=0) {
        //判断是否为视频流
        if(packet.stream_index==videoStream) {
            //对该帧进行解码
            avcodec_decode_video2(pCodecCtx, pFrame, &frameFinished, &packet);
            if (frameFinished) {
                // lock native window
                ANativeWindow_lock(nativeWindow, &windowBuffer, 0);
                // 格式转换
                sws_scale(sws_ctx, (uint8_t const * const *)pFrame->data,
                          pFrame->linesize, 0, pCodecCtx->height,
                          pFrameRGBA->data, pFrameRGBA->linesize);
                // 获取stride
                uint8_t * dst = windowBuffer.bits;
                int dstStride = windowBuffer.stride * 4;
                uint8_t * src = pFrameRGBA->data[0];
                int srcStride = pFrameRGBA->linesize[0];
                // 由于window的stride和帧的stride不同,因此需要逐行复制
                int h;
                for (h = 0; h < videoHeight; h++) {
                    memcpy(dst + h * dstStride, src + h * srcStride, (size_t) srcStride);
                }
                ANativeWindow_unlockAndPost(nativeWindow);
            }
            //延迟等待
            usleep((unsigned long) (1000 * 40 * play_rate));
        }
        av_packet_unref(&packet);
    }
    //释放内存以及关闭文件
    av_free(buffer);
    av_free(pFrameRGBA);
    av_free(pFrame);
    avcodec_close(pCodecCtx);
    avformat_close_input(&pFormatCtx);
    return 0;
}

//设置播放速率
VIDEO_PLAYER_FUNC(void, setPlayRate, jfloat playRate){
        play_rate = playRate;
}

//获取视频总时长
VIDEO_PLAYER_FUNC(jint, getDuration){
        return duration;
}