#if defined(__ANDROID__)

#include "android/JniEnv.h"

#include <pthread.h>

static pthread_key_t kThreadKey;
JavaVM *kJvm = nullptr;

JniEnvPtr::JniEnvPtr() {
    if (!kJvm) {
        return;
    }

    if (kJvm->GetEnv(reinterpret_cast<void **>(&mEnv), JNI_VERSION_1_6) !=
        JNI_EDETACHED) {
        return;
    }

    if (kJvm->AttachCurrentThread(&mEnv, nullptr) < 0) {
        return;
    }

    pthread_setspecific(kThreadKey, mEnv);
}

JniEnvPtr::~JniEnvPtr() {}

JNIEnv *JniEnvPtr::operator->() {
    return mEnv;
}

JNIEnv *JniEnvPtr::Env() const {
    return mEnv;
}

void JniEnvPtr::JniThreadDestroyed(void *value) {
    auto *env = reinterpret_cast<JNIEnv *>(value);

    if (env != nullptr && kJvm != nullptr) {
        kJvm->DetachCurrentThread();
        pthread_setspecific(kThreadKey, nullptr);
    }
}

void JniEnvPtr::GlobalInit(JavaVM *vm) {
    kJvm = vm;
    pthread_key_create(&kThreadKey, JniThreadDestroyed);
}

#endif // __ANDROID__
