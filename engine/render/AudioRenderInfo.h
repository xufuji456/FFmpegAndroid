/**
 * Note: information of audio render
 * Date: 2025/12/9
 * Author: frank
 */

#ifndef AUDIO_RENDER_INFO_H
#define AUDIO_RENDER_INFO_H

#include <cstdint>
#include <iostream>

#ifdef __cplusplus
extern "C" {
#endif
#include "libavutil/channel_layout.h"
#ifdef __cplusplus
}
#endif

const int kAudioMinBufferSize      = 512;
const int kAudioQueueBufferCount   = 3;
const int kAudioMaxCallbacksPerSec = 30;

struct AudioRenderInfo {
    int channels;
    int sample_rate;
    AVChannelLayout *channel_layout;

    int format;
    uint32_t size;
    uint8_t silence;
    uint16_t samples;

    AudioRenderInfo &operator=(const AudioRenderInfo &info) {
        this->size           = info.size;
        this->format         = info.format;
        this->samples        = info.samples;
        this->silence        = info.silence;
        this->channels       = info.channels;
        this->sample_rate    = info.sample_rate;
        this->channel_layout = info.channel_layout;
        return *this;
    }
};

class AudioCallback {
public:
    explicit AudioCallback(void *userData) : mUserData(userData) {

    }

    virtual ~AudioCallback() = default;

    virtual void GetBuffer(uint8_t *stream, int len) {}

protected:
    void *mUserData = nullptr;
};

#endif
