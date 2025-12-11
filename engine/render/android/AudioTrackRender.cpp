/**
 * Note: render of AudioTrack
 * Date: 2025/12/11
 * Author: frank
 */

#if defined(__ANDROID__)

#include "AudioTrackRender.h"

#include <thread>

#include "android/JniEnv.h"
#include "NextLog.h"

#ifdef __cplusplus
extern "C" {
#endif
#include "libavutil/samplefmt.h"
#ifdef __cplusplus
}
#endif

#define AUDIO_TRACK_RENDER "AudioTrackRender"

AudioTrackRender::AudioTrackRender()
        : AudioRender() {}

AudioTrackRender::~AudioTrackRender() {
    if (mBuffer) {
        delete[] mBuffer;
        mBuffer = nullptr;
    }
    JniEnvPtr jni_env;
    JNIEnv *env = jni_env.Env();
    if (!env) {
        NEXT_LOGE(AUDIO_TRACK_RENDER, "release, JNIEnv is null!");
        return;
    }
    if (mByteBuffer) {
        mByteBufferCapacity = 0;
        JniDeleteGlobalRefP(env, reinterpret_cast<jobject *>(&mByteBuffer));
    }
    if (mAudioTrack) {
        AudioTrackJni::AudioTrackRelease(env, mAudioTrack);
        JniDeleteGlobalRefP(env, reinterpret_cast<jobject *>(&mAudioTrack));
    }
}

void AudioTrackRender::AdjustAudioInfo(const AudioRenderInfo &expect, AudioRenderInfo &actual) {
    actual = expect;
    if (expect.sample_rate < 4000 || expect.sample_rate > 48000) {
        actual.sample_rate = 44100;
    }
    if (expect.channels > 2) {
        actual.channels = 2;
    }
    if (actual.format != AV_SAMPLE_FMT_S16) {
        actual.format = AV_SAMPLE_FMT_S16;
    }
}

int AudioTrackRender::OpenAudio(const AudioRenderInfo &expect, AudioRenderInfo &actual,
                                std::unique_ptr<AudioCallback> &audioCallback) {
    AdjustAudioInfo(expect, actual);
    int streamType = AUDIO_STREAM_TYPE_MUSIC;
    int audioFormat = static_cast<int>(AudioTrackFormat::ENCODING_PCM_16BIT);
    int channelConfig = 0;
    if (actual.channels == 2) {
        channelConfig = static_cast<int>(AudioTrackChannelConfig::CHANNEL_CONFIG_STEREO);
    } else if (actual.channels == 1) {
        channelConfig = static_cast<int>(AudioTrackChannelConfig::CHANNEL_CONFIG_MONO);
    } else {
        RS_LOGE(AUDIO_TRACK_RENDER, "don't support channels: %d", actual.channels);
        return -1;
    }
    int sampleRate = actual.sample_rate;
    int mode = AUDIO_TRACK_MODE_STREAM;
    JniEnvPtr jni_env;
    JNIEnv *env = jni_env.Env();
    if (!env) {
        NEXT_LOGE(AUDIO_TRACK_RENDER, "OpenAudio error, JNIEnv is null!");
        return -1;
    }
    int bufferSize = AudioTrackJni::AudioTrackGetMinBufferSize(
            env, sampleRate, channelConfig, audioFormat);
    if (bufferSize <= 0) {
        NEXT_LOGE(AUDIO_TRACK_RENDER, "invalid buffer size: %d", bufferSize);
        return -1;
    }

    bufferSize *= AUDIO_TRACK_MAX_SPEED;
    mAudioTrack = AudioTrackJni::AudioTrackAsGlobalRef(
            env, streamType, sampleRate, channelConfig, audioFormat,
            bufferSize, mode);
    mAudioSessionId =
            AudioTrackJni::AudioTrackGetAudioSessionId(env, mAudioTrack);
    mBufferSize = bufferSize;
    actual.size = bufferSize;
    mBuffer = new uint8_t[bufferSize];
    mAudioCallback = std::move(audioCallback);
    std::unique_lock<std::mutex> mutex(mThreadMutex);

    if (!bAbortRequest && !mAudioThread) {
        try {
            mAudioThread = new std::thread(AudioLoop, this);
        } catch (const std::system_error &e) {
            mAudioThread = nullptr;
            NEXT_LOGE(AUDIO_TRACK_RENDER, "OpenAudio error: %s", e.what());
            return -1;
        }
    }
    return 0;
}

int AudioTrackRender::AudioLoop(AudioTrackRender *pRender) {
    int copySize = 256;
    JniEnvPtr jni_env;
    JNIEnv *env = jni_env.Env();

    if (!pRender->bAbortRequest && !pRender->bIsPaused)
        AudioTrackJni::AudioTrackPlay(env, pRender->mAudioTrack);

    while (!pRender->bAbortRequest) {
        {
            std::unique_lock<std::mutex> mutex(pRender->mMutex);
            if (!pRender->bAbortRequest && pRender->bIsPaused) {
                AudioTrackJni::AudioTrackPause(env, pRender->mAudioTrack);
                while (!pRender->bAbortRequest && pRender->bIsPaused) {
                    pRender->mWakeupCond.wait_for(mutex, std::chrono::milliseconds(1000));
                }
                if (!pRender->bAbortRequest && !pRender->bIsPaused) {
                    if (pRender->bNeedFlush) {
                        pRender->bNeedFlush = false;
                        AudioTrackJni::AudioTrackFlush(env, pRender->mAudioTrack);
                    }
                    AudioTrackJni::AudioTrackPlay(env, pRender->mAudioTrack);
                } else if (pRender->bAbortRequest) {
                    break;
                }
            }

            if (pRender->bNeedFlush) {
                pRender->bNeedFlush = false;
                AudioTrackJni::AudioTrackFlush(env, pRender->mAudioTrack);
            }
            if (pRender->bSetVolume) {
                pRender->bSetVolume = false;
                AudioTrackJni::AudioTrackSetVolume(
                        env, pRender->mAudioTrack, pRender->mLeftVolume, pRender->mRightVolume);
            }
            if (pRender->bChangeSpeed) {
                pRender->bChangeSpeed = false;
                AudioTrackJni::AudioTrackSetSpeed(env, pRender->mAudioTrack, pRender->mCurrentSpeed);
            }
        }
        pRender->mAudioCallback->GetBuffer(pRender->mBuffer, copySize);
        {
            std::unique_lock<std::mutex> mutex(pRender->mMutex);
            if (pRender->bNeedFlush) {
                pRender->bNeedFlush = false;
                AudioTrackJni::AudioTrackFlush(env, pRender->mAudioTrack);
            }
        }

        if (!pRender->mByteBuffer || copySize > pRender->mByteBufferCapacity) {
            if (pRender->mByteBuffer) {
                JniDeleteGlobalRefP(env, reinterpret_cast<jobject *>(&pRender->mByteBuffer));
            }
            pRender->mByteBufferCapacity = 0;
            int capacity = std::max(copySize, pRender->mBufferSize);
            pRender->mByteBuffer = JniNewByteArrayGlobalRefCatch(env, capacity);
            if (!pRender->mByteBuffer) {
                NEXT_LOGE(AUDIO_TRACK_RENDER, "pRender->mByteBuffer is null!");
                return -1;
            }
            pRender->mByteBufferCapacity = capacity;
        }
        env->SetByteArrayRegion(pRender->mByteBuffer, 0, copySize,
                                reinterpret_cast<jbyte *>(pRender->mBuffer));
        if (JniCheckExceptionClear(env)) {
            NEXT_LOGE(AUDIO_TRACK_RENDER, "SetByteArrayRegion failed!");
            return -1;
        }
        // write data to AudioTrack
        int written = AudioTrackJni::AudioTrackWrite(
                env, pRender->mAudioTrack, pRender->mByteBuffer, 0, copySize);
        if (written != copySize) {
            NEXT_LOGI(AUDIO_TRACK_RENDER, "written=%d, copySize=%d", written, copySize);
        }
    }
    return 0;
}

void AudioTrackRender::PauseAudio(bool paused) {
    std::unique_lock<std::mutex> mutex(mMutex);
    bIsPaused = paused;
    if (!bIsPaused) {
        mWakeupCond.notify_one();
    }
}

void AudioTrackRender::FlushAudio() {
    std::unique_lock<std::mutex> mutex(mMutex);
    bNeedFlush = true;
    mWakeupCond.notify_one();
}

double AudioTrackRender::GetDelay() {
    return mMinLatency;
}

void AudioTrackRender::SetDefaultDelay(double latency) {
    mMinLatency = latency;
}

int AudioTrackRender::GetAudioCallBack() {
    return kAudioMaxCallbacksPerSec;
}

void AudioTrackRender::SetPlaybackRate(float playbackRate) {
    std::unique_lock<std::mutex> mutex(mMutex);
    mCurrentSpeed = playbackRate;
    bChangeSpeed  = true;
    mWakeupCond.notify_one();
}

void AudioTrackRender::SetPlaybackVolume(float volume) {
    std::unique_lock<std::mutex> mutex(mMutex);
    mLeftVolume  = volume;
    mRightVolume = volume;
    bSetVolume   = true;
}

int AudioTrackRender::GetAudioSessionId() {
    return mAudioSessionId;
}

void AudioTrackRender::CloseAudio(bool waiting) {
    std::unique_lock<std::mutex> mutex(mMutex);
    bAbortRequest = true;
    mWakeupCond.notify_one();
    mutex.unlock();

    if (waiting) {
        std::unique_lock<std::mutex> threadMutex(mThreadMutex);
        if (mAudioThread && mAudioThread->joinable()) {
            mAudioThread->join();
            delete mAudioThread;
            mAudioThread = nullptr;
        }
    }
}

#endif
