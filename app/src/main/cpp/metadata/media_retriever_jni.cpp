/*
 * Created by frank on 2022/2/23
 *
 * part of code from William Seemann
 */

#include <android/log.h>
#include <media_retriever.h>
#include "jni.h"

#include <android/bitmap.h>
#include "ffmpeg_jni_define.h"

#define LOG_TAG "FFmpegMediaRetriever"

struct fields_t {
    jfieldID context;
};

static fields_t fields;
static ANativeWindow* theNativeWindow;
static const char* kClassPathName = "com/frank/ffmpeg/metadata/FFmpegMediaRetriever";

static jstring NewStringUTF(JNIEnv* env, const char* data) {
    jstring str = nullptr;
    int size = strlen(data);
    jbyteArray array = env->NewByteArray(size);
    if (!array) {
        LOGE(LOG_TAG, "convertString: OutOfMemoryError is thrown.");
    } else {
        jbyte* bytes = env->GetByteArrayElements(array, nullptr);
        if (bytes != nullptr) {
            strcpy((char *)bytes, data);
            env->ReleaseByteArrayElements(array, bytes, 0);

            jclass string_Clazz = env->FindClass("java/lang/String");
            jmethodID string_initMethodID = env->GetMethodID(string_Clazz, "<init>", "([BLjava/lang/String;)V");
            jstring utf = env->NewStringUTF("UTF-8");
            str = (jstring) env->NewObject(string_Clazz, string_initMethodID, array, utf);

            env->DeleteLocalRef(utf);
        }
    }
    env->DeleteLocalRef(array);

    return str;
}

void jniThrowException(JNIEnv* env, const char* className,
    const char* msg) {
    jclass exception = env->FindClass(className);
    env->ThrowNew(exception, msg);
}

static void process_retriever_call(JNIEnv *env, int status, const char* exception, const char *message)
{
    if (status == -2) {
        jniThrowException(env, "java/lang/IllegalStateException", nullptr);
    } else if (status == -1) {
        if (strlen(message) > 520) {
            jniThrowException( env, exception, message);
        } else {
            char msg[256];
            sprintf(msg, "%s: status = 0x%X", message, status);
            jniThrowException( env, exception, msg);
        }
    }
}

static MediaRetriever* getRetriever(JNIEnv* env, jobject thiz)
{
    auto* retriever = (MediaRetriever*) env->GetLongField(thiz, fields.context);
    return retriever;
}

static void setRetriever(JNIEnv* env, jobject thiz, long retriever)
{
    env->SetLongField(thiz, fields.context, retriever);
}

RETRIEVER_FUNC(void, native_setup)
{
    auto* retriever = new MediaRetriever();
    setRetriever(env, thiz, (long)retriever);
}

RETRIEVER_FUNC(void, native_init)
{
    jclass clazz = env->FindClass(kClassPathName);
    if (!clazz) {
        return;
    }

    fields.context = env->GetFieldID(clazz, "mNativeContext", "J");
    if (fields.context == nullptr) {
        return;
    }

    av_register_all();
    avformat_network_init();
}

RETRIEVER_FUNC(void, native_set_dataSource, jstring path) {
    MediaRetriever* retriever = getRetriever(env, thiz);
    if (retriever == nullptr) {
        jniThrowException(env, "java/lang/IllegalStateException", "No retriever available");
        return;
    }
    if (!path) {
        jniThrowException(env, "java/lang/IllegalArgumentException", "Null pointer");
        return;
    }
    const char *tmp = env->GetStringUTFChars(path, nullptr);
    if (!tmp) {
        return;
    }

    process_retriever_call(
            env,
            retriever->setDataSource(tmp),
            "java/lang/IllegalArgumentException",
            "setDataSource failed");

    env->ReleaseStringUTFChars(path, tmp);
}

static int getFileDescriptor(JNIEnv * env, jobject fileDescriptor) {
    jint fd = -1;
    jclass fdClass = env->FindClass("java/io/FileDescriptor");

    if (fdClass != nullptr) {
        jfieldID fdClassDescriptorFieldID = env->GetFieldID(fdClass, "descriptor", "I");
        if (fdClassDescriptorFieldID != nullptr && fileDescriptor != nullptr) {
            fd = env->GetIntField(fileDescriptor, fdClassDescriptorFieldID);
        }
    }

    return fd;
}

RETRIEVER_FUNC(void, native_set_dataSourceFD, jobject fileDescriptor, jlong offset, jlong length)
{
    if (offset < 0 || length < 0) {
        return;
    }
    MediaRetriever* retriever = getRetriever(env, thiz);
    if (retriever == nullptr) {
        jniThrowException(env, "java/lang/IllegalStateException", "No retriever available");
        return;
    }
    if (!fileDescriptor) {
        jniThrowException(env, "java/lang/IllegalArgumentException", nullptr);
        return;
    }
    int fd = getFileDescriptor(env, fileDescriptor);
    if (fd < 0) {
        LOGE(LOG_TAG, "invalid file descriptor!");
        jniThrowException(env, "java/lang/IllegalArgumentException", nullptr);
        return;
    }
    process_retriever_call(env, retriever->setDataSource(fd, offset, length),
            "java/lang/RuntimeException", "setDataSource failed");
}

RETRIEVER_FUNC(void, native_set_surface, jobject surface)
{
    MediaRetriever* retriever = getRetriever(env, thiz);
    if (retriever == nullptr) {
        jniThrowException(env, "java/lang/IllegalStateException", "No retriever available");
        return;
    }
    theNativeWindow = ANativeWindow_fromSurface(env, surface);
    if (theNativeWindow != nullptr) {
        retriever->setNativeWindow(theNativeWindow);
    }
}

RETRIEVER_FUNC(jobject, native_extract_metadata, jstring jkey)
{
    MediaRetriever* retriever = getRetriever(env, thiz);
    if (retriever == nullptr) {
        jniThrowException(env, "java/lang/IllegalStateException", "No retriever available");
        return nullptr;
    }
    if (!jkey) {
        jniThrowException(env, "java/lang/IllegalArgumentException", "Null pointer");
        return nullptr;
    }
    const char *key = env->GetStringUTFChars(jkey, nullptr);
    if (!key) {
        return nullptr;
    }
    const char* value = retriever->extractMetadata(key);
    if (!value) {
        return nullptr;
    }
    env->ReleaseStringUTFChars(jkey, key);
    return NewStringUTF(env, value);
}

RETRIEVER_FUNC(jbyteArray, native_get_frameAtTime, jlong timeUs, jint option)
{
    MediaRetriever* retriever = getRetriever(env, thiz);
    if (retriever == nullptr) {
        jniThrowException(env, "java/lang/IllegalStateException", "No retriever available");
        return nullptr;
    }

    AVPacket packet;
    av_init_packet(&packet);
    jbyteArray array = nullptr;

    if (retriever->getFrameAtTime(timeUs, option, &packet) == 0) {
        int size = packet.size;
        uint8_t* data = packet.data;
        array = env->NewByteArray(size);
        if (!array) {
            LOGE(LOG_TAG, "getFrameAtTime: OutOfMemoryError is thrown.");
        } else {
            jbyte* bytes = env->GetByteArrayElements(array, nullptr);
            if (bytes != nullptr) {
                memcpy(bytes, data, size);
                env->ReleaseByteArrayElements(array, bytes, 0);
            }
        }
    }

    av_packet_unref(&packet);

    return array;
}

RETRIEVER_FUNC(jbyteArray, native_get_scaleFrameAtTime, jlong timeUs, jint option, jint width, jint height)
{
    MediaRetriever* retriever = getRetriever(env, thiz);
    if (retriever == nullptr) {
        jniThrowException(env, "java/lang/IllegalStateException", "No retriever available");
        return nullptr;
    }

    AVPacket packet;
    av_init_packet(&packet);
    jbyteArray array = nullptr;

    if (retriever->getScaledFrameAtTime(timeUs, option, &packet, width, height) == 0) {
        int size = packet.size;
        uint8_t* data = packet.data;
        array = env->NewByteArray(size);
        if (!array) {
            LOGE(LOG_TAG, "getFrameAtTime: OutOfMemoryError is thrown.");
        } else {
            jbyte* bytes = env->GetByteArrayElements(array, nullptr);
            if (bytes != nullptr) {
                memcpy(bytes, data, size);
                env->ReleaseByteArrayElements(array, bytes, 0);
            }
        }
    }

    av_packet_unref(&packet);

    return array;
}

RETRIEVER_FUNC(void, native_release)
{
    MediaRetriever* retriever = getRetriever(env, thiz);
    delete retriever;
    setRetriever(env, thiz, 0);
}