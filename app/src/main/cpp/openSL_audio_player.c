//
// Created by frank on 2018/2/1.
//

#include <jni.h>
#include <string.h>
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"
#include "libswresample/swresample.h"
#include "libavutil/samplefmt.h"
#include <SLES/OpenSLES.h>
#include <SLES/OpenSLES_Android.h>
#include <android/log.h>
#include <libavutil/opt.h>
#include "ffmpeg_jni_define.h"

#define TAG "OpenSLPlayer"

//引擎接口
SLObjectItf engineObject = NULL;
SLEngineItf engineEngine;

//输出混音器接口
SLObjectItf outputMixObject = NULL;
SLEnvironmentalReverbItf outputMixEnvironmentalReverb = NULL;

//缓冲播放器接口
SLObjectItf bqPlayerObject = NULL;
SLPlayItf bqPlayerPlay;
SLAndroidSimpleBufferQueueItf bqPlayerBufferQueue;
SLEffectSendItf bqPlayerEffectSend;
SLVolumeItf bqPlayerVolume;

//音效设置
const SLEnvironmentalReverbSettings reverbSettings = SL_I3DL2_ENVIRONMENT_PRESET_STONECORRIDOR;
void *buffer;
size_t bufferSize;
uint8_t *outputBuffer;
size_t outputBufferSize;

//FFmpeg相关
AVPacket packet;
int audioStream;
AVFrame *aFrame;
SwrContext *swr;
AVFormatContext *aFormatCtx;
AVCodecContext *aCodecCtx;
int frame_count = 0;

int createAudioPlayer(int *rate, int *channel, const char *file_name) ;

// 释放相关资源
int releaseAudioPlayer();

// 获取PCM数据, 自动回调获取
int getPCM(void **pcm, size_t *pcmSize) ;

//播放回调方法
void bqPlayerCallback(SLAndroidSimpleBufferQueueItf bufferQueueItf, void *context) {
    bufferSize = 0;
    getPCM(&buffer, &bufferSize);
    //如果buffer不为空，入待播放队列
    if (NULL != buffer && 0 != bufferSize) {
        SLresult result;
        result = (*bqPlayerBufferQueue)->Enqueue(bqPlayerBufferQueue, buffer, bufferSize);
        if(result < 0){
            LOGE(TAG, "Enqueue error...");
        } else{
            LOGI(TAG, "decode frame count=%d", frame_count++);
        }
    }
}

//创建OpenSLES引擎
void createEngine() {
    SLresult result;
    //创建引擎
    result = slCreateEngine(&engineObject, 0, NULL, 0, NULL, NULL);
    LOGI(TAG, "slCreateEngine=%d", result);
    result = (*engineObject)->Realize(engineObject, SL_BOOLEAN_FALSE);
    LOGI(TAG, "engineObject->Realize=%d", result);
    //获取引擎接口
    result = (*engineObject)->GetInterface(engineObject, SL_IID_ENGINE, &engineEngine);
    LOGI(TAG, "engineObject->GetInterface=%d", result);
    //创建输出混音器
    result = (*engineEngine)->CreateOutputMix(engineEngine, &outputMixObject, 0, 0, 0);
    LOGI(TAG, "CreateOutputMix=%d", result);
    //关联输出混音器
    result = (*outputMixObject)->Realize(outputMixObject, SL_BOOLEAN_FALSE);
    LOGI(TAG, "outputMixObject->Realize=%d", result);
    //获取reverb接口
    result = (*outputMixObject)->GetInterface(outputMixObject, SL_IID_ENVIRONMENTALREVERB,
                                              &outputMixEnvironmentalReverb);
    LOGI(TAG, "outputMixObject->GetInterface=%d", result);
    if (SL_RESULT_SUCCESS == result) {
        result = (*outputMixEnvironmentalReverb)->SetEnvironmentalReverbProperties(
                outputMixEnvironmentalReverb, &reverbSettings);
    }
    LOGI(TAG, "SetEnvironmentalReverbProperties=%d", result);
}


//创建带有缓冲队列的音频播放器
void createBufferQueueAudioPlayer(int rate, int channel, int bitsPerSample) {
    SLresult result;

    //配置音频源
    SLDataLocator_AndroidSimpleBufferQueue buffer_queue = {SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE, 2};
    SLDataFormat_PCM format_pcm;
    format_pcm.formatType = SL_DATAFORMAT_PCM;
    format_pcm.numChannels = (SLuint32) channel;
    format_pcm.bitsPerSample = (SLuint32) bitsPerSample;
    format_pcm.samplesPerSec = (SLuint32) (rate * 1000);
    format_pcm.containerSize = 16;
    if (channel == 2)
        format_pcm.channelMask = SL_SPEAKER_FRONT_LEFT | SL_SPEAKER_FRONT_RIGHT;
    else
        format_pcm.channelMask = SL_SPEAKER_FRONT_CENTER;
    format_pcm.endianness = SL_BYTEORDER_LITTLEENDIAN;
    SLDataSource audioSrc = {&buffer_queue, &format_pcm};

    //配置音频池
    SLDataLocator_OutputMix loc_outmix = {SL_DATALOCATOR_OUTPUTMIX, outputMixObject};
    SLDataSink audioSnk = {&loc_outmix, NULL};

    //创建音频播放器
    const SLInterfaceID ids[3] = {SL_IID_BUFFERQUEUE, SL_IID_EFFECTSEND, SL_IID_VOLUME};
    const SLboolean req[3] = {SL_BOOLEAN_TRUE, SL_BOOLEAN_TRUE, SL_BOOLEAN_TRUE};
    result = (*engineEngine)->CreateAudioPlayer(engineEngine, &bqPlayerObject, &audioSrc, &audioSnk,
                                                3, ids, req);
    LOGI(TAG, "CreateAudioPlayer=%d", result);

    //关联播放器
    result = (*bqPlayerObject)->Realize(bqPlayerObject, SL_BOOLEAN_FALSE);
    LOGI(TAG, "bqPlayerObject Realize=%d", result);

    //获取播放接口
    result = (*bqPlayerObject)->GetInterface(bqPlayerObject, SL_IID_PLAY, &bqPlayerPlay);
    LOGI(TAG, "GetInterface bqPlayerPlay=%d", result);

    //获取缓冲队列接口
    result = (*bqPlayerObject)->GetInterface(bqPlayerObject, SL_IID_BUFFERQUEUE,
                                             &bqPlayerBufferQueue);
    LOGI(TAG, "GetInterface bqPlayerBufferQueue=%d", result);

    //注册缓冲队列回调
    result = (*bqPlayerBufferQueue)->RegisterCallback(bqPlayerBufferQueue, bqPlayerCallback, NULL);
    LOGI(TAG, "RegisterCallback=%d", result);

    //获取音效接口
    result = (*bqPlayerObject)->GetInterface(bqPlayerObject, SL_IID_EFFECTSEND,
                                             &bqPlayerEffectSend);
    LOGI(TAG, "GetInterface effect=%d", result);

    //获取音量接口
    result = (*bqPlayerObject)->GetInterface(bqPlayerObject, SL_IID_VOLUME, &bqPlayerVolume);
    LOGI(TAG, "GetInterface volume=%d", result);

    //开始播放音乐
    result = (*bqPlayerPlay)->SetPlayState(bqPlayerPlay, SL_PLAYSTATE_PLAYING);
    LOGI(TAG, "SetPlayState=%d", result);
}

int createAudioPlayer(int *rate, int *channel, const char *file_name) {
    //注册相关组件
    av_register_all();
    aFormatCtx = avformat_alloc_context();

    //打开音频文件
    if (avformat_open_input(&aFormatCtx, file_name, NULL, NULL) != 0) {
        LOGE(TAG, "Couldn't open file:%s\n", file_name);
        return -1; // Couldn't open file
    }

    //寻找stream信息
    if (avformat_find_stream_info(aFormatCtx, NULL) < 0) {
        LOGE(TAG, "Couldn't find stream information.");
        return -1;
    }

    //寻找音频stream
    int i;
    audioStream = -1;
    for (i = 0; i < aFormatCtx->nb_streams; i++) {
        if (aFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO &&
            audioStream < 0) {
            audioStream = i;
        }
    }
    if (audioStream == -1) {
        LOGE(TAG, "Couldn't find audio stream!");
        return -1;
    }
    //获取解码器context
    aCodecCtx = aFormatCtx->streams[audioStream]->codec;
    //寻找音频解码器
    AVCodec *aCodec = avcodec_find_decoder(aCodecCtx->codec_id);
    if (!aCodec) {
        fprintf(stderr, "Unsupported codec!\n");
        return -1;
    }
    //打开解码器
    if (avcodec_open2(aCodecCtx, aCodec, NULL) < 0) {
        LOGE(TAG, "Could not open codec.");
        return -1;
    }
    aFrame = av_frame_alloc();
    // 设置格式转换
    swr = swr_alloc();
    av_opt_set_int(swr, "in_channel_layout",  aCodecCtx->channel_layout, 0);
    av_opt_set_int(swr, "out_channel_layout", aCodecCtx->channel_layout,  0);
    av_opt_set_int(swr, "in_sample_rate",     aCodecCtx->sample_rate, 0);
    av_opt_set_int(swr, "out_sample_rate",    aCodecCtx->sample_rate, 0);
    av_opt_set_sample_fmt(swr, "in_sample_fmt",  aCodecCtx->sample_fmt, 0);
    av_opt_set_sample_fmt(swr, "out_sample_fmt", AV_SAMPLE_FMT_S16,  0);
    swr_init(swr);

    // 分配PCM数据缓存
    outputBufferSize = 8196;
    outputBuffer = (uint8_t *) malloc(sizeof(uint8_t) * outputBufferSize);
    // 返回sample rate和channels
    *rate = aCodecCtx->sample_rate;
    *channel = aCodecCtx->channels;
    return 0;
}

// 获取PCM数据, 自动回调获取
int getPCM(void **pcm, size_t *pcmSize) {
    while (av_read_frame(aFormatCtx, &packet) >= 0) {
        int frameFinished = 0;
        //音频流
        if (packet.stream_index == audioStream) {
            avcodec_decode_audio4(aCodecCtx, aFrame, &frameFinished, &packet);
            //解码完一帧数据
            if (frameFinished) {
                // data_size为音频数据所占的字节数
                int data_size = av_samples_get_buffer_size(
                        aFrame->linesize, aCodecCtx->channels,
                        aFrame->nb_samples, aCodecCtx->sample_fmt, 1);

                if (data_size > outputBufferSize) {
                    outputBufferSize = (size_t) data_size;
                    outputBuffer = (uint8_t *) realloc(outputBuffer, sizeof(uint8_t) * outputBufferSize);
                }

                // 音频格式转换
                swr_convert(swr, &outputBuffer, aFrame->nb_samples,
                            (uint8_t const **) (aFrame->extended_data),
                            aFrame->nb_samples);

                // 返回pcm数据
                *pcm = outputBuffer;
                *pcmSize = (size_t) data_size;
                return 0;
            }
        }
    }
    return -1;
}

// 释放相关资源
int releaseAudioPlayer() {
    av_packet_unref(&packet);
    av_free(outputBuffer);
    av_free(aFrame);
    avcodec_close(aCodecCtx);
    avformat_close_input(&aFormatCtx);
    return 0;
}

AUDIO_PLAYER_FUNC(void, playAudio, jstring filePath) {

    int rate, channel;
    const char *file_name = (*env)->GetStringUTFChars(env, filePath, NULL);
    LOGI(TAG, "file_name=%s", file_name);

    // 创建音频解码器
    createAudioPlayer(&rate, &channel, file_name);

    // 创建播放引擎
    createEngine();

    // 创建缓冲队列音频播放器
    createBufferQueueAudioPlayer(rate, channel, SL_PCMSAMPLEFORMAT_FIXED_16);

    // 启动音频播放
    bqPlayerCallback(bqPlayerBufferQueue, NULL);
}

//停止播放，释放相关资源
AUDIO_PLAYER_FUNC(void, stop) {
    if (bqPlayerObject != NULL) {
        (*bqPlayerObject)->Destroy(bqPlayerObject);
        bqPlayerObject = NULL;
        bqPlayerPlay = NULL;
        bqPlayerBufferQueue = NULL;
        bqPlayerEffectSend = NULL;
        bqPlayerVolume = NULL;
    }

    if (outputMixObject != NULL) {
        (*outputMixObject)->Destroy(outputMixObject);
        outputMixObject = NULL;
        outputMixEnvironmentalReverb = NULL;
    }

    if (engineObject != NULL) {
        (*engineObject)->Destroy(engineObject);
        engineObject = NULL;
        engineEngine = NULL;
    }

    // 释放解码器相关资源
    releaseAudioPlayer();
}
