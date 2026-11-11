/**
 * Note: custom packet queue
 * Date: 2026/4/20
 * Author: frank
 */

#include "NextPacketQueue.h"

#include "NextDefine.h"
#include "NextErrorCode.h"
#include "NextLog.h"

#define TAG "PacketQueue"

#define UNIQUE_LOCK std::unique_lock<std::mutex>

NextPacketQueue::NextPacketQueue(int type) {
    mDuration  = 0;
    mByteCount = 0;
}

int NextPacketQueue::putPacket(std::unique_ptr<NextPacket> &pkt) {
    UNIQUE_LOCK lock(mLock);
    AVPacket *packet = pkt ? pkt->getPacket() : nullptr;
    mByteCount += packet ? packet->size : 0;
    mDuration  += packet ? std::max(packet->duration, (int64_t) MIN_PKT_DURATION) : 0;
    mPktQueue.push(std::move(pkt));
    mCond.notify_all();
    return RESULT_OK;
}

int NextPacketQueue::getPacket(std::unique_ptr<NextPacket> &pkt, bool block) {
    UNIQUE_LOCK lock(mLock);
    if (mPktQueue.empty() && !block)
        return ERROR_PLAYER_TRY_AGAIN;
    int count = 0;
    while (mPktQueue.empty() && !bAbort) {
        if (mCond.wait_for(lock, std::chrono::milliseconds (QUEUE_WAIT_TIMEOUT))
            == std::cv_status::timeout) {
            if (count++ > 10) {
                count = 0;
                NEXT_LOGD(TAG, "empty, wait for 1000ms timeout!\n");
            }
        }
    }
    if (mPktQueue.empty()) {
        return ERROR_PLAYER_TRY_AGAIN;
    }
    pkt = std::move(mPktQueue.front());
    AVPacket *packet = pkt ? pkt->getPacket() : nullptr;
    mByteCount -= packet ? packet->size : 0;
    mDuration  -= packet ? std::max(packet->duration, (int64_t) MIN_PKT_DURATION) : 0;
    mPktQueue.pop();
    return RESULT_OK;
}

bool NextPacketQueue::isFlushPacket() {
    UNIQUE_LOCK lock(mLock);
    if (mPktQueue.empty()) {
        return false;
    }
    if (!mPktQueue.front()) {
        return false;
    }
    return mPktQueue.front()->isFlushPacket();
}

int NextPacketQueue::packetCount() {
    UNIQUE_LOCK lock(mLock);
    return static_cast<int>(mPktQueue.size());
}

int64_t NextPacketQueue::byteCount() {
    UNIQUE_LOCK lock(mLock);
    return mByteCount;
}

int64_t NextPacketQueue::duration() {
    UNIQUE_LOCK lock(mLock);
    return mDuration;
}

void NextPacketQueue::flush() {
    UNIQUE_LOCK lock(mLock);
    int flushCount = 0;
    while (!mPktQueue.empty()) {
        auto pkt = std::move(mPktQueue.front());
        if (pkt && pkt->isFlushPacket()) {
            flushCount++;
        }
        mPktQueue.pop();
    }
    for (int i = 0; i < flushCount; ++i) {
        std::unique_ptr<NextPacket> flushPacket(new NextPacket(PKT_OP_TYPE_FLUSH));
        mPktQueue.push(std::move(flushPacket));
    }

    mDuration  = 0;
    mByteCount = 0;
}

void NextPacketQueue::abort() {
    UNIQUE_LOCK lock(mLock);
    bAbort = true;
    mCond.notify_all();
}

void NextPacketQueue::release() {
    UNIQUE_LOCK lock(mLock);
    while (!mPktQueue.empty()) {
        mPktQueue.pop();
    }
    mDuration  = 0;
    mByteCount = 0;
}
