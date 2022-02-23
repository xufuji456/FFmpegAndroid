//
// Created by frank on 2018/2/1.
//

#include <jni.h>
#include <cstring>
#include <SLES/OpenSLES.h>
#include <SLES/OpenSLES_Android.h>

#ifdef __cplusplus
extern "C" {
#endif
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"
#include "libswresample/swresample.h"
#include "libavutil/samplefmt.h"
#include "libavutil/opt.h"
#include "ffmpeg_jni_define.h"
#ifdef __cplusplus
}
#endif

#define TAG "OpenSLPlayer"

//object of engine
SLObjectItf engineObject = nullptr;
SLEngineItf engineEngine = nullptr;

//object of mixer
SLObjectItf outputMixObject = nullptr;
SLEnvironmentalReverbItf outputMixEnvironmentalReverb = nullptr;

//object of buffer
SLPlayItf mPlayerPlay = nullptr;
SLVolumeItf mPlayerVolume = nullptr;
SLObjectItf mPlayerObject = nullptr;
SLEffectSendItf mPlayerEffectSend = nullptr;
SLAndroidSimpleBufferQueueItf mPlayerBufferQueue = nullptr;


//audio effect
void *openBuffer;
size_t bufferSize;
uint8_t *outputBuffer;
size_t outputBufferSize;
const SLEnvironmentalReverbSettings reverbSettings = SL_I3DL2_ENVIRONMENT_PRESET_STONECORRIDOR;

AVPacket packet;
int audioStream;
AVFrame *aFrame;
SwrContext *swr;
int frame_count = 0;
AVFormatContext *aFormatCtx;
AVCodecContext *aCodecCtx;

int releaseAudioPlayer();

int getPCMData(void **pcm, size_t *pcmSize);

int createAudioPlayer(int *rate, int *channel, const char *file_name);


void audioCallback(SLAndroidSimpleBufferQueueItf bufferQueueItf, void *context) {
    bufferSize = 0;
    getPCMData(&openBuffer, &bufferSize);
    if (nullptr != openBuffer && 0 != bufferSize) {
        SLresult result;
        result = (*mPlayerBufferQueue)->Enqueue(mPlayerBufferQueue, openBuffer, bufferSize);
        if (result < 0) {
            LOGE(TAG, "Enqueue error...");
        } else {
            LOGI(TAG, "decode frame count=%d", frame_count++);
        }
    }
}

//create the engine of OpenSLES
int createEngine() {
    SLresult result;
    result = slCreateEngine(&engineObject, 0, nullptr,
                            0, nullptr, nullptr);
    if (result != SL_RESULT_SUCCESS) {
        LOGE(TAG, "slCreateEngine error=%d", result);
        return result;
    }
    result = (*engineObject)->Realize(engineObject, SL_BOOLEAN_FALSE);
    if (result != SL_RESULT_SUCCESS) {
        LOGE(TAG, "engineObject->Realize error=%d", result);
        return result;
    }
    result = (*engineObject)->GetInterface(engineObject, SL_IID_ENGINE, &engineEngine);
    if (result != SL_RESULT_SUCCESS) {
        LOGE(TAG, "engineObject->GetInterface error=%d", result);
        return result;
    }
    result = (*engineEngine)->CreateOutputMix(engineEngine, &outputMixObject, 0, nullptr, nullptr);
    if (result != SL_RESULT_SUCCESS) {
        LOGE(TAG, "engineEngine->CreateOutputMix error=%d", result);
        return result;
    }
    result = (*outputMixObject)->Realize(outputMixObject, SL_BOOLEAN_FALSE);
    if (result != SL_RESULT_SUCCESS) {
        LOGE(TAG, "outputMixObject->Realize error=%d", result);
        return result;
    }
    result = (*outputMixObject)->GetInterface(outputMixObject, SL_IID_ENVIRONMENTALREVERB,
                                              &outputMixEnvironmentalReverb);
    if (result != SL_RESULT_SUCCESS) {
        LOGE(TAG, "outputMixObject->GetInterface error=%d", result);
        return result;
    }
    result = (*outputMixEnvironmentalReverb)->SetEnvironmentalReverbProperties(
            outputMixEnvironmentalReverb, &reverbSettings);
    return result;
}


int createBufferQueueAudioPlayer(int rate, int channel, int bitsPerSample) {
    SLresult result;

    //config audio source
    SLDataLocator_AndroidSimpleBufferQueue buffer_queue = {SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE,
                                                           2};
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

    SLDataLocator_OutputMix loc_outmix = {SL_DATALOCATOR_OUTPUTMIX, outputMixObject};
    SLDataSink audioSnk = {&loc_outmix, nullptr};

    const SLInterfaceID ids[3] = {SL_IID_BUFFERQUEUE, SL_IID_EFFECTSEND, SL_IID_VOLUME};
    const SLboolean req[3] = {SL_BOOLEAN_TRUE, SL_BOOLEAN_TRUE, SL_BOOLEAN_TRUE};
    result = (*engineEngine)->CreateAudioPlayer(engineEngine, &mPlayerObject, &audioSrc, &audioSnk,
                                                3, ids, req);
    if (result != SL_RESULT_SUCCESS) {
        LOGE(TAG, "outputMixObject->GetInterface error=%d", result);
        return result;
    }
    result = (*mPlayerObject)->Realize(mPlayerObject, SL_BOOLEAN_FALSE);
    if (result != SL_RESULT_SUCCESS) {
        LOGE(TAG, "mPlayerObject->Realize error=%d", result);
        return result;
    }
    (*mPlayerObject)->GetInterface(mPlayerObject, SL_IID_PLAY, &mPlayerPlay);
    (*mPlayerObject)->GetInterface(mPlayerObject, SL_IID_BUFFERQUEUE, &mPlayerBufferQueue);
    result = (*mPlayerBufferQueue)->RegisterCallback(mPlayerBufferQueue, audioCallback, nullptr);
    if (result != SL_RESULT_SUCCESS) {
        LOGE(TAG, "mPlayerBufferQueue->RegisterCallback error=%d", result);
        return result;
    }
    (*mPlayerObject)->GetInterface(mPlayerObject, SL_IID_EFFECTSEND, &mPlayerEffectSend);
    (*mPlayerObject)->GetInterface(mPlayerObject, SL_IID_VOLUME, &mPlayerVolume);
    result = (*mPlayerPlay)->SetPlayState(mPlayerPlay, SL_PLAYSTATE_PLAYING);
    return result;
}

int createAudioPlayer(int *rate, int *channel, const char *file_name) {

    av_register_all();
    aFormatCtx = avformat_alloc_context();

    if (avformat_open_input(&aFormatCtx, file_name, nullptr, nullptr) != 0) {
        LOGE(TAG, "Couldn't open file:%s\n", file_name);
        return -1;
    }

    if (avformat_find_stream_info(aFormatCtx, nullptr) < 0) {
        LOGE(TAG, "Couldn't find stream information.");
        return -1;
    }

    int i;
    audioStream = -1;
    for (i = 0; i < aFormatCtx->nb_streams; i++) {
        if (aFormatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO &&
            audioStream < 0) {
            audioStream = i;
        }
    }
    if (audioStream == -1) {
        LOGE(TAG, "Couldn't find audio stream!");
        return -1;
    }

    aCodecCtx = aFormatCtx->streams[audioStream]->codec;
    AVCodec *aCodec = avcodec_find_decoder(aCodecCtx->codec_id);
    if (!aCodec) {
        fprintf(stderr, "Unsupported codec!\n");
        return -1;
    }
    if (avcodec_open2(aCodecCtx, aCodec, nullptr) < 0) {
        LOGE(TAG, "Could not open codec.");
        return -1;
    }
    aFrame = av_frame_alloc();
    swr = swr_alloc();
    av_opt_set_int(swr, "in_channel_layout", aCodecCtx->channel_layout, 0);
    av_opt_set_int(swr, "out_channel_layout", aCodecCtx->channel_layout, 0);
    av_opt_set_int(swr, "in_sample_rate", aCodecCtx->sample_rate, 0);
    av_opt_set_int(swr, "out_sample_rate", aCodecCtx->sample_rate, 0);
    av_opt_set_sample_fmt(swr, "in_sample_fmt", aCodecCtx->sample_fmt, 0);
    av_opt_set_sample_fmt(swr, "out_sample_fmt", AV_SAMPLE_FMT_S16, 0);
    swr_init(swr);

    outputBufferSize = 8196;
    outputBuffer = (uint8_t *) malloc(sizeof(uint8_t) * outputBufferSize);
    *rate = aCodecCtx->sample_rate;
    *channel = aCodecCtx->channels;
    return 0;
}

int getPCMData(void **pcm, size_t *pcmSize) {
    while (av_read_frame(aFormatCtx, &packet) >= 0) {
        int frameFinished = 0;
        //is audio stream
        if (packet.stream_index == audioStream) {
            avcodec_decode_audio4(aCodecCtx, aFrame, &frameFinished, &packet);
            //decode success
            if (frameFinished) {
                int data_size = av_samples_get_buffer_size(
                        aFrame->linesize, aCodecCtx->channels,
                        aFrame->nb_samples, aCodecCtx->sample_fmt, 1);

                if (data_size > outputBufferSize) {
                    outputBufferSize = (size_t) data_size;
                    outputBuffer = (uint8_t *) realloc(outputBuffer,sizeof(uint8_t) * outputBufferSize);
                }

                swr_convert(swr, &outputBuffer, aFrame->nb_samples,
                            (uint8_t const **) (aFrame->extended_data),
                            aFrame->nb_samples);

                *pcm = outputBuffer;
                *pcmSize = (size_t) data_size;
                return 0;
            }
        }
    }
    return -1;
}

int releaseAudioPlayer() {
    av_packet_unref(&packet);
    av_free(outputBuffer);
    av_free(aFrame);
    avcodec_free_context(&aCodecCtx);
    avformat_close_input(&aFormatCtx);
    return 0;
}

AUDIO_PLAYER_FUNC(void, playAudio, jstring filePath) {

    int rate, channel;
    const char *file_name = env->GetStringUTFChars(filePath, nullptr);
    LOGI(TAG, "file_name=%s", file_name);

    int ret = createAudioPlayer(&rate, &channel, file_name);
    if (ret < 0)
        return;
    ret = createEngine();
    if (ret < 0)
        return;
    ret = createBufferQueueAudioPlayer(rate, channel, SL_PCMSAMPLEFORMAT_FIXED_16);
    if (ret < 0)
        return;
    audioCallback(mPlayerBufferQueue, nullptr);
}

AUDIO_PLAYER_FUNC(void, stop) {
    if (mPlayerObject != nullptr) {
        (*mPlayerObject)->Destroy(mPlayerObject);
        mPlayerObject = nullptr;
        mPlayerPlay = nullptr;
        mPlayerBufferQueue = nullptr;
        mPlayerEffectSend = nullptr;
        mPlayerVolume = nullptr;
    }

    if (outputMixObject != nullptr) {
        (*outputMixObject)->Destroy(outputMixObject);
        outputMixObject = nullptr;
        outputMixEnvironmentalReverb = nullptr;
    }

    if (engineObject != nullptr) {
        (*engineObject)->Destroy(engineObject);
        engineObject = nullptr;
        engineEngine = nullptr;
    }

    releaseAudioPlayer();
}
