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

int FrameQueue::putFrame(std::unique_ptr<FrameBuffer> &frame) {
    UNIQUE_LOCK lock(mLock);
    int count = 0;
    while (!bAbort && mCapacity > 0 && mFrameQueue.size() >= mCapacity) {
        if (mCond.wait_for(lock, std::chrono::milliseconds (QUEUE_WAIT_TIMEOUT))
            == std::cv_status::timeout) {
            if (count++ > 10) {
                count = 0;
                NEXT_LOGD(TAG, "full, wait for 1000ms timeout!\n");
            }
        }
    }
    if (bAbort) {
        return RESULT_OK;
    }
    mFrameQueue.push(std::move(frame));
    mCond.notify_all();
    return RESULT_OK;
}

int FrameQueue::getFrame(std::unique_ptr<FrameBuffer> &frame, int timeoutMs) {
    UNIQUE_LOCK lock(mLock);
    if (timeoutMs >= 0) {
        mCond.wait_for(lock, std::chrono::milliseconds(timeoutMs),
                       [this] { return bAbort || !mFrameQueue.empty(); });
    } else {
        int count = 0;
        while (!bAbort && mFrameQueue.empty()) {
            if (mCond.wait_for(lock, std::chrono::milliseconds (QUEUE_WAIT_TIMEOUT))
                == std::cv_status::timeout) {
                if (count++ > 10) {
                    count = 0;
                    NEXT_LOGD(TAG, "empty, wait for 1000ms timeout!\n");
                }
            }
        }
    }
    if (bAbort || mFrameQueue.empty()) {
        return ERROR_PLAYER_TRY_AGAIN;
    }
    frame = std::move(mFrameQueue.front());
    mFrameQueue.pop();
    mCond.notify_all();
    return RESULT_OK;
}

int FrameQueue::size() {
    UNIQUE_LOCK lock(mLock);
    return static_cast<int>(mFrameQueue.size());
}

void FrameQueue::flush() {
    UNIQUE_LOCK lock(mLock);
    while (!mFrameQueue.empty()) {
        mFrameQueue.pop();
    }
    mCond.notify_all();
}

void FrameQueue::abort() {
    UNIQUE_LOCK lock(mLock);
    bAbort = true;
    mCond.notify_all();
}
