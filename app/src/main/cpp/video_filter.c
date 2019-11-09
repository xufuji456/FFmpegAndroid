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
#include "libswresample/swresample.h"
#include "ffmpeg_jni_define.h"

#define TAG "VideoFilter"

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

#define MAX_AUDIO_FRAME_SIZE 48000 * 4
jmethodID audio_track_write_mid;
uint8_t *out_buffer;
jobject audio_track;
SwrContext *audio_swr_ctx;
int out_channel_nb;
enum AVSampleFormat out_sample_fmt;
int audio_stream_index = -1;
int got_frame;
AVCodecContext *audioCodecCtx;
jboolean playAudio = JNI_TRUE;

//const char *filter_descr = "lutyuv='u=128:v=128'";//黑白
//const char *filter_descr = "hue='h=60:s=-3'";//hue滤镜
//const char *filter_descr = "vflip";//上下反序
//const char *filter_descr = "hflip";//左右反序
//const char *filter_descr = "rotate=90";//旋转90°
//const char *filter_descr = "colorbalance=bs=0.3";//添加蓝色背景
//const char *filter_descr = "drawbox=x=100:y=100:w=100:h=100:color=pink@0.5'";//绘制矩形
//const char *filter_descr = "drawgrid=w=iw/3:h=ih/3:t=2:c=white@0.5";//九宫格分割
//const char *filter_descr = "edgedetect=low=0.1:high=0.4";//边缘检测
//const char *filter_descr = "lutrgb='r=0:g=0'";//去掉红色、绿色分量，只保留蓝色
//const char *filter_descr = "noise=alls=20:allf=t+u";//添加噪声
//const char *filter_descr = "vignette='PI/4+random(1)*PI/50':eval=frame";//闪烁装饰
//const char *filter_descr = "gblur=sigma=0.5:steps=1:planes=1:sigmaV=1";//高斯模糊
//const char *filter_descr = "drawtext=fontfile='arial.ttf':fontcolor=green:fontsize=30:text='Hello world'";//绘制文字
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

    outputs->name       = av_strdup("in");
    outputs->filter_ctx = buffersrc_ctx;
    outputs->pad_idx    = 0;
    outputs->next       = NULL;

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

//初始化视频解码器与播放器
int open_input(JNIEnv * env, const char* file_name, jobject surface){
    LOGI(TAG, "open file:%s\n", file_name);
    //注册所有组件
    av_register_all();
    //分配上下文
    pFormatCtx = avformat_alloc_context();
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
    int i;
    for (i = 0; i < pFormatCtx->nb_streams; i++) {
        if (pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO
            && video_stream_index < 0) {
            video_stream_index = i;
        }
    }
    if(video_stream_index == -1) {
        LOGE(TAG, "couldn't find a video stream.");
        return -1;
    }

    //获取codec上下文指针
    pCodecCtx = pFormatCtx->streams[video_stream_index]->codec;
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
    nativeWindow = ANativeWindow_fromSurface(env, surface);

    // 设置native window的buffer大小,可自动拉伸
    ANativeWindow_setBuffersGeometry(nativeWindow,  pCodecCtx->width, pCodecCtx->height, WINDOW_FORMAT_RGBA_8888);
    //申请内存
    pFrame = av_frame_alloc();
    pFrameRGBA = av_frame_alloc();
    if(pFrameRGBA == NULL || pFrame == NULL) {
        LOGE(TAG, "Couldn't allocate video frame.");
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

//初始化音频解码器与播放器
int init_audio(JNIEnv * env, jclass jthiz){
    //获取音频流索引位置
    int i;
    for(i=0; i < pFormatCtx->nb_streams;i++){
        if(pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO){
            audio_stream_index = i;
            break;
        }
    }

    //获取音频解码器
    audioCodecCtx = pFormatCtx->streams[audio_stream_index]->codec;
    AVCodec *codec = avcodec_find_decoder(audioCodecCtx->codec_id);
    if(codec == NULL){
        LOGE(TAG, "无法获取音频解码器");
        return -1;
    }
    //打开音频解码器
    if(avcodec_open2(audioCodecCtx,codec,NULL) < 0){
        LOGE(TAG, "无法打开音频解码器");
        return -1;
    }
    //frame->16bit 44100 PCM 统一音频采样格式与采样率
    audio_swr_ctx = swr_alloc();

    //输入的采样格式
    enum AVSampleFormat in_sample_fmt = audioCodecCtx->sample_fmt;
    //输出采样格式16bit PCM
    out_sample_fmt = AV_SAMPLE_FMT_S16;
    //输入采样率
    int in_sample_rate = audioCodecCtx->sample_rate;
    //输出采样率
    int out_sample_rate = in_sample_rate;
    //声道布局（2个声道，默认立体声stereo）
    uint64_t in_ch_layout = audioCodecCtx->channel_layout;
    //输出的声道布局（立体声）
    uint64_t out_ch_layout = AV_CH_LAYOUT_STEREO;

    swr_alloc_set_opts(audio_swr_ctx,
                       out_ch_layout,out_sample_fmt,out_sample_rate,
                       in_ch_layout,in_sample_fmt,in_sample_rate,
                       0, NULL);
    swr_init(audio_swr_ctx);

    //输出的声道个数
    out_channel_nb = av_get_channel_layout_nb_channels(out_ch_layout);

    jclass player_class = (*env)->GetObjectClass(env,jthiz);
    if(!player_class){
        LOGE(TAG, "player_class not found...");
        return -1;
    }
    //AudioTrack对象
    jmethodID audio_track_method = (*env)->GetMethodID(env,player_class,"createAudioTrack","(II)Landroid/media/AudioTrack;");
    if(!audio_track_method){
        LOGE(TAG, "audio_track_method not found...");
        return -1;
    }
    audio_track = (*env)->CallObjectMethod(env,jthiz,audio_track_method,out_sample_rate,out_channel_nb);

    //调用play方法
    jclass audio_track_class = (*env)->GetObjectClass(env,audio_track);
    jmethodID audio_track_play_mid = (*env)->GetMethodID(env,audio_track_class,"play","()V");
    (*env)->CallVoidMethod(env,audio_track,audio_track_play_mid);

    //获取write()方法
    audio_track_write_mid = (*env)->GetMethodID(env,audio_track_class,"write","([BII)I");

    //16bit 44100 PCM 数据
    out_buffer = (uint8_t *)av_malloc(MAX_AUDIO_FRAME_SIZE);
    return 0;
}

int play_audio(JNIEnv * env, AVPacket* packet, AVFrame* frame){
    //解码
    int ret = avcodec_decode_audio4(audioCodecCtx, frame, &got_frame, packet);
    if(ret < 0){
        return ret;
    }
    //解码一帧成功
    if(got_frame > 0){
        //音频格式转换
        swr_convert(audio_swr_ctx, &out_buffer, MAX_AUDIO_FRAME_SIZE,(const uint8_t **)frame->data,frame->nb_samples);
        int out_buffer_size = av_samples_get_buffer_size(NULL, out_channel_nb,
                                                         frame->nb_samples, out_sample_fmt, 1);

        jbyteArray audio_sample_array = (*env)->NewByteArray(env,out_buffer_size);
        jbyte* sample_byte_array = (*env)->GetByteArrayElements(env,audio_sample_array,NULL);
        //拷贝缓冲数据
        memcpy(sample_byte_array, out_buffer, (size_t) out_buffer_size);
        //释放数组
        (*env)->ReleaseByteArrayElements(env,audio_sample_array,sample_byte_array,0);
        //调用AudioTrack的write方法进行播放
        (*env)->CallIntMethod(env,audio_track,audio_track_write_mid,
                              audio_sample_array,0,out_buffer_size);
        //释放局部引用
        (*env)->DeleteLocalRef(env,audio_sample_array);
        usleep(1000);//1000 * 16
    }
    return 0;
}

VIDEO_PLAYER_FUNC(jint, filter, jstring filePath, jobject surface, jstring filterDescr){

    int ret;
    const char * file_name = (*env)->GetStringUTFChars(env, filePath, JNI_FALSE);
    const char *filter_descr = (*env)->GetStringUTFChars(env, filterDescr, JNI_FALSE);
    //打开输入文件
    if(!is_playing){
        LOGI(TAG, "open_input...");
        if((ret = open_input(env, file_name, surface)) < 0){
            LOGE(TAG, "Couldn't allocate video frame.");
            goto end;
        }
        //注册滤波器
        avfilter_register_all();
        filter_frame = av_frame_alloc();
        if(filter_frame == NULL) {
            LOGE(TAG, "Couldn't allocate filter frame.");
            ret = -1;
            goto end;
        }
        //初始化音频解码器
        if ((ret = init_audio(env, thiz)) < 0){
            LOGE(TAG, "Couldn't init_audio.");
            goto end;
        }

    }

    //初始化滤波器
    if ((ret = init_filters(filter_descr)) < 0){
        LOGE(TAG, "init_filter error, ret=%d\n", ret);
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
                    LOGE(TAG, "Error while feeding the filter_graph\n");
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
            //延迟等待
            if (!playAudio){
                usleep((unsigned long) (1000 * 40));//1000 * 40
            }
        } else if(packet.stream_index == audio_stream_index){//音频帧
            if (playAudio){
                play_audio(env, &packet, pFrame);
            }
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
    avcodec_close(audioCodecCtx);
    free(buffer);
    free(sws_ctx);
    free(&windowBuffer);
    free(out_buffer);
    free(audio_swr_ctx);
    free(audio_track);
    free(audio_track_write_mid);
    ANativeWindow_release(nativeWindow);
    (*env)->ReleaseStringUTFChars(env, filePath, file_name);
    (*env)->ReleaseStringUTFChars(env, filterDescr, filter_descr);
    LOGE(TAG, "do release...");
    again:
    again = 0;
    LOGE(TAG, "play again...");
    return ret;
}

VIDEO_PLAYER_FUNC(void, again) {
    again = 1;
}

VIDEO_PLAYER_FUNC(void, release) {
    release = 1;
}

VIDEO_PLAYER_FUNC(void, playAudio, jboolean play_audio) {
    playAudio = play_audio;
}