//
// Created by frank on 2021/8/28.
//

#include <jni.h>
#include "frank_visualizer.h"

#define VISUALIZER_FUNC(RETURN_TYPE, NAME, ...) \
  extern "C" { \
  JNIEXPORT RETURN_TYPE \
    Java_com_frank_ffmpeg_effect_FrankVisualizer_ ## NAME \
      (JNIEnv* env, jobject thiz, ##__VA_ARGS__);\
  } \
  JNIEXPORT RETURN_TYPE \
    Java_com_frank_ffmpeg_effect_FrankVisualizer_ ## NAME \
      (JNIEnv* env, jobject thiz, ##__VA_ARGS__)\

struct fields_t {
    jfieldID context;
    jclass visual_class;
    jmethodID fft_method;
    jbyteArray data_array;
};

static fields_t fields;
static const char *className = "com/frank/ffmpeg/effect/FrankVisualizer";

void setCustomVisualizer(JNIEnv *env, jobject thiz) {
    auto *customVisualizer = new FrankVisualizer();
    jclass clazz = env->FindClass(className);
    if (!clazz) {
        return;
    }
    fields.context = env->GetFieldID(clazz, "mNativeVisualizer", "J");
    if (!fields.context) {
        return;
    }
    env->SetLongField(thiz, fields.context, (jlong) customVisualizer);
}

FrankVisualizer *getCustomVisualizer(JNIEnv *env, jobject thiz) {
    if (!fields.context) return nullptr;
    return (FrankVisualizer *) env->GetLongField(thiz, fields.context);
}

void fft_callback(JNIEnv *jniEnv, int8_t * arg, int samples) {
    jniEnv->SetByteArrayRegion(fields.data_array, 0, samples, arg);
    jniEnv->CallStaticVoidMethod(fields.visual_class, fields.fft_method, fields.data_array);
}

VISUALIZER_FUNC(int, nativeInitVisualizer) {
    setCustomVisualizer(env, thiz);
    FrankVisualizer *mVisualizer = getCustomVisualizer(env, thiz);
    if (!mVisualizer) return -2;
    jclass mVisualClass = env->FindClass(className);
    fields.visual_class = (jclass) env->NewGlobalRef(mVisualClass);
    fields.fft_method = env->GetStaticMethodID(fields.visual_class, "onFftCallback", "([B)V");
    jbyteArray dataArray = env->NewByteArray(MAX_FFT_SIZE);
    fields.data_array = (jbyteArray) env->NewGlobalRef(dataArray);
    return mVisualizer->init_visualizer();
}

VISUALIZER_FUNC(int, nativeCaptureData, jobject buffer, jint size) {
    if (!buffer) return -1;
    FrankVisualizer *mVisualizer = getCustomVisualizer(env, thiz);
    if (!mVisualizer) return -2;
    int nb_samples = size < MAX_FFT_SIZE ? size : MAX_FFT_SIZE;
    if (nb_samples >= MIN_FFT_SIZE) {
        auto *input_buffer = static_cast<uint8_t *>(env->GetDirectBufferAddress(buffer));
        int8_t *output_data = mVisualizer->fft_run(input_buffer, nb_samples);
        fft_callback(env, output_data, mVisualizer->getOutputSample());
    }
    return 0;
}

VISUALIZER_FUNC(void, nativeReleaseVisualizer) {
    FrankVisualizer *mVisualizer = getCustomVisualizer(env, thiz);
    if (!mVisualizer) return;
    mVisualizer->release_visualizer();
    delete mVisualizer;
    env->DeleteGlobalRef(fields.data_array);
    env->DeleteGlobalRef(fields.visual_class);
    env->SetLongField(thiz, fields.context, 0);
}