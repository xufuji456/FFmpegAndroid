#ifndef JNI_UTIL_H
#define JNI_UTIL_H

#if defined(__ANDROID__)

#include <functional>
#include <jni.h>
#include <string>

int JniGetApiLevel();
bool JniCheckException(JNIEnv *env);
bool JniCheckExceptionClear(JNIEnv *env);

jclass JniGetClassGlobalRef(JNIEnv *env, const char *name);

std::string JniGetStringUTFChars(JNIEnv *env, jstring str);


jbyteArray JniNewByteArrayGlobalRefCatch(JNIEnv *env, jsize capacity);

void JniDeleteLocalRefP(JNIEnv *env, jobject *obj);
void JniDeleteGlobalRefP(JNIEnv *env, jobject *obj);

#endif // __ANDROID__
#endif // JNI_UTIL_H
