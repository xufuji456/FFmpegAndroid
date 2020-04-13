//
// Created by frank on 2018/2/1.
//
#include <jni.h>
#include <stdlib.h>
#include <unistd.h>
#include <android/log.h>

#include "libavformat/avformat.h"
#include "libavcodec/avcodec.h"
#include "libswscale/swscale.h"
#include "libswresample/swresample.h"
#include "ffmpeg_jni_define.h"

#define TAG "AudioPlayer"

#define MAX_AUDIO_FRAME_SIZE 48000 * 4

AUDIO_PLAYER_FUNC(void, play, jstring input_jstr) {
    const char *input_cstr = (*env)->GetStringUTFChars(env, input_jstr, NULL);
    LOGI(TAG, "input_cstr=%s", input_cstr);
    //register all modules
    av_register_all();
    AVFormatContext *pFormatCtx = avformat_alloc_context();
    //open the audio file
    if (avformat_open_input(&pFormatCtx, input_cstr, NULL, NULL) != 0) {
        LOGE(TAG, "Couldn't open the audio file!");
        return;
    }
    //find all streams info
    if (avformat_find_stream_info(pFormatCtx, NULL) < 0) {
        LOGE(TAG, "Couldn't find stream info!");
        return;
    }
    //get the audio stream index in the stream array
    int i = 0, audio_stream_idx = -1;
    for (; i < pFormatCtx->nb_streams; i++) {
        if (pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO) {
            audio_stream_idx = i;
            break;
        }
    }

    //find audio decoder
    AVCodecContext *codecCtx = pFormatCtx->streams[audio_stream_idx]->codec;
    AVCodec *codec = avcodec_find_decoder(codecCtx->codec_id);
    if (codec == NULL) {
        LOGE(TAG, "Couldn't find audio decoder!");
        return;
    }
    //open audio decoder
    if (avcodec_open2(codecCtx, codec, NULL) < 0) {
        LOGE(TAG, "Couldn't open audio decoder");
        return;
    }
    //malloc packet memory
    AVPacket *packet = (AVPacket *) av_malloc(sizeof(AVPacket));
    //malloc frame memory
    AVFrame *frame = av_frame_alloc();
    //frame->16bit 44100 PCM
    SwrContext *swrCtx = swr_alloc();

    //input sampleFormat
    enum AVSampleFormat in_sample_fmt = codecCtx->sample_fmt;
    //output sampleFormat: 16bit PCM
    enum AVSampleFormat out_sample_fmt = AV_SAMPLE_FMT_S16;
    //input sampleRate
    int in_sample_rate = codecCtx->sample_rate;
    //output sampleRate
    int out_sample_rate = in_sample_rate;
    //input channel layout(2 channels, stereo by default)
    uint64_t in_ch_layout = codecCtx->channel_layout;
    //output channel layout
    uint64_t out_ch_layout = AV_CH_LAYOUT_STEREO;

    swr_alloc_set_opts(swrCtx,
                       out_ch_layout, out_sample_fmt, out_sample_rate,
                       in_ch_layout, in_sample_fmt, in_sample_rate,
                       0, NULL);
    swr_init(swrCtx);

    //output channel number
    int out_channel_nb = av_get_channel_layout_nb_channels(out_ch_layout);

    jclass player_class = (*env)->GetObjectClass(env, thiz);
    if (!player_class) {
        LOGE(TAG, "player_class not found...");
    }
    //get AudioTrack by reflection
    jmethodID audio_track_method = (*env)->GetMethodID(env, player_class, "createAudioTrack",
                                                       "(II)Landroid/media/AudioTrack;");
    if (!audio_track_method) {
        LOGE(TAG, "audio_track_method not found...");
    }
    jobject audio_track = (*env)->CallObjectMethod(env, thiz, audio_track_method, out_sample_rate,
                                                   out_channel_nb);

    //call play method
    jclass audio_track_class = (*env)->GetObjectClass(env, audio_track);
    jmethodID audio_track_play_mid = (*env)->GetMethodID(env, audio_track_class, "play", "()V");
    (*env)->CallVoidMethod(env, audio_track, audio_track_play_mid);

    //get write method
    jmethodID audio_track_write_mid = (*env)->GetMethodID(env, audio_track_class, "write",
                                                          "([BII)I");

    uint8_t *out_buffer = (uint8_t *) av_malloc(MAX_AUDIO_FRAME_SIZE);

    int got_frame = 0, index = 0, ret;
    //read audio frame
    while (av_read_frame(pFormatCtx, packet) >= 0) {
        //is audio stream index
        if (packet->stream_index == audio_stream_idx) {
            //do decode
            ret = avcodec_decode_audio4(codecCtx, frame, &got_frame, packet);
            if (ret < 0) {
                break;
            }
            //decode success
            if (got_frame > 0) {
                LOGI(TAG, "decode frame count=%d", index++);
                //convert audio format
                swr_convert(swrCtx, &out_buffer, MAX_AUDIO_FRAME_SIZE,
                            (const uint8_t **) frame->data, frame->nb_samples);
                int out_buffer_size = av_samples_get_buffer_size(NULL, out_channel_nb,
                                                                 frame->nb_samples, out_sample_fmt,
                                                                 1);

                jbyteArray audio_sample_array = (*env)->NewByteArray(env, out_buffer_size);
                jbyte *sample_byte_array = (*env)->GetByteArrayElements(env, audio_sample_array,
                                                                        NULL);
                //copy buffer data
                memcpy(sample_byte_array, out_buffer, (size_t) out_buffer_size);
                //release byteArray
                (*env)->ReleaseByteArrayElements(env, audio_sample_array, sample_byte_array, 0);
                //call write method to play
                (*env)->CallIntMethod(env, audio_track, audio_track_write_mid,
                                      audio_sample_array, 0, out_buffer_size);
                //delete local reference
                (*env)->DeleteLocalRef(env, audio_sample_array);
                usleep(1000 * 16);
            }
        }
        av_free_packet(packet);
    }
    LOGI(TAG, "decode audio finish");
    av_frame_free(&frame);
    av_free(out_buffer);
    swr_free(&swrCtx);
    avcodec_close(codecCtx);
    avformat_close_input(&pFormatCtx);
    (*env)->ReleaseStringUTFChars(env, input_jstr, input_cstr);

}