#include <jni.h>
#include "ffmpeg/ffmpeg.h"
#include "ffmpeg_jni_define.h"

#define FFMPEG_TAG "FFmpegCmd"
#define INPUT_SIZE (4 * 1024)

#define ALOGI(TAG, FORMAT, ...) __android_log_vprint(ANDROID_LOG_INFO, TAG, FORMAT, ##__VA_ARGS__);
#define ALOGE(TAG, FORMAT, ...) __android_log_vprint(ANDROID_LOG_ERROR, TAG, FORMAT, ##__VA_ARGS__);

int err_count;
JNIEnv *ff_env;
jclass ff_class;
jmethodID ff_method;
jmethodID msg_method;

void log_callback(void *, int, const char *, va_list);

void init(JNIEnv *env) {
    ff_env = env;
    err_count = 0;
    ff_class = (*env)->FindClass(env, "com/frank/ffmpeg/FFmpegCmd");
    ff_method = (*env)->GetStaticMethodID(env, ff_class, "onProgressCallback", "(III)V");
    msg_method = (*env)->GetStaticMethodID(env, ff_class, "onMsgCallback", "(Ljava/lang/String;)V");
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
        argv[i] = malloc(INPUT_SIZE);
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

void msg_callback(const char *format, va_list args) {
    if (ff_env && msg_method) {
        char *ff_msg = (char *) malloc(sizeof(char) * INPUT_SIZE);
        vsprintf(ff_msg, format, args);
        jstring jstr = (*ff_env)->NewStringUTF(ff_env, ff_msg);
        (*ff_env)->CallStaticVoidMethod(ff_env, ff_class, msg_method, jstr);
        free(ff_msg);
    }
}

void log_callback(void *ptr, int level, const char *format, va_list args) {
    switch (level) {
        case AV_LOG_INFO:
            ALOGI(FFMPEG_TAG, format, args);
            if (format && strncmp("silence", format, 7) == 0) {
                msg_callback(format, args);
            }
            break;
        case AV_LOG_ERROR:
            ALOGE(FFMPEG_TAG, format, args);
            if (err_count < 10) {
                err_count++;
                msg_callback(format, args);
            }
            break;
        default:
            break;
    }
}

void progress_callback(int position, int duration, int state) {
    if (ff_env && ff_class && ff_method) {
        (*ff_env)->CallStaticVoidMethod(ff_env, ff_class, ff_method, position, duration, state);
    }
}