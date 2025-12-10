/**
 * Note: JNI helper of AudioTrack
 * Date: 2025/12/10
 * Author: frank
 */

#if defined(__ANDROID__)
#ifndef AUDIO_TRACK_HELPER_H
#define AUDIO_TRACK_HELPER_H

#include <jni.h>

#include "android/JniUtil.h"
#include "NextLog.h"

#define AUDIO_TRACK_JNI "AudioTrackJNI"

class AudioTrackJni {
public:
    static jclass FindAudioTrack(JNIEnv *env) {
        return env->FindClass("android/media/AudioTrack");
    }

    static jobject AudioTrackAsGlobalRef(
            JNIEnv *env, jint streamType, jint sampleRate, jint channelConfig,
            jint audioFormat, jint bufferSize, jint mode) {
        if (!env) {
            return nullptr;
        }
        jclass classAudioTrack = FindAudioTrack(env);
        jmethodID constructorAudioTrack = env->GetMethodID(classAudioTrack, "<init>", "(IIIIII)V");
        jobject ret_value = env->NewObject(
                classAudioTrack, constructorAudioTrack, streamType,
                sampleRate, channelConfig, audioFormat, bufferSize, mode);
        if (JniCheckExceptionClear(env)) {
            NEXT_LOGE(AUDIO_TRACK_JNI, "NewObject failed!\n");
            return nullptr;
        }
        jobject object_global = env->NewGlobalRef(ret_value);
        if (!object_global) {
            return nullptr;
        }

        JniDeleteLocalRefP(env, &ret_value);
        return object_global;
    }

    static jint AudioTrackGetMinBufferSize(JNIEnv *env, jint sampleRate,
                                           jint channelConfig, jint audioFormat) {
        if (!env) {
            return -1;
        }
        jclass classAudioTrack = FindAudioTrack(env);
        jmethodID methodGetMinBufferSize = env->GetStaticMethodID(classAudioTrack,
                                                                  "getMinBufferSize", "(III)I");
        jint ret_value =
                env->CallStaticIntMethod(classAudioTrack, methodGetMinBufferSize,
                                         sampleRate, channelConfig, audioFormat);
        if (JniCheckExceptionClear(env)) {
            return -1;
        }
        return ret_value;
    }

    static void AudioTrackPlay(JNIEnv *env, jobject clazz) {
        if (!env || !clazz) {
            return;
        }
        jclass classAudioTrack = FindAudioTrack(env);
        jmethodID methodPlay = env->GetMethodID(classAudioTrack, "play", "()V");
        env->CallVoidMethod(clazz, methodPlay);
        if (JniCheckExceptionClear(env)) {
            return;
        }
    }

    static void AudioTrackPause(JNIEnv *env, jobject clazz) {
        if (!env || !clazz) {
            return;
        }
        jclass classAudioTrack = FindAudioTrack(env);
        jmethodID methodPause = env->GetMethodID(classAudioTrack, "pause", "()V");
        env->CallVoidMethod(clazz, methodPause);
        if (JniCheckExceptionClear(env)) {
            return;
        }
    }

    static void AudioTrackFlush(JNIEnv *env, jobject clazz) {
        if (!env || !clazz) {
            return;
        }
        jclass classAudioTrack = FindAudioTrack(env);
        jmethodID methodFlush = env->GetMethodID(classAudioTrack, "flush", "()V");
        env->CallVoidMethod(clazz, methodFlush);
        if (JniCheckExceptionClear(env)) {
            NEXT_LOGE(AUDIO_TRACK_JNI, "flush error!\n");
            return;
        }
    }

    static void AudioTrackRelease(JNIEnv *env, jobject clazz) {
        if (!env || !clazz) {
            return;
        }
        jclass classAudioTrack = FindAudioTrack(env);
        jmethodID methodRelease = env->GetMethodID(classAudioTrack, "release", "()V");
        env->CallVoidMethod(clazz, methodRelease);
        if (JniCheckExceptionClear(env)) {
            NEXT_LOGE(AUDIO_TRACK_JNI, "release error!\n");
            return;
        }
    }

    static jint AudioTrackWrite(JNIEnv *env, jobject clazz, jbyteArray data, jint offset, jint size) {
        if (!env || !clazz) {
            return -1;
        }
        jclass classAudioTrack = FindAudioTrack(env);
        jmethodID methodWrite = env->GetMethodID(classAudioTrack, "write", "([BII)I");
        jint ret_value = env->CallIntMethod(clazz, methodWrite, data, offset, size);
        if (JniCheckExceptionClear(env)) {
            NEXT_LOGE(AUDIO_TRACK_JNI, "write error!\n");
            return -1;
        }
        return ret_value;
    }

    static jint AudioTrackSetVolume(JNIEnv *env, jobject clazz, jfloat left, jfloat right) {
        if (!env || !clazz) {
            return -1;
        }
        jclass classAudioTrack = FindAudioTrack(env);
        jmethodID methodSetVolume = env->GetMethodID(classAudioTrack, "setStereoVolume", "(FF)I");
        jint ret_value = env->CallIntMethod(clazz, methodSetVolume, left, right);
        if (JniCheckExceptionClear(env)) {
            return 0;
        }
        return ret_value;
    }

    static jint AudioTrackGetAudioSessionId(JNIEnv *env, jobject clazz) {
        if (!env || !clazz) {
            return -1;
        }
        jclass classAudioTrack = FindAudioTrack(env);
        jmethodID methodGetAudioSessionId = env->GetMethodID(classAudioTrack, "getAudioSessionId", "()I");
        jint ret_value = env->CallIntMethod(clazz, methodGetAudioSessionId);
        if (JniCheckExceptionClear(env)) {
            return 0;
        }
        return ret_value;
    }

    static void AudioTrackSetSpeed(JNIEnv *env, jobject clazz, jfloat speed) {
        jclass classAudioTrack = FindAudioTrack(env);
        if (JniGetApiLevel() < 23) {
            jmethodID methodGetSampleRate = env->GetMethodID(classAudioTrack, "getSampleRate", "()I");
            jint sample_rate = env->CallIntMethod(clazz, methodGetSampleRate);
            if (JniCheckException(env)) {
                return;
            }
            jmethodID methodSetPlaybackRate = env->GetMethodID(classAudioTrack, "setPlaybackRate", "(I)I");
            env->CallIntMethod(clazz, methodSetPlaybackRate, (jint) (sample_rate * speed));
            if (JniCheckExceptionClear(env)) {
                NEXT_LOGE(AUDIO_TRACK_JNI, "set speed fail, sample_rate=%d, speed=%f", sample_rate, speed);
                return;
            }
            return;
        }

        jmethodID methodGetPlaybackParams = env->GetMethodID(classAudioTrack, "getPlaybackParams",
                                                             "()Landroid/media/PlaybackParams;");
        jobject params = env->CallObjectMethod(clazz, methodGetPlaybackParams);
        jclass classPlaybackParams = env->FindClass("android/media/PlaybackParams");
        jmethodID methodParamsSetSpeed = env->GetMethodID(classPlaybackParams, "setSpeed",
                                                          "(F)Landroid/media/PlaybackParams;");
        jobject temp = env->CallObjectMethod(params, methodParamsSetSpeed, speed);
        JniDeleteLocalRefP(env, reinterpret_cast<jobject *>(&temp));

        jmethodID methodSetPlaybackParams = env->GetMethodID(classAudioTrack, "setPlaybackParams",
                                                             "(Landroid/media/PlaybackParams;)V");
        env->CallVoidMethod(clazz, methodSetPlaybackParams, params);

        JniDeleteLocalRefP(env, reinterpret_cast<jobject *>(&params));
    }
};

#endif // AUDIO_TRACK_HELPER_H
#endif // defined(__ANDROID__)
