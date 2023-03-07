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
static const char* mClassName = "com/frank/ffmpeg/metadata/FFmpegMediaRetriever";
static const char* mNoAvailableMsg = "No retriever available";
static const char* mIllegalStateException = "java/lang/IllegalStateException";
static const char* mIllegalArgException = "java/lang/IllegalArgumentException";

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
        jniThrowException(env, mIllegalStateException, nullptr);
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

RETRIEVER_FUNC(void, native_1setup)
{
    auto* retriever = new MediaRetriever();
    setRetriever(env, thiz, (long)retriever);
}

RETRIEVER_FUNC(void, native_1init)
{
    jclass clazz = env->FindClass(mClassName);
    if (!clazz) {
        return;
    }

    fields.context = env->GetFieldID(clazz, "mNativeRetriever", "J");
    if (fields.context == nullptr) {
        return;
    }

    avformat_network_init();
}

RETRIEVER_FUNC(void, native_1setDataSource, jstring path) {
    MediaRetriever* retriever = getRetriever(env, thiz);
    if (retriever == nullptr) {
        jniThrowException(env, mIllegalStateException, mNoAvailableMsg);
        return;
    }
    if (!path) {
        jniThrowException(env, mIllegalArgException, "Null of path");
        return;
    }
    const char *tmp = env->GetStringUTFChars(path, nullptr);
    if (!tmp) {
        return;
    }

    process_retriever_call(env, retriever->setDataSource(tmp),
            mIllegalArgException,"setDataSource failed");
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

RETRIEVER_FUNC(void, native_1setDataSourceFD, jobject fileDescriptor, jlong offset, jlong length)
{
    if (offset < 0 || length < 0) {
        return;
    }
    MediaRetriever* retriever = getRetriever(env, thiz);
    if (retriever == nullptr) {
        jniThrowException(env, mIllegalStateException, mNoAvailableMsg);
        return;
    }
    if (!fileDescriptor) {
        jniThrowException(env, mIllegalArgException, nullptr);
        return;
    }
    int fd = getFileDescriptor(env, fileDescriptor);
    if (fd < 0) {
        LOGE(LOG_TAG, "invalid file descriptor!");
        jniThrowException(env, mIllegalArgException, nullptr);
        return;
    }
    process_retriever_call(env, retriever->setDataSource(fd, offset, length),
            "java/lang/IOException", "setDataSource failed");
}

RETRIEVER_FUNC(void, native_1setSurface, jobject surface)
{
    MediaRetriever* retriever = getRetriever(env, thiz);
    if (retriever == nullptr) {
        jniThrowException(env, mIllegalStateException, mNoAvailableMsg);
        return;
    }
    ANativeWindow *mNativeWindow = ANativeWindow_fromSurface(env, surface);
    if (mNativeWindow != nullptr) {
        retriever->setNativeWindow(mNativeWindow);
    }
}

RETRIEVER_FUNC(jobject, native_1extractMetadata, jstring jkey)
{
    MediaRetriever* retriever = getRetriever(env, thiz);
    if (retriever == nullptr) {
        jniThrowException(env, mIllegalStateException, mNoAvailableMsg);
        return nullptr;
    }
    if (!jkey) {
        jniThrowException(env, mIllegalArgException, "Null of key");
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

RETRIEVER_FUNC(jbyteArray, native_1getFrameAtTime, jlong timeUs, jint option)
{
    MediaRetriever* retriever = getRetriever(env, thiz);
    if (retriever == nullptr) {
        jniThrowException(env, mIllegalStateException, mNoAvailableMsg);
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

RETRIEVER_FUNC(jbyteArray, native_1getScaleFrameAtTime, jlong timeUs, jint option, jint width, jint height)
{
    MediaRetriever* retriever = getRetriever(env, thiz);
    if (retriever == nullptr) {
        jniThrowException(env, mIllegalStateException, mNoAvailableMsg);
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

RETRIEVER_FUNC(jbyteArray, native_1getAudioThumbnail)
{
    MediaRetriever* retriever = getRetriever(env, thiz);
    if (retriever == nullptr) {
        jniThrowException(env, mIllegalStateException, mNoAvailableMsg);
        return nullptr;
    }

    AVPacket packet;
    av_init_packet(&packet);
    jbyteArray array = nullptr;

    if (retriever->getAudioThumbnail(&packet) == 0) {
        int size = packet.size;
        uint8_t* data = packet.data;
        array = env->NewByteArray(size);
        if (!array) {
            LOGE(LOG_TAG, "getAudioThumbnail: OutOfMemoryError is thrown.");
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

RETRIEVER_FUNC(void, native_1release)
{
    MediaRetriever* retriever = getRetriever(env, thiz);
    delete retriever;
    setRetriever(env, thiz, 0);
}