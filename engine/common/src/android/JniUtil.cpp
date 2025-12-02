#if defined(__ANDROID__)

#include "android/JniUtil.h"

#include <new>
#include <stdexcept>
#include <sys/system_properties.h>

int JniGetApiLevel() {
    int ret = 0;
    char value[1024] = {0};
    __system_property_get("ro.build.version.sdk", value);
    ret = atoi(value);
    return ret;
}

bool JniCheckException(JNIEnv *env) {
    if (env->ExceptionCheck()) {
        env->ExceptionDescribe();
        return true;
    }

    return false;
}

bool JniCheckExceptionClear(JNIEnv *env) {
    if (env->ExceptionCheck()) {
        env->ExceptionDescribe();
        env->ExceptionClear();
        return true;
    }

    return false;
}

jclass JniGetClassGlobalRef(JNIEnv *env, const char *name) {
    jclass clazz = env->FindClass(name);
    if (!clazz) {
        return clazz;
    }
    auto clazz_global = (jclass) env->NewGlobalRef(clazz);
    JniDeleteLocalRefP(env, reinterpret_cast<jobject *>(&clazz));

    return clazz_global;
}

std::string JniGetStringUTFChars(JNIEnv *env, jstring str) {
    std::string result;
    if (!str)
        return result;

    const char *data = env->GetStringUTFChars(str, JNI_FALSE);
    result = (data ? data : "");
    env->ReleaseStringUTFChars(str, data);

    return result;
}

jbyteArray JniNewByteArrayGlobalRefCatch(JNIEnv *env, jsize capacity) {
    if (capacity <= 0) {
        return nullptr;
    }
    jbyteArray local = env->NewByteArray(capacity);
    if (JniCheckExceptionClear(env)) {
        return nullptr;
    }
    if (!local) {
        return nullptr;
    }
    auto global = (jbyteArray) env->NewGlobalRef((jobject) local);

    JniDeleteLocalRefP(env, reinterpret_cast<jobject *>(&local));

    return global;
}

void JniDeleteLocalRefP(JNIEnv *env, jobject *obj) {
    if (!obj)
        return;
    env->DeleteLocalRef(*obj);
    *obj = nullptr;
}

void JniDeleteGlobalRefP(JNIEnv *env, jobject *obj) {
    if (!obj)
        return;
    env->DeleteGlobalRef(*obj);
    *obj = nullptr;
}

#endif // __ANDROID__
