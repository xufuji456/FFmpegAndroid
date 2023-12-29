//
// Created by xu fulong on 2022/9/4.
//

#include "ff_audio_player.h"
#include <jni.h>
#include <unistd.h>

#define SLEEP_TIME (16000)

void fftCallback(JNIEnv *env, jobject thiz, jmethodID fft_method, int8_t *data, int size) {
    jbyteArray dataArray = env->NewByteArray(size);
    env->SetByteArrayRegion(dataArray, 0, size, data);
    env->CallVoidMethod(thiz, fft_method, dataArray);
    env->DeleteLocalRef(dataArray);
}

AUDIO_PLAYER_FUNC(long, native_1init) {
    auto *audioPlayer = new FFAudioPlayer();
    return (long)audioPlayer;
}

AUDIO_PLAYER_FUNC(void, native_1play, long context, jstring path, jstring filter) {
    if (path == nullptr)
        return;

    int result = 0;
    const char* native_path = env->GetStringUTFChars(path, JNI_FALSE);
    auto *audioPlayer = (FFAudioPlayer*) context;
    // open stream, and init work
    audioPlayer->open(native_path);
    // init AudioTrack
    jclass audio_class = env->GetObjectClass(thiz);
    jmethodID audio_track_method = env->GetMethodID(audio_class,
                                                    "createAudioTrack", "(II)Landroid/media/AudioTrack;");
    jobject audio_track = env->CallObjectMethod(thiz, audio_track_method,
                                                audioPlayer->getSampleRate(), audioPlayer->getChannel());
    // play function
    jclass audio_track_class = env->GetObjectClass(audio_track);
    jmethodID play_method = env->GetMethodID(audio_track_class, "play", "()V");
    env->CallVoidMethod(audio_track, play_method);
    jmethodID write_method     = env->GetMethodID(audio_track_class, "write", "([BII)I");
    jmethodID play_info_method = env->GetMethodID(audio_class, "playInfoFromJNI", "(I)V");
    env->CallVoidMethod(thiz, play_info_method, 1);

    // demux decode and play
    while (result >= 0) {
        result = audioPlayer->decodeAudio();
        if (result == 0) {
            continue;
        } else if (result < 0) {
            break;
        }
        int size = result;
        // call AudioTrack to play(should be reused array)
        jbyteArray audio_array = env->NewByteArray(size);
        jbyte *data_address = env->GetByteArrayElements(audio_array, JNI_FALSE);
        memcpy(data_address, audioPlayer->getDecodeFrame(), size);
        env->ReleaseByteArrayElements(audio_array, data_address, 0);
        env->CallIntMethod(audio_track, write_method, audio_array, 0, size);
        env->DeleteLocalRef(audio_array);

        // audio sync
        usleep(SLEEP_TIME);
    }

    if (result == AVERROR_EOF) {
        env->CallVoidMethod(thiz, play_info_method, 2);
    }
    env->ReleaseStringUTFChars(path, native_path);
    jmethodID release_method = env->GetMethodID(audio_class, "releaseAudioTrack", "()V");
    env->CallVoidMethod(thiz, release_method);
    audioPlayer->close();
    delete audioPlayer;
}

AUDIO_PLAYER_FUNC(void, native_1again, long context, jstring filter_jstr) {
    if (!filter_jstr) return;
    auto *audioPlayer = (FFAudioPlayer*) context;
    audioPlayer->setFilterAgain(true);
    const char *desc = env->GetStringUTFChars(filter_jstr, nullptr);
    audioPlayer->setFilterDesc(desc);
}

AUDIO_PLAYER_FUNC(long, native_1get_1position, long context) {
    auto *audioPlayer = (FFAudioPlayer*) context;
    if (!audioPlayer)
        return 0;
    return audioPlayer->getCurrentPosition();
}

AUDIO_PLAYER_FUNC(long, native_1get_1duration, long context) {
    auto *audioPlayer = (FFAudioPlayer*) context;
    if (!audioPlayer)
        return 0;
    return audioPlayer->getDuration();
}

AUDIO_PLAYER_FUNC(void, native_1release, long context) {
    auto *audioPlayer = (FFAudioPlayer*) context;
    if (!audioPlayer)
        return;
    audioPlayer->setExit(true);
}


extern "C"
JNIEXPORT jstring JNICALL
Java_com_frank_ffmpeg_FFmpegCmd_getInfo(JNIEnv *env, jclass clazz) {
    const char* ffmpeg_version = av_version_info();
    LOGE("Version","ffmpeg version: %s",ffmpeg_version);
    return env->NewStringUTF( ffmpeg_version);
}