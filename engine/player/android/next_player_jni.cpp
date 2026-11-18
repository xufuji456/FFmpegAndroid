#if defined(__ANDROID__)

#include <cassert>
#include <cinttypes>
#include <cstring>
#include <jni.h>
#include <map>
#include <pthread.h>
#include <thread>
#include <unistd.h>

#include "android/JniEnv.h"
#include "android/JniUtil.h"
#include "NextPlayer.h"
#include "NextLog.h"

#ifdef __cplusplus
extern "C" {
#endif
#include "libavutil/log.h"
#ifdef __cplusplus
}
#endif


#define TAG "NextPlayerJNI"
#define JNI_CLASS_RSPLAYER_INTERFACE                                           \
  "com/frank/next/player/NextPlayer"

#define JNI_CHECK_RET_VOID(condition, msg)                                     \
do {                                                                           \
    if (!condition) {                                                          \
        NEXT_LOGE(TAG, "%s\n", msg);                                           \
        return;                                                                \
    }                                                                          \
} while (0)

#define JNI_CHECK_RET(condition, msg, ret)                                     \
do {                                                                           \
    if (!condition) {                                                          \
        NEXT_LOGE(TAG, "%s\n", msg);                                           \
        return ret;                                                            \
    }                                                                          \
} while (0)

static std::mutex *g_lock = new std::mutex();
static std::map<int, sp<NextPlayer>> *g_map = new std::map<int, sp<NextPlayer>>;

struct fields_t {
    jclass clazz;
    jfieldID log_cb_level;
    jmethodID post_event;
    jmethodID native_log;
};
static fields_t g_fields;

static int injectCallback(void *opaque, int what, void *data) { return 0; }

static void jniOnNativeLog(JNIEnv *env, jint level, jstring tag,
                           jbyteArray bytes) {
    return env->CallStaticVoidMethod(g_fields.clazz, g_fields.native_log, level,
                                     tag, bytes);
}

inline static void onNativeLog(JNIEnv *env, jint level, jstring tag,
                               jbyteArray bytes) {
    return jniOnNativeLog(env, level, tag, bytes);
}

static void nextLogCallback(int level, const char *tag, const char *buffer) {
    JNIEnv *env = nullptr;
    jstring jstr = nullptr;
    std::unique_ptr<JniEnvPtr> envPtr = std::make_unique<JniEnvPtr>();
    if (!envPtr) {
        return;
    }
    env = envPtr->Env();
    if (!env || !buffer) {
        return;
    }
    jbyteArray bytes = JniNewByteArrayGlobalRefCatch(env, (int)strlen(buffer));
    if (!bytes) {
        return;
    }
    env->SetByteArrayRegion(bytes, 0, (int)strlen(buffer), (jbyte *) buffer);
    if (JniCheckExceptionClear(env)) {
        goto end;
    }
    jstr = env->NewStringUTF(tag);
    if (JniCheckExceptionClear(env) || !jstr) {
        goto end;
    }
    onNativeLog(env, level, jstr, bytes);
end:
    JniDeleteLocalRefP(env, reinterpret_cast<jobject *>(&jstr));
    JniDeleteGlobalRefP(env, reinterpret_cast<jobject *>(&bytes));
}

static void postEventFromNative(JNIEnv *env, jobject weakObj,
                                int what, int arg1, int arg2, jobject obj) {
    if (!weakObj) {
        NEXT_LOGW(TAG, "%s null weakObj\n", __func__);
        return;
    }

    env->CallStaticVoidMethod(g_fields.clazz, g_fields.post_event, weakObj, what, arg1, arg2, obj);
}

inline static void postEvent(JNIEnv *env, jobject weakObj, jlong ctime, int what) {
    postEventFromNative(env, weakObj, what, -1, -1, nullptr);
}

inline static void postEvent(JNIEnv *env, jobject weakObj, jlong ctime,
                             int what, int arg1, int arg2) {
    postEventFromNative(env, weakObj, what, arg1, arg2, nullptr);
}

inline static void postEvent(JNIEnv *env, jobject weakObj, jlong ctime,
                             int what, int arg1, int arg2, jobject obj) {
    postEventFromNative(env, weakObj, what, arg1, arg2, obj);
}

static void messageLoopN(JNIEnv *env, NextPlayer *mp) {
    if (!mp) {
        NEXT_LOGE(TAG, "%s null mp\n", __func__);
        return;
    }
    auto weakObj = static_cast<jobject>(mp->getWeakThiz());

    while (true) {
        sp<AVMessage> msg = mp->getMessage(true);
        if (!msg)
            break;

        int64_t time = msg->mTime;
        int32_t arg1 = msg->mArg1, arg2 = msg->mArg2;
        void *obj1 = msg->mObj;

        switch (msg->mWhat) {
            case MSG_ON_FLUSH:
                postEvent(env, weakObj, static_cast<jlong>(time), MSG_ON_FLUSH, 0, 0);
                break;
            case MSG_ON_ERROR:
                NEXT_LOGE(TAG, "MSG_ON_ERROR: %d, %d", arg1, arg2);
                postEvent(env, weakObj, static_cast<jlong>(time), MSG_ON_ERROR, arg1, arg2);
                break;
            case MSG_ON_PREPARED:
                NEXT_LOGI(TAG, "MSG_ON_PREPARED");
                postEvent(env, weakObj, static_cast<jlong>(time), MSG_ON_PREPARED, 0, 0);
                break;
            case MSG_ON_COMPLETED:
                NEXT_LOGI(TAG, "MSG_ON_COMPLETED\n");
                postEvent(env, weakObj, static_cast<jlong>(time), MSG_ON_COMPLETED, 0, 0);
                break;
            case MSG_VIDEO_SIZE_CHANGED:
                NEXT_LOGI(TAG, "MSG_VIDEO_SIZE_CHANGED: %dx%d\n", arg1, arg2);
                postEvent(env, weakObj, static_cast<jlong>(time), MSG_VIDEO_SIZE_CHANGED, arg1, arg2);
                break;
            case MSG_SAR_CHANGED:
                NEXT_LOGI(TAG, "MSG_SAR_CHANGED: %d/%d\n", arg1, arg2);
                postEvent(env, weakObj, static_cast<jlong>(time), MSG_SAR_CHANGED, arg1, arg2);
                break;
            case MSG_VIDEO_RENDER_START:
                NEXT_LOGI(TAG, "MSG_VIDEO_RENDER_START\n");
                postEvent(env, weakObj, static_cast<jlong>(time), MSG_VIDEO_RENDER_START);
                break;
            case MSG_AUDIO_RENDER_START:
                NEXT_LOGI(TAG, "MSG_AUDIO_RENDER_START\n");
                postEvent(env, weakObj, static_cast<jlong>(time), MSG_MEDIA_INFO,
                          MSG_AUDIO_RENDER_START, 0);
                break;
            case MSG_ROTATION_CHANGED:
                NEXT_LOGI(TAG, "MSG_ROTATION_CHANGED: %d\n", arg1);
                postEvent(env, weakObj, static_cast<jlong>(time), MSG_MEDIA_INFO,
                          MSG_ROTATION_CHANGED, arg1);
                break;
            case MSG_AUDIO_DECODE_START:
                NEXT_LOGI(TAG, "MSG_AUDIO_DECODE_START\n");
                postEvent(env, weakObj, static_cast<jlong>(time), MSG_MEDIA_INFO,
                          MSG_AUDIO_DECODE_START, 0);
                break;
            case MSG_VIDEO_DECODE_START:
                NEXT_LOGI(TAG, "MSG_VIDEO_DECODE_START\n");
                postEvent(env, weakObj, static_cast<jlong>(time), MSG_MEDIA_INFO,
                          MSG_VIDEO_DECODE_START, 0);
                break;
            case MSG_OPEN_INPUT:
                NEXT_LOGI(TAG, "MSG_OPEN_INPUT\n");
                postEvent(env, weakObj, static_cast<jlong>(time), MSG_MEDIA_INFO, MSG_OPEN_INPUT, 0);
                break;
            case MSG_FIND_STREAM_INFO:
                NEXT_LOGI(TAG, "MSG_FIND_STREAM_INFO\n");
                postEvent(env, weakObj, static_cast<jlong>(time), MSG_MEDIA_INFO, MSG_FIND_STREAM_INFO, 0);
                break;
            case MSG_COMPONENT_OPEN:
                postEvent(env, weakObj, static_cast<jlong>(time), MSG_MEDIA_INFO, MSG_COMPONENT_OPEN, 0);
                break;
            case MSG_BUFFER_START:
                NEXT_LOGI(TAG, "MSG_BUFFER_START: %d\n", arg1);
                postEvent(env, weakObj, static_cast<jlong>(time), MSG_MEDIA_INFO, MSG_BUFFER_START, arg1);
                break;
            case MSG_BUFFER_END:
                NEXT_LOGI(TAG, "MSG_BUFFER_END: %d\n", arg1);
                postEvent(env, weakObj, static_cast<jlong>(time), MSG_MEDIA_INFO, MSG_BUFFER_END, arg1);
                break;
            case MSG_BUFFER_UPDATE:
                postEvent(env, weakObj, static_cast<jlong>(time), MSG_BUFFER_UPDATE, arg1, arg2);
                break;
            case MSG_SEEK_COMPLETE:
                NEXT_LOGI(TAG, "MSG_SEEK_COMPLETE: %d %d\n", arg1, arg2);
                postEvent(env, weakObj, static_cast<jlong>(time), MSG_SEEK_COMPLETE, arg1, arg2);
                break;
            case MSG_ACCURATE_SEEK_COMPLETE:
                NEXT_LOGI(TAG, "MSG_ACCURATE_SEEK_COMPLETE: %d\n", arg1);
                postEvent(env, weakObj, static_cast<jlong>(time), MSG_MEDIA_INFO,
                          MSG_ACCURATE_SEEK_COMPLETE, arg1);
                break;
            case MSG_PLAY_STATE_CHANGED:
                postEvent(env, weakObj, static_cast<jlong>(time), MSG_PLAY_STATE_CHANGED, arg1, arg2);
                break;
            case MSG_SUBTITLE_UPDATE:
                if (obj1) {
                    jstring text = env->NewStringUTF(static_cast<char *>(obj1));
                    postEvent(env, weakObj, static_cast<jlong>(time), MSG_SUBTITLE_UPDATE, 0, 0,
                              text);
                    JniDeleteLocalRefP(env, reinterpret_cast<jobject *>(&text));
                } else {
                    postEvent(env, weakObj, static_cast<jlong>(time), MSG_SUBTITLE_UPDATE, 0, 0,
                              nullptr);
                }
                break;
            case MSG_VIDEO_SEEK_RENDER_START:
                NEXT_LOGI(TAG, "MSG_VIDEO_SEEK_RENDER_START: %d\n", arg1);
                postEvent(env, weakObj, static_cast<jlong>(time), MSG_MEDIA_INFO,
                          MSG_VIDEO_SEEK_RENDER_START, arg1);
                break;
            case MSG_AUDIO_SEEK_RENDER_START:
                NEXT_LOGI(TAG, "MSG_AUDIO_SEEK_RENDER_START: %d\n", arg1);
                postEvent(env, weakObj, static_cast<jlong>(time), MSG_MEDIA_INFO,
                          MSG_AUDIO_SEEK_RENDER_START, arg1);
                break;
            case MSG_VIDEO_FIRST_PACKET:
                NEXT_LOGI(TAG, "MSG_VIDEO_FIRST_PACKET\n");
                postEvent(env, weakObj, static_cast<jlong>(time), MSG_MEDIA_INFO,
                          MSG_VIDEO_FIRST_PACKET, 0);
                break;
            default:
                NEXT_LOGE(TAG, "unknown msg code %d\n", msg->mWhat);
                break;
        }
        mp->recycleMessage(msg);
    }
}

static int32_t messageLoop(NextPlayer *mp) {
    if (!mp) {
        NEXT_LOGE(TAG, "%s: null mp\n", __func__);
        return ERROR_PLAYER_NOT_INIT;
    }

    std::unique_ptr<JniEnvPtr> env = std::make_unique<JniEnvPtr>();
    if (!env || !env->Env()) {
        NEXT_LOGE(TAG, "%s: null Env\n", __func__);
        return ERROR_OTHER_OOM;
    }

    messageLoopN(env->Env(), mp);

    return RESULT_OK;
}

static sp<NextPlayer> setPlayer(JNIEnv *env, jobject thiz, sp<NextPlayer> &mp) {
    int id = 0;
    sp<NextPlayer> old;

    jfieldID jid = env->GetFieldID(g_fields.clazz, "mPlayerId", "I");
    if (!jid) {
        NEXT_LOGE("%s Failed to get playerId\n", __func__);
        return old;
    }
    id = static_cast<int>(env->GetIntField(thiz, jid));

    if (id <= 0) {
        NEXT_LOGE(TAG, "%s Invalid player id!\n", __func__);
        return old;
    }

    std::lock_guard<std::mutex> lock(*g_lock);
    auto it = (*g_map).find(id);
    if (it != (*g_map).end()) {
        old = it->second;
        (*g_map).erase(it);
    }
    if (mp) {
        (*g_map).emplace(id, mp);
    }

    return old;
}

static sp<NextPlayer> getPlayer(JNIEnv *env, jobject thiz) {
    int id = 0;
    sp<NextPlayer> mp;

    jfieldID jid = env->GetFieldID(g_fields.clazz, "mPlayerId", "I");
    if (!jid) {
        NEXT_LOGE("%s Failed to get playerId\n", __func__);
        return mp;
    }
    id = static_cast<int>(env->GetIntField(thiz, jid));

    if (id <= 0) {
        NEXT_LOGE(TAG, "%s Invalid player id!\n", __func__);
        return mp;
    }

    std::lock_guard<std::mutex> lock(*g_lock);
    auto it = (*g_map).find(id);
    if (it != (*g_map).end()) {
        mp = it->second;
    }

    return mp;
}

static void nativeInit(JNIEnv *env, jclass thiz) {
    jclass clazz;

    clazz = env->FindClass(JNI_CLASS_RSPLAYER_INTERFACE);
    if (!clazz) {
        NEXT_LOGE("%s Failed to get class\n", __func__);
        return;
    }
    g_fields.clazz = static_cast<jclass>(env->NewGlobalRef(clazz));

    g_fields.log_cb_level = env->GetStaticFieldID(clazz, "gLogCallBackLevel", "I");
    if (!g_fields.log_cb_level) {
        NEXT_LOGE("%s Failed to get gLogCallBackLevel\n", __func__);
        return;
    }
    jint level = env->GetStaticIntField(g_fields.clazz, g_fields.log_cb_level);
    if (JniCheckExceptionClear(env)) {
        level = 0;
    }
    setLogCallbackLevel(static_cast<int>(level));

    g_fields.post_event = env->GetStaticMethodID(clazz, "postEventFromNative",
                                                 "(Ljava/lang/Object;IIILjava/lang/Object;)V");
    if (!g_fields.post_event) {
        NEXT_LOGE("%s Failed to get post_event method\n", __func__);
        return;
    }

    g_fields.native_log = env->GetStaticMethodID(clazz, "onNativeLog", "(ILjava/lang/String;[B)V");
    if (!g_fields.native_log) {
        NEXT_LOGE("%s Failed to get native_log method\n", __func__);
        return;
    }
    setLogCallback(nextLogCallback);

    env->DeleteLocalRef(clazz);
}

static void nativeSetup(JNIEnv *env, jobject thiz, jobject weakObj) {
    int id = 0;
    jfieldID jid = env->GetFieldID(g_fields.clazz, "mPlayerId", "I");
    if (!jid) {
        NEXT_LOGE("%s Failed to get playerId\n", __func__);
        return;
    }
    id = static_cast<int>(env->GetIntField(thiz, jid));
    if (id <= 0) {
        NEXT_LOGE(TAG, "%s Invalid player id!\n", __func__);
        return;
    }
    sp<NextPlayer> mp =
            NextPlayer::create(id, std::bind(&messageLoop, std::placeholders::_1));
    JNI_CHECK_RET_VOID(mp, "nativeSetup: oom");
    setPlayer(env, thiz, mp);
    mp->setWeakThiz(static_cast<void *>(env->NewGlobalRef(weakObj)));
    mp->setInjectOpaque(mp->getWeakThiz());
}

static void nativeSetSurface(JNIEnv *env, jobject thiz,
                             jobject jsurface) {
    sp<NextPlayer> mp = getPlayer(env, thiz);
    JNI_CHECK_RET_VOID(mp, "nativeSetSurface: player is nullptr");

    mp->setVideoSurface(env, jsurface);
}

static void nativeSetDataSource(JNIEnv *env, jobject thiz, jstring path) {
    sp<NextPlayer> mp = getPlayer(env, thiz);
    JNI_CHECK_RET_VOID(mp, "nativeSetDataSource: player is nullptr");
    JNI_CHECK_RET_VOID(path, "nativeSetDataSource: path is nullptr");
    std::string c_path = JniGetStringUTFChars(env, path);
    mp->setDataSource(c_path);
}

static void nativePrepareAsync(JNIEnv *env, jobject thiz) {
    int32_t ret = 0;
    sp<NextPlayer> mp = getPlayer(env, thiz);

    JNI_CHECK_RET_VOID(mp, "nativePrepareAsync: player is nullptr");

    ret = mp->prepareAsync();
    if (ret < RESULT_OK) {
        NEXT_LOGE(TAG, "nativePrepareAsync fail, ret=%d\n", ret);
    }
}

static void nativeStart(JNIEnv *env, jobject thiz) {
    sp<NextPlayer> mp = getPlayer(env, thiz);
    JNI_CHECK_RET_VOID(mp, "Start: player is nullptr");
    mp->start();
}

static void nativePause(JNIEnv *env, jobject thiz) {
    sp<NextPlayer> mp = getPlayer(env, thiz);
    JNI_CHECK_RET_VOID(mp, "Pause: player is nullptr");
    mp->pause();
}

static void nativeStop(JNIEnv *env, jobject thiz) {
    sp<NextPlayer> mp = getPlayer(env, thiz);
    JNI_CHECK_RET_VOID(mp, "stop: player is nullptr");
    mp->stop();
}

static jboolean nativeIsPlaying(JNIEnv *env, jobject thiz) {
    jboolean ret = JNI_FALSE;
    sp<NextPlayer> mp = getPlayer(env, thiz);
    JNI_CHECK_RET(mp, "isPlaying: player is nullptr", ret);

    ret = mp->isPlaying() ? JNI_TRUE : JNI_FALSE;

    return ret;
}

static void nativeSeekTo(JNIEnv *env, jobject thiz, jlong msec) {
    sp<NextPlayer> mp = getPlayer(env, thiz);
    JNI_CHECK_RET_VOID(mp, "seekTo: player is nullptr");
    mp->seekTo(msec);
}

static void nativeRelease(JNIEnv *env, jobject thiz) {
    sp<NextPlayer> mp = getPlayer(env, thiz);
    if (!mp) {
        NEXT_LOGW(TAG, "%s Player already remove\n", __func__);
        return;
    }
    NEXT_LOGI(TAG, "stop begin...");
    mp->stop();
    mp->setVideoSurface(env, nullptr);
    mp->setInjectOpaque(nullptr);
    auto weakObj = static_cast<jobject>(mp->setWeakThiz(nullptr));
    sp<NextPlayer> empty = nullptr;
    setPlayer(env, thiz, empty);

    // Use worker thread to release
    auto releaseTask = [](sp<NextPlayer> player, jobject weak) {
        NEXT_LOGI(TAG, "release begin...");
        player->release();
        JniEnvPtr jniEnv;
        if (weak && jniEnv.Env()) {
            jniEnv->DeleteGlobalRef(weak);
        }
        player.reset();
        NEXT_LOGI(TAG, "release end...");
    };
    try {
        std::thread(releaseTask, mp, weakObj).detach();
    } catch (const std::system_error &e) {
        mp->release();
        if (weakObj) {
            env->DeleteGlobalRef(weakObj);
        }
    }
}

static void nativeReset(JNIEnv *env, jobject thiz) {
    sp<NextPlayer> mp = getPlayer(env, thiz);
    if (!mp) {
        NEXT_LOGW(TAG, "nativeReset Player is nullptr\n");
        return;
    }
    auto weakObj = static_cast<jobject>(mp->setWeakThiz(nullptr));

    nativeRelease(env, thiz);
    nativeSetup(env, thiz, weakObj);
}

static void nativeSetHeaders(JNIEnv *env, jobject thiz, jstring headers) {
    sp<NextPlayer> mp = getPlayer(env, thiz);
    JNI_CHECK_RET_VOID(headers, "setHeaders: headers is nullptr");
    JNI_CHECK_RET_VOID(mp, "setHeaders: player is nullptr");

    std::string c_headers = JniGetStringUTFChars(env, headers);
    JNI_CHECK_RET_VOID(!c_headers.empty(), "setHeaders: headers oom");
    mp->setConfig(CONFIG_TYPE_FORMAT, "headers", c_headers);
}

static void nativeSetSpeed(JNIEnv *env, jobject thiz, jfloat speed) {
    sp<NextPlayer> mp = getPlayer(env, thiz);
    JNI_CHECK_RET_VOID(mp, "setSpeed: player is nullptr");

    mp->setOption(OPTION_FLOAT_PLAYBACK_RATE, speed);
}

static void nativeSetVolume(JNIEnv *env, jobject thiz, jfloat left_volume, jfloat right_volume) {
    sp<NextPlayer> mp = getPlayer(env, thiz);
    JNI_CHECK_RET_VOID(mp, "setVolume: player is nullptr");
    mp->setVolume(left_volume, right_volume);
}

static void nativeSetEnableMediaCodec(JNIEnv *env, jobject thiz, jboolean enable) {
    sp<NextPlayer> mp = getPlayer(env, thiz);
    JNI_CHECK_RET_VOID(mp, "setEnableMediaCodec: player is nullptr");

    mp->setConfig(CONFIG_TYPE_PLAYER, "mediacodec-all-videos", static_cast<int64_t>(enable));
    mp->setConfig(CONFIG_TYPE_PLAYER, "enable-mediacodec", static_cast<int64_t>(enable));
}

static void nativeSetCacheDir(JNIEnv *env, jobject thiz, jstring dir) {
    sp<NextPlayer> mp = getPlayer(env, thiz);
    JNI_CHECK_RET_VOID(dir, "setVideoCacheDir: dir is nullptr");
    JNI_CHECK_RET_VOID(mp, "setVideoCacheDir: player is nullptr");

    std::string c_dir = JniGetStringUTFChars(env, dir);
    JNI_CHECK_RET_VOID(!c_dir.empty(), "setVideoCacheDir: dir oom");
    mp->setConfig(CONFIG_TYPE_FORMAT, "cache_file_dir", c_dir);
}

static jlong nativeGetCurrentPosition(JNIEnv *env, jobject thiz) {
    jlong position = 0;
    sp<NextPlayer> mp = getPlayer(env, thiz);
    JNI_CHECK_RET(mp, "getCurrentPosition: player is nullptr", 0);
    mp->getCurrentPosition(position);
    return position;
}

static jlong nativeGetDuration(JNIEnv *env, jobject thiz) {
    jlong duration = 0;
    sp<NextPlayer> mp = getPlayer(env, thiz);
    JNI_CHECK_RET(mp, "getDuration: player is nullptr", 0);
    mp->getDuration(duration);
    return duration;
}

static jstring getVideoCodecInfo(JNIEnv *env, jobject thiz) {
    jstring jcodec_info = nullptr;
    int32_t ret = RESULT_OK;
    std::string codec_info;
    sp<NextPlayer> mp = getPlayer(env, thiz);

    JNI_CHECK_RET(mp, "getVideoCodecInfo: player is nullptr", jcodec_info);
    ret = mp->getVideoCodecInfo(codec_info);
    if (ret != RESULT_OK || codec_info.empty())
        return jcodec_info;

    jcodec_info = env->NewStringUTF(codec_info.c_str());

    return jcodec_info;
}

static jstring getAudioCodecInfo(JNIEnv *env, jobject thiz) {
    jstring jcodec_info = nullptr;
    int32_t ret = RESULT_OK;
    std::string codec_info;
    sp<NextPlayer> mp = getPlayer(env, thiz);

    JNI_CHECK_RET(mp, "getAudioCodecInfo: player is nullptr", jcodec_info);
    ret = mp->getAudioCodecInfo(codec_info);
    if (ret != RESULT_OK || codec_info.empty())
        return jcodec_info;

    jcodec_info = env->NewStringUTF(codec_info.c_str());

    return jcodec_info;
}

static jstring getPlayUrl(JNIEnv *env, jobject thiz) {
    jstring jurl = nullptr;
    int32_t ret = RESULT_OK;
    std::string url;
    sp<NextPlayer> mp = getPlayer(env, thiz);

    JNI_CHECK_RET(mp, "getPlayUrl: player is nullptr", jurl);
    ret = mp->getPlayUrl(url);
    if (ret != RESULT_OK || url.empty())
        return jurl;

    jurl = env->NewStringUTF(url.c_str());

    return jurl;
}

static jint nativeGetPlayerState(JNIEnv *env, jobject thiz) {
    int state = -1;
    sp<NextPlayer> mp = getPlayer(env, thiz);
    JNI_CHECK_RET(mp, "nativeGetPlayerState: player is nullptr", state);
    state = mp->getPlayerState();

    return state;
}

static jfloat nativeGetFrameRate(JNIEnv *env, jobject thiz) {
    jfloat ret = 0.0;
    sp<NextPlayer> mp = getPlayer(env, thiz);
    JNI_CHECK_RET(mp, "nativeGetFrameRate: player is nullptr", ret);

    ret = static_cast<jfloat>(
            mp->getOption(OPTION_FLOAT_VIDEO_FRAME_RATE, 0.0f));
    return ret;
}

static jlong nativeGetOptionLong(JNIEnv *env, jobject thiz, int option) {
    sp<NextPlayer> mp = getPlayer(env, thiz);
    JNI_CHECK_RET(mp, "nativeGetOptionLong: player is nullptr", 0);
    return mp->getOption(option, 0L);
}

static JNINativeMethod g_methods[] = {
        {"native_init",                    "()V", reinterpret_cast<void *>(nativeInit)},
        {"native_setup",                   "(Ljava/lang/Object;)V", reinterpret_cast<void *>(nativeSetup)},
        {"_setVideoSurface",               "(Landroid/view/Surface;)V", reinterpret_cast<void *>(nativeSetSurface)},
        {"_setDataSource",                 "(Ljava/lang/String;)V", reinterpret_cast<void *>(nativeSetDataSource)},
        {"_prepareAsync",                  "()V", reinterpret_cast<void *>(nativePrepareAsync)},
        {"_start",                         "()V", reinterpret_cast<void *>(nativeStart)},
        {"_stop",                          "()V", reinterpret_cast<void *>(nativeStop)},
        {"_seekTo",                        "(J)V", reinterpret_cast<void *>(nativeSeekTo)},
        {"_pause",                         "()V", reinterpret_cast<void *>(nativePause)},
        {"_playing",                       "()Z", reinterpret_cast<void *>(nativeIsPlaying)},
        {"_getCurrentPosition",            "()J", reinterpret_cast<void *>(nativeGetCurrentPosition)},
        {"_getDuration",                   "()J", reinterpret_cast<void *>(nativeGetDuration)},
        {"_release",                       "()V", reinterpret_cast<void *>(nativeRelease)},
        {"_reset",                         "()V", reinterpret_cast<void *>(nativeReset)},
        {"_getVideoCodecInfo",             "()Ljava/lang/String;", reinterpret_cast<void *>(getVideoCodecInfo)},
        {"_getAudioCodecInfo",             "()Ljava/lang/String;", reinterpret_cast<void *>(getAudioCodecInfo)},
        {"_getPlayUrl",                    "()Ljava/lang/String;", reinterpret_cast<void *>(getPlayUrl)},
        {"_getPlayerState",                "()I", reinterpret_cast<void *>(nativeGetPlayerState)},
        {"_setVolume",                     "(FF)V", reinterpret_cast<void *>(nativeSetVolume)},
        {"_setEnableMediaCodec",           "(Z)V", reinterpret_cast<void *>(nativeSetEnableMediaCodec)},
        {"_setVideoCacheDir",              "(Ljava/lang/String;)V", reinterpret_cast<void *>(nativeSetCacheDir)},
        {"_getVideoFileFps",               "()F", reinterpret_cast<void *>(nativeGetFrameRate)},
        {"_setHeaders",                    "(Ljava/lang/String;)V", reinterpret_cast<void *>(nativeSetHeaders)},
        {"_setSpeed",                      "(F)V", reinterpret_cast<void *>(nativeSetSpeed)},
        {"_getOptionLong",                 "(I)J", reinterpret_cast<void *>(nativeGetOptionLong)}};

extern "C" JNIEXPORT jint JNI_OnLoad(JavaVM *jvm, void *reserved) {
    JNIEnv *env;
    if (JNI_OK != jvm->GetEnv(reinterpret_cast<void **>(&env), JNI_VERSION_1_6)) {
        return JNI_ERR;
    }

    JniEnvPtr::GlobalInit(jvm);

    jclass clazz = env->FindClass(JNI_CLASS_RSPLAYER_INTERFACE);

    if (env->RegisterNatives(clazz, g_methods,
                             sizeof(g_methods) / sizeof((g_methods)[0])) < 0) {
        return JNI_ERR;
    }

    globalInit();
    globalSetInjectCallback(injectCallback);

    return JNI_VERSION_1_6;
}

extern "C" JNIEXPORT void JNI_OnUnload(JavaVM *jvm, void *reserved) {
    JNIEnv *env;
    if (JNI_OK != jvm->GetEnv(reinterpret_cast<void **>(&env), JNI_VERSION_1_6)) {
        return;
    }

    globalUninit();
    env->DeleteGlobalRef(g_fields.clazz);
}

#endif
