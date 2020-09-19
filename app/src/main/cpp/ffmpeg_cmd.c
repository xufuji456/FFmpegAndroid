#include <jni.h>
#include "ffmpeg/ffmpeg.h"
#include "ffmpeg_jni_define.h"

#define FFMPEG_TAG "FFmpegCmd"

#define ALOGI(TAG, FORMAT, ...) __android_log_vprint(ANDROID_LOG_INFO, TAG, FORMAT, ##__VA_ARGS__);
#define ALOGE(TAG, FORMAT, ...) __android_log_vprint(ANDROID_LOG_ERROR, TAG, FORMAT, ##__VA_ARGS__);
#define ALOGW(TAG, FORMAT, ...) __android_log_vprint(ANDROID_LOG_WARN, TAG, FORMAT, ##__VA_ARGS__);

JNIEnv *ff_env;
jclass ff_class;
jmethodID ff_method;

void log_callback(void*, int, const char*, va_list);

void init(JNIEnv *env) {
    ff_env = env;
    ff_class = (*env)->FindClass(env, "com/frank/ffmpeg/FFmpegCmd");
    ff_method = (*env)->GetStaticMethodID(env, ff_class, "onProgressCallback", "(III)V");
}

FFMPEG_FUNC(jint, handle, jobjectArray commands) {
    init(env);
    // set the level of log
    av_log_set_level(AV_LOG_INFO);
    // set the callback of log, and redirect to print android log
    av_log_set_callback(log_callback);

    int argc = (*env)->GetArrayLength(env, commands);
    char **argv = (char **) malloc(argc * sizeof(char *));
    int i;
    int result;
    for (i = 0; i < argc; i++) {
        jstring jstr = (jstring) (*env)->GetObjectArrayElement(env, commands, i);
        char *temp = (char *) (*env)->GetStringUTFChars(env, jstr, 0);
        argv[i] = malloc(1024);
        strcpy(argv[i], temp);
        (*env)->ReleaseStringUTFChars(env, jstr, temp);
    }
    //execute ffmpeg cmd
    result = run(argc, argv);
    //release memory
    for (i = 0; i < argc; i++) {
        free(argv[i]);
    }
    free(argv);
    return result;
}

FFMPEG_FUNC(void, cancelTaskJni, jint cancel) {
    cancel_task(cancel);
}

void log_callback(void* ptr, int level, const char* format, va_list args) {
    switch (level) {
        case AV_LOG_WARNING:
            ALOGW(FFMPEG_TAG, format, args);
            break;
        case AV_LOG_INFO:
            ALOGI(FFMPEG_TAG, format, args);
            break;
        case AV_LOG_ERROR:
            ALOGE(FFMPEG_TAG, format, args);
            break;
        default:
            ALOGI(FFMPEG_TAG, format, args);
            break;
    }
}

void progress_callback(int position, int duration, int state) {
    if (ff_env && ff_class && ff_method) {
        (*ff_env)->CallStaticVoidMethod(ff_env, ff_class, ff_method, position, duration, state);
    }
}