//
// Created by xu fulong on 2022/9/4.
//

#ifndef FF_AUDIO_PLAYER_H
#define FF_AUDIO_PLAYER_H

#include <stdatomic.h>
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
#include "libavfilter/avfiltergraph.h"
#ifdef __cplusplus
}
#endif

class FFAudioPlayer {
private:

    AVFormatContext *formatContext;
    AVCodecContext *codecContext;
    int audio_index = -1;
    SwrContext *swrContext;
    int out_sample_rate;
    int out_ch_layout;
    int out_channel;
    enum AVSampleFormat out_sample_fmt;
    AVPacket *packet;
    AVFrame *inputFrame;
    AVFrame *filterFrame;
    uint8_t *out_buffer;

    const char *filterDesc;
    std::atomic_bool filterAgain;
    std::atomic_bool exitPlaying;

    AVFilterGraph *audioFilterGraph;
    AVFilterContext *audioSrcContext;
    AVFilterContext *audioSinkContext;

    bool m_enableVisualizer = false;
    FrankVisualizer *mVisualizer;

public:
    int open(const char* path);

    int getSampleRate() const;

    int getChannel() const;

    int decodeAudio();

    uint8_t *getDecodeFrame() const;

    void setFilterAgain(bool again);

    void setFilterDesc(const char *filterDescription);

    void setEnableVisualizer(bool enable);

    bool enableVisualizer() const;

    int8_t* getFFTData() const;

    int getFFTSize() const;

    void setExit(bool exit);

    void close();
};
#endif //FF_AUDIO_PLAYER_H
