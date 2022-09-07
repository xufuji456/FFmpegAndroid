//
// Created by xu fulong on 2022/9/7.
//

#include <jni.h>

#include "ff_audio_resample.h"

COMMON_MEDIA_FUNC(int, audioResample, jstring srcFile, jstring dstFile, int sampleRate) {
    const char *src_file = env->GetStringUTFChars(srcFile, JNI_FALSE);
    const char *dst_file = env->GetStringUTFChars(dstFile, JNI_FALSE);

    auto *audioResample = new FFAudioResample();
    int ret = audioResample->resampling(src_file, dst_file, sampleRate);

    delete audioResample;
    env->ReleaseStringUTFChars(dstFile, dst_file);
    env->ReleaseStringUTFChars(srcFile, src_file);
    LOGE("AudioResample", "done......");
    return ret;
}