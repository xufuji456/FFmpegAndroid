//
// Created by xu fulong on 2022/9/4.
//

#ifndef FF_AUDIO_PLAYER_H
#define FF_AUDIO_PLAYER_H

#include <atomic>
#include "ffmpeg_jni_define.h"
#include "visualizer/frank_visualizer.h"

#ifdef __cplusplus
extern "C" {
#endif
#include "libavformat/avformat.h"
#include "libavcodec/avcodec.h"
#include "libswresample/swresample.h"

#include "libavfilter/buffersink.h"
#include "libavfilter/buffersrc.h"
#include "libavfilter/avfilter.h"
#ifdef __cplusplus
}
#endif

struct AudioPlayerState {
    int out_channel;
    int out_ch_layout;
    int out_sample_rate;
    enum AVSampleFormat out_sample_fmt;
    int64_t m_position;

    AVPacket *packet;
    AVFrame *inputFrame;
    AVFrame *filterFrame;
    int audioIndex = -1;
    uint8_t *outBuffer;
    SwrContext *swrContext;

    AVFormatContext *formatContext;
    AVCodecContext *codecContext;

    const char *filterDesc;
    std::atomic<bool> filterAgain;
    bool exitPlaying;
    std::mutex m_playMutex;

    AVFilterGraph *audioFilterGraph;
    AVFilterContext *audioSrcContext;
    AVFilterContext *audioSinkContext;
};

class FFAudioPlayer {
private:

    AudioPlayerState *m_state;

public:

    FFAudioPlayer();

    ~FFAudioPlayer();

    int open(const char* path);

    int getSampleRate() const;

    int getChannel() const;

    int decodeAudio();

    uint8_t *getDecodeFrame() const;

    void setFilterAgain(bool again);

    void setFilterDesc(const char *filterDescription);

    void setExit(bool exit);

    int64_t getCurrentPosition();

    int64_t getDuration();

    void close();
};
#endif //FF_AUDIO_PLAYER_H
