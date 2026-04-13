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

    int PutFrame(std::unique_ptr<FrameBuffer> &frame);

    int GetFrame(std::unique_ptr<FrameBuffer> &frame);

    int Size();

    void Flush();

    void Abort();

private:
    bool bAbort   = false;
    int mCapacity = FRAME_QUEUE_SIZE;

    std::mutex mLock;
    std::condition_variable mCond;
    std::queue<std::unique_ptr<FrameBuffer>> mFrameQueue;
};

#endif //NEXT_FRAME_QUEUE_H
