
#ifndef FFMPEGANDROID_FRAMEQUEUE_H
#define FFMPEGANDROID_FRAMEQUEUE_H

#include <mutex>
#include "ffplay_define.h"

extern "C" {
#include "libavcodec/avcodec.h"
}

typedef struct Frame {
    double pts;
    int width;
    int height;
    int format;
    AVFrame *frame;
    double duration;
} Frame;

class FrameQueue {

private:
    std::mutex m_mutex;
    int m_abort_request;
    int m_read_idx;
    int m_write_idx;
    int m_size;
    int m_max_size;
    int m_keep_last;
    int m_show_idx;
    Frame m_queue[FRAME_QUEUE_SIZE];

    void unrefFrame(Frame *vp);

public:
    FrameQueue(int max_size, int keep_last);

    virtual ~FrameQueue();

    void start();

    void abort();

    Frame *currentFrame();

    Frame *nextFrame();

    Frame *lastFrame();

    Frame *peekWritable();

    void pushFrame();

    void popFrame();

    void flush();

    int getFrameSize();

    int getShowIndex() const;

};


#endif //FFMPEGANDROID_FRAMEQUEUE_H
