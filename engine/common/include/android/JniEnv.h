#ifndef JNI_ENV_H
#define JNI_ENV_H

#if defined(__ANDROID__)

#include <jni.h>

class JniEnvPtr {
public:
    JniEnvPtr();

    ~JniEnvPtr();

    JNIEnv *operator->();

    JNIEnv *Env() const;

    static void GlobalInit(JavaVM *vm);

private:
    JniEnvPtr(const JniEnvPtr &) = delete;

    JniEnvPtr &operator=(const JniEnvPtr &) = delete;

    static void JniThreadDestroyed(void *value);

private:
    JNIEnv *mEnv = nullptr;
};

#endif // __ANDROID__
#endif // JNI_ENV_H
