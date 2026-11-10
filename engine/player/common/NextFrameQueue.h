#ifndef NEXT_FRAME_QUEUE_H
#define NEXT_FRAME_QUEUE_H

#include <condition_variable>
#include <mutex>
#include <queue>

#include "NextFrameBuffer.h"

class FrameQueue {
public:
    FrameQueue() = default;

    explicit FrameQueue(int capacity);

    ~FrameQueue() = default;

    int putFrame(std::unique_ptr<FrameBuffer> &frame);

    int getFrame(std::unique_ptr<FrameBuffer> &frame, int timeoutMs = -1);

    int size();

    void flush();

    void abort();

private:
    bool bAbort   = false;
    int mCapacity = FRAME_QUEUE_SIZE;

    std::mutex mLock;
    std::condition_variable mCond;
    std::queue<std::unique_ptr<FrameBuffer>> mFrameQueue;
};

#endif //NEXT_FRAME_QUEUE_H
