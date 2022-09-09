//
// Created by xu fulong on 2022/9/9.
//

#include <jni.h>
#include "ff_http_pusher.h"

PUSHER_FUNC(int, pushStream, jstring inputPath, jstring outputPath) {
    int ret;
    const char *input_path = env->GetStringUTFChars(inputPath, JNI_FALSE);
    const char *output_path = env->GetStringUTFChars(outputPath, JNI_FALSE);
    auto *httpPusher = new FFHttpPusher();
    ret = httpPusher->open(input_path, output_path);
    if (ret < 0) {
        LOGE("HttpPusher", "open error=%d", ret);
        return ret;
    }
    ret = httpPusher->push();

    httpPusher->close();
    env->ReleaseStringUTFChars(inputPath, input_path);
    env->ReleaseStringUTFChars(outputPath, output_path);

    return ret;
}