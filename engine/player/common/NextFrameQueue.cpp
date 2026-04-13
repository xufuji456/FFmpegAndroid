/**
 * Note: frame queue: put、get、flush
 * Date: 2026/4/13
 * Author: frank
 */

#include "NextFrameQueue.h"

#include "NextDefine.h"
#include "NextErrorCode.h"
#include "NextLog.h"

#define TAG "FrameQueue"
#define UNIQUE_LOCK std::unique_lock<std::mutex>

FrameQueue::FrameQueue(int capacity)
    : mCapacity(capacity) {}

int FrameQueue::PutFrame(std::unique_ptr<FrameBuffer> &frame) {
    UNIQUE_LOCK lock(mLock);
    int count = 0;
    while (mCapacity > 0 && mFrameQueue.size() >= mCapacity) {
        if (bAbort) {
            return RESULT_OK;
        }
        if (mCond.wait_for(lock, std::chrono::milliseconds (QUEUE_WAIT_TIMEOUT))
            == std::cv_status::timeout) {
            if (count++ > 10) {
                count = 0;
                NEXT_LOGD(TAG, "full, wait for 1000ms timeout!\n");
            }
        }
    }
    mFrameQueue.push(std::move(frame));
    mCond.notify_one();
    return RESULT_OK;
}

int FrameQueue::GetFrame(std::unique_ptr<FrameBuffer> &frame) {
    UNIQUE_LOCK lock(mLock);
    int count = 0;
    while (mFrameQueue.empty()) {
        if (bAbort) {
            return RESULT_OK;
        }
        if (mCond.wait_for(lock, std::chrono::milliseconds (QUEUE_WAIT_TIMEOUT))
            == std::cv_status::timeout) {
            if (count++ > 10) {
                count = 0;
                NEXT_LOGD(TAG, "empty, wait for 1000ms timeout!\n");
            }
        }
    }
    frame = std::move(mFrameQueue.front());
    mFrameQueue.pop();
    mCond.notify_one();
    return RESULT_OK;
}

int FrameQueue::Size() {
    UNIQUE_LOCK lock(mLock);
    return static_cast<int>(mFrameQueue.size());
}

void FrameQueue::Flush() {
    UNIQUE_LOCK lock(mLock);
    while (!mFrameQueue.empty()) {
        mFrameQueue.pop();
    }
    mCond.notify_one();
}

void FrameQueue::Abort() {
    UNIQUE_LOCK lock(mLock);
    bAbort = true;
    mCapacity = 0;
    mCond.notify_one();
}
