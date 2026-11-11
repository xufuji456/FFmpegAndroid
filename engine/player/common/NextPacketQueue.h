#ifndef NEXT_PACKET_QUEUE_H
#define NEXT_PACKET_QUEUE_H

#include <condition_variable>
#include <mutex>
#include <queue>

#include "NextPacket.h"

#define MIN_PKT_DURATION 16

class NextPacketQueue {
public:
    NextPacketQueue() = default;

    explicit NextPacketQueue(int type);

    ~NextPacketQueue() = default;

    int putPacket(std::unique_ptr<NextPacket> &pkt);

    int getPacket(std::unique_ptr<NextPacket> &pkt, bool block);

    bool isFlushPacket();

    int packetCount();

    int64_t byteCount();

    int64_t duration();

    void flush();

    void abort();

    void release();

private:
    bool bAbort        = false;
    int64_t mDuration  = 0;
    int64_t mByteCount = 0;

    std::mutex mLock;
    std::condition_variable mCond;
    std::queue<std::unique_ptr<NextPacket>> mPktQueue;
};


#endif //Next_PACKET_QUEUE_H
