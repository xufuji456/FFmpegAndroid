//
// Created by frank on 2018/2/3.
//
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"
#include "libswresample/swresample.h"
#include "AVpacket_queue.h"
#include <android/native_window.h>
#include <android/native_window_jni.h>
#include <stdio.h>
#include <unistd.h>
#include <libavutil/imgutils.h>
#include <android/log.h>
#include <pthread.h>
#include <jni.h>
#include <libavutil/time.h>
#include "ffmpeg_jni_define.h"

#define TAG "MediaPlayer"

#define MAX_AUDIO_FRAME_SIZE 48000 * 4
#define PACKET_SIZE 50
#define MIN_SLEEP_TIME_US 1000ll
#define AUDIO_TIME_ADJUST_US -200000ll

struct timeval now;
struct timespec timeout;

typedef struct MediaPlayer{
    AVFormatContext* format_context;
    int video_stream_index;
    int audio_stream_index;
    AVCodecContext* video_codec_context;
    AVCodecContext* audio_codec_context;
    AVCodec* video_codec;
    AVCodec* audio_codec;
    ANativeWindow* native_window;
    uint8_t* buffer;
    AVFrame* yuv_frame;
    AVFrame* rgba_frame;
    int video_width;
    int video_height;
    SwrContext* swrContext;
    int out_channel_nb;
    int out_sample_rate;
    enum AVSampleFormat out_sample_fmt;
    jobject audio_track;
    jmethodID audio_track_write_mid;
    uint8_t* audio_buffer;
    AVFrame* audio_frame;
    AVPacketQueue* packets[2];
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    int64_t start_time;
    int64_t audio_clock;
    pthread_t write_thread;
    pthread_t video_thread;
    pthread_t audio_thread;
}MediaPlayer;

typedef struct Decoder{
    MediaPlayer* player;
    int stream_index;
}Decoder;

JavaVM* javaVM;
MediaPlayer* player;

jint JNICALL JNI_OnLoad(JavaVM* vm, void* reserved){
    javaVM = vm;
    return JNI_VERSION_1_6;
}

//init player
int init_input_format_context(MediaPlayer* player, const char* file_name){
    //register all modules
    av_register_all();
    //alloc format context
    player->format_context = avformat_alloc_context();
    //open the input file
    if(avformat_open_input(&player->format_context, file_name, NULL, NULL)!=0) {
        LOGE(TAG, "Couldn't open file:%s\n", file_name);
        return -1;
    }
    //find the info of all streams
    if(avformat_find_stream_info(player->format_context, NULL)<0) {
        LOGE(TAG, "Couldn't find stream information.");
        return -1;
    }
    //find the index of audio and video stream
    int i;
    player->video_stream_index = -1;
    player->audio_stream_index = -1;
    for (i = 0; i < player->format_context->nb_streams; i++) {
        if (player->format_context->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO
            && player->video_stream_index < 0) {
            player->video_stream_index = i;
        } else if (player->format_context->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO
            && player->audio_stream_index < 0) {
            player->audio_stream_index = i;
        }
    }
    if(player->video_stream_index==-1) {
        LOGE(TAG, "couldn't find a video stream.");
        return -1;
    }
    if(player->audio_stream_index==-1) {
        LOGE(TAG, "couldn't find a audio stream.");
        return -1;
    }
    LOGI(TAG, "video_stream_index=%d", player->video_stream_index);
    LOGI(TAG, "audio_stream_index=%d", player->audio_stream_index);
    return 0;
}

//open decoder
int init_condec_context(MediaPlayer* player){
    //get codec context
    player->video_codec_context = player->format_context->streams[player->video_stream_index]->codec;
    //find audio and video decoder
    player->video_codec = avcodec_find_decoder(player->video_codec_context->codec_id);
    if(player->video_codec == NULL) {
        LOGE(TAG, "couldn't find video Codec.");
        return -1;
    }
    if(avcodec_open2(player->video_codec_context, player->video_codec, NULL) < 0) {
        LOGE(TAG, "Couldn't open video codec.");
        return -1;
    }
    player->audio_codec_context = player->format_context->streams[player->audio_stream_index]->codec;
    player->audio_codec = avcodec_find_decoder(player->audio_codec_context->codec_id);
    if( player->audio_codec == NULL) {
        LOGE(TAG, "couldn't find audio Codec.");
        return -1;
    }
    if(avcodec_open2(player->audio_codec_context, player->audio_codec, NULL) < 0) {
        LOGE(TAG, "Couldn't open audio codec.");
        return -1;
    }
    // width and height
    player->video_width = player->video_codec_context->width;
    player->video_height = player->video_codec_context->height;
    return 0;
}

void video_player_prepare(MediaPlayer* player, JNIEnv* env, jobject surface){
    // get native window
    player->native_window = ANativeWindow_fromSurface(env, surface);
}

//get current playing time
int64_t get_play_time(MediaPlayer* player){
    return (int64_t)(av_gettime() - player->start_time);
}

/**
 * audio and video synchronization
 */
void player_wait_for_frame(MediaPlayer *player, int64_t stream_time) {
    pthread_mutex_lock(&player->mutex);
    for(;;){
        int64_t current_video_time = get_play_time(player);
        int64_t sleep_time = stream_time - current_video_time;
        if (sleep_time < -300000ll) {
            // 300 ms late
            int64_t new_value = player->start_time - sleep_time;
            player->start_time = new_value;
            pthread_cond_broadcast(&player->cond);
        }

        if (sleep_time <= MIN_SLEEP_TIME_US) {
            // We do not need to wait if time is slower then minimal sleep time
            break;
        }

        if (sleep_time > 500000ll) {
            // if sleep time is bigger then 500ms just sleep this 500ms
            // and check everything again
            sleep_time = 500000ll;
        }
        //TODO: waiting util time out
//        pthread_cond_timeout_np(&player->cond, &player->mutex,
//                                                  (unsigned int) (sleep_time / 1000ll));
        gettimeofday(&now, NULL);
        timeout.tv_sec = now.tv_sec;
        timeout.tv_nsec = (now.tv_usec + sleep_time) * 1000;
        pthread_cond_timedwait(&player->cond, &player->mutex, &timeout);
    }
    pthread_mutex_unlock(&player->mutex);
}

//decode video packet as a frame
int decode_video(MediaPlayer* player, AVPacket* packet){
    ANativeWindow_setBuffersGeometry(player->native_window,  player->video_width,
                                     player->video_height, WINDOW_FORMAT_RGBA_8888);

    ANativeWindow_Buffer windowBuffer;
    player->yuv_frame = av_frame_alloc();
    player->rgba_frame = av_frame_alloc();
    if(player->rgba_frame == NULL || player->yuv_frame == NULL) {
        LOGE(TAG, "Couldn't allocate video frame.");
        return -1;
    }

    // get buffer size, the format is RGBA
    int numBytes=av_image_get_buffer_size(AV_PIX_FMT_RGBA, player->video_width, player->video_height, 1);

    player->buffer = (uint8_t *)av_malloc(numBytes*sizeof(uint8_t));
    av_image_fill_arrays(player->rgba_frame->data, player->rgba_frame->linesize, player->buffer, AV_PIX_FMT_RGBA,
                         player->video_width, player->video_height, 1);

    struct SwsContext *sws_ctx = sws_getContext(
            player->video_width,
            player->video_height,
            player->video_codec_context->pix_fmt,
            player->video_width,
            player->video_height,
            AV_PIX_FMT_RGBA,
            SWS_BILINEAR,
            NULL,
            NULL,
            NULL);

    int frameFinished;
    //decode video frame
    int ret = avcodec_decode_video2(player->video_codec_context, player->yuv_frame, &frameFinished, packet);
    if(ret < 0){
        LOGE(TAG, "avcodec_decode_video2 error...");
        return -1;
    }
    if (frameFinished) {
        // lock native window
        ANativeWindow_lock(player->native_window, &windowBuffer, 0);
        // yuv to rgba
        sws_scale(sws_ctx, (uint8_t const * const *)player->yuv_frame->data,
                  player->yuv_frame->linesize, 0, player->video_height,
                  player->rgba_frame->data, player->rgba_frame->linesize);
        uint8_t * dst = windowBuffer.bits;
        int dstStride = windowBuffer.stride * 4;
        uint8_t * src = player->rgba_frame->data[0];
        int srcStride = player->rgba_frame->linesize[0];
        // copy data to the target buffer
        int h;
        for (h = 0; h < player->video_height; h++) {
            memcpy(dst + h * dstStride, src + h * srcStride, (size_t) srcStride);
        }

        //calculate pts
        int64_t pts = av_frame_get_best_effort_timestamp(player->yuv_frame);
        AVStream *stream = player->format_context->streams[player->video_stream_index];
        //convert to real time
        int64_t time = av_rescale_q(pts, stream->time_base, AV_TIME_BASE_Q);
        //synchronize
        player_wait_for_frame(player, time);

        ANativeWindow_unlockAndPost(player->native_window);
    }
    return 0;
}

//init audio decoder
void audio_decoder_prepare(MediaPlayer* player) {
    player->swrContext = swr_alloc();

    //input format
    enum AVSampleFormat in_sample_fmt = player->audio_codec_context->sample_fmt;
    //output format
    player->out_sample_fmt = AV_SAMPLE_FMT_S16;
    //input sampleRate
    int in_sample_rate = player->audio_codec_context->sample_rate;
    //output sampleRate
    player->out_sample_rate = in_sample_rate;
    //channel layout (stereo by default)
    uint64_t in_ch_layout = player->audio_codec_context->channel_layout;
    //output channel layout
    uint64_t out_ch_layout = AV_CH_LAYOUT_STEREO;

    swr_alloc_set_opts(player->swrContext,
                       out_ch_layout, player->out_sample_fmt, player->out_sample_rate,
                       in_ch_layout, in_sample_fmt, in_sample_rate,
                       0, NULL);
    swr_init(player->swrContext);
    player->out_channel_nb = av_get_channel_layout_nb_channels(out_ch_layout);
}

void audio_player_prepare(MediaPlayer* player, JNIEnv* env, jclass jthiz){
    jclass player_class = (*env)->GetObjectClass(env,jthiz);
    if(!player_class){
        LOGE(TAG, "player_class not found...");
    }
    //get AudioTrack by reflection
    jmethodID audio_track_method = (*env)->GetMethodID(
            env,player_class,"createAudioTrack","(II)Landroid/media/AudioTrack;");
    if(!audio_track_method){
        LOGE(TAG, "audio_track_method not found...");
    }
    jobject audio_track = (*env)->CallObjectMethod(
            env,jthiz,audio_track_method, player->out_sample_rate, player->out_channel_nb);

    jclass audio_track_class = (*env)->GetObjectClass(env, audio_track);
    jmethodID audio_track_play_mid = (*env)->GetMethodID(env,audio_track_class,"play","()V");
    (*env)->CallVoidMethod(env, audio_track, audio_track_play_mid);

    player->audio_track = (*env)->NewGlobalRef(env, audio_track);
    player->audio_track_write_mid = (*env)->GetMethodID(env,audio_track_class,"write","([BII)I");

    //malloc buffer
    player->audio_buffer = (uint8_t *)av_malloc(MAX_AUDIO_FRAME_SIZE);
    player->audio_frame = av_frame_alloc();
}

//audio decode function
int decode_audio(MediaPlayer* player, AVPacket* packet){
    int got_frame = 0, ret;
    ret = avcodec_decode_audio4(player->audio_codec_context, player->audio_frame, &got_frame, packet);
    if(ret < 0){
        LOGE(TAG, "avcodec_decode_audio4 error...");
        return -1;
    }
    //decode success
    if(got_frame > 0){
        //convert audio format
        swr_convert(player->swrContext,
                &player->audio_buffer,
                MAX_AUDIO_FRAME_SIZE,
                (const uint8_t **)player->audio_frame->data,
                player->audio_frame->nb_samples);
        int out_buffer_size = av_samples_get_buffer_size(NULL,
                player->out_channel_nb,
                player->audio_frame->nb_samples,
                player->out_sample_fmt,
                1);

        //synchronize
        int64_t pts = packet->pts;
        if (pts != AV_NOPTS_VALUE) {
            AVStream *stream = player->format_context->streams[player->audio_stream_index];
            player->audio_clock = av_rescale_q(pts, stream->time_base, AV_TIME_BASE_Q);
            player_wait_for_frame(player, player->audio_clock + AUDIO_TIME_ADJUST_US);
        }

        if(javaVM != NULL){
            JNIEnv * env;
            (*javaVM)->AttachCurrentThread(javaVM, &env, NULL);
            jbyteArray audio_sample_array = (*env)->NewByteArray(env,out_buffer_size);
            jbyte* sample_byte_array = (*env)->GetByteArrayElements(env,audio_sample_array,NULL);
            memcpy(sample_byte_array, player->audio_buffer, (size_t) out_buffer_size);
            (*env)->ReleaseByteArrayElements(env,audio_sample_array,sample_byte_array,0);
            //call write method of AudioTrack to play
            (*env)->CallIntMethod(env, player->audio_track, player->audio_track_write_mid,
                                  audio_sample_array,0,out_buffer_size);
            //release local reference
            (*env)->DeleteLocalRef(env,audio_sample_array);
        }
    }
    if(javaVM != NULL){
        (*javaVM)->DetachCurrentThread(javaVM);
    }
    return 0;
}

void init_queue(MediaPlayer* player, int size){
    int i;
    for (i = 0; i < 2; ++i) {
        AVPacketQueue* queue = queue_init(size);
        player->packets[i] = queue;
    }
}

void delete_queue(MediaPlayer* player){
    int i;
    for (i = 0; i < 2; ++i) {
        queue_free(player->packets[i]);
    }
}

//the thread of write packet
void* write_packet_to_queue(void* arg){
    MediaPlayer* player = (MediaPlayer*)arg;
    AVPacket packet, *pkt = &packet;
    int ret;
    for(;;){
        ret = av_read_frame(player->format_context, pkt);
        if(ret < 0){
            break;
        }
        if(pkt->stream_index == player->video_stream_index || pkt->stream_index == player->audio_stream_index){
            AVPacketQueue *queue = player->packets[pkt->stream_index];
            pthread_mutex_lock(&player->mutex);
            AVPacket* data = queue_push(queue, &player->mutex, &player->cond);
            pthread_mutex_unlock(&player->mutex);
            *data = packet;
        }
    }
}

//the thread of decoding
void* decode_func(void* arg){
    Decoder *decoder_data = (Decoder*)arg;
    MediaPlayer *player = decoder_data->player;
    int stream_index = decoder_data->stream_index;
    AVPacketQueue *queue = player->packets[stream_index];
    int ret = 0;

    for(;;) {
        pthread_mutex_lock(&player->mutex);
        AVPacket *packet = (AVPacket*)queue_pop(queue, &player->mutex, &player->cond);
        pthread_mutex_unlock(&player->mutex);

        if(stream_index == player->video_stream_index) {//video stream
            ret = decode_video(player, packet);
        } else if(stream_index == player->audio_stream_index) {//audio stream
            ret = decode_audio(player, packet);
        }
        av_packet_unref(packet);
        if(ret < 0){
            break;
        }
    }
}

MEDIA_PLAYER_FUNC(jint, setup, jstring filePath, jobject surface){

    const char *file_name = (*env)->GetStringUTFChars(env, filePath, JNI_FALSE);
    int ret;
    player = malloc(sizeof(MediaPlayer));
    if(player == NULL){
        return -1;
    }
    ret = init_input_format_context(player, file_name);
    if(ret < 0){
        return ret;
    }
    ret = init_condec_context(player);
    if(ret < 0){
        return ret;
    }
    //init surface
    video_player_prepare( player, env, surface);
    //init params of player
    audio_decoder_prepare(player);
    //init player
    audio_player_prepare(player, env, thiz);
    //init queue
    init_queue(player, PACKET_SIZE);

    return 0;
}

MEDIA_PLAYER_FUNC(jint, play){
    pthread_mutex_init(&player->mutex, NULL);
    pthread_cond_init(&player->cond, NULL);

    pthread_create(&player->write_thread, NULL, write_packet_to_queue, (void*)player);
    sleep(1);
    player->start_time = 0;

    Decoder data1 = {player, player->video_stream_index}, *decoder_data1 = &data1;
    pthread_create(&player->video_thread, NULL, decode_func, (void*)decoder_data1);

    Decoder data2 = {player, player->audio_stream_index}, *decoder_data2 = &data2;
    pthread_create(&player->audio_thread,NULL,decode_func,(void*)decoder_data2);


    pthread_join(player->write_thread, NULL);
    pthread_join(player->video_thread, NULL);
    pthread_join(player->audio_thread, NULL);

    return 0;
}

MEDIA_PLAYER_FUNC(void, release){
    free(player->audio_track);
    free(player->audio_track_write_mid);
    av_free(player->buffer);
    av_free(player->rgba_frame);
    av_free(player->yuv_frame);
    av_free(player->audio_buffer);
    av_free(player->audio_frame);
    avcodec_close(player->video_codec_context);
    avcodec_close(player->audio_codec_context);
    avformat_close_input(&player->format_context);
    ANativeWindow_release(player->native_window);
    delete_queue(player);
    pthread_cond_destroy(&player->cond);
    pthread_mutex_destroy(&player->mutex);
    free(player);
    (*javaVM)->DestroyJavaVM(javaVM);
}
