#ifndef MESSAGE_QUEUE_H
#define MESSAGE_QUEUE_H

#include "NextDefine.h"
#include "NextStructDefine.h"

#include <condition_variable>
#include <mutex>
#include <queue>

class AVMessage {
public:
    AVMessage() = default;

    AVMessage(int what, int arg1 = 0, int arg2 = 0, void *obj = nullptr, int len = 0);

    ~AVMessage();

    void Clear();

public:
    int mWhat     = 0;
    int mArg1     = 0;
    int mArg2     = 0;
    void *mObj    = nullptr;
    int64_t mTime = 0;
};

class MessageQueue {
public:
    MessageQueue();

    ~MessageQueue();

    int32_t Start();

    int32_t Push(int what, int arg1 = 0, int arg2 = 0, void *obj = nullptr, int len = 0);

    sp<AVMessage> Pop(bool block);

    int32_t Flush();

    int32_t Remove(int what);

    int32_t Recycle(sp<AVMessage> &msg);

    int32_t Abort();

private:
    bool mMsgAbort;
    std::mutex mMsgMutex;
    std::condition_variable mMsgCondition;
    std::queue<sp<AVMessage>> mMessageQueue;
    std::queue<sp<AVMessage>> mRecycledQueue;
};

#endif
