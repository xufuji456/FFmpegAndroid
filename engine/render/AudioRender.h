/**
 * Note: interface of audio render
 * Date: 2025/12/10
 * Author: frank
 */

#ifndef AUDIO_RENDER_H
#define AUDIO_RENDER_H

#include <memory>

#include "AudioRenderInfo.h"

class AudioRender {
public:
    AudioRender() = default;

    virtual ~AudioRender() = default;

    virtual int OpenAudio(const AudioRenderInfo &expect, AudioRenderInfo &actual,
                          std::unique_ptr<AudioCallback> &audioCallback) = 0;

    virtual void PauseAudio(bool paused) = 0;

    virtual void FlushAudio() = 0;

    virtual double GetDelay() = 0;

    virtual void SetDefaultDelay(double latency) = 0;

    virtual int GetAudioCallBack() = 0;

    virtual void SetPlaybackRate(float playbackRate) = 0;

    virtual void SetPlaybackVolume(float volume) = 0;

    virtual int GetAudioSessionId() = 0;

    virtual void CloseAudio(bool waiting) = 0;

protected:
    std::unique_ptr<AudioCallback> mAudioCallback = nullptr;
};

#endif
