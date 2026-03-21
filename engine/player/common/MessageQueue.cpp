/**
 * Note: message queue：push、pop、remove、recycle、clear
 * Date: 2026/3/21
 * Author: frank
 */

#include "MessageQueue.h"

#include "CommonUtil.h"
#include "NextErrorCode.h"
#include "NextLog.h"

#ifdef __cplusplus
extern "C" {
#endif
#include "libavutil/mem.h"
#ifdef __cplusplus
}
#endif

#define MQ_TAG "MessageQueue"

AVMessage::AVMessage(int what, int arg1, int arg2, void *obj, int len)
        : mWhat(what), mArg1(arg1), mArg2(arg2) {
    mTime = CurrentTimeMs();
    if (obj && len > 0) {
        mObj = av_mallocz(len * sizeof(uint8_t));
        if (mObj) {
            memcpy(mObj, obj, len);
        }
    }
}

AVMessage::~AVMessage() {
    Clear();
}

int32_t MessageQueue::Start() {
    std::lock_guard<std::mutex> lock(mMsgMutex);
    mMsgAbort = false;
    return RESULT_OK;
}

void AVMessage::Clear() {
    mWhat = 0;
    mArg1 = 0;
    mArg2 = 0;
    mTime = 0;
    if (mObj) {
        av_freep(&mObj);
    }
}

MessageQueue::MessageQueue() {
    mMsgAbort = false;
}

MessageQueue::~MessageQueue() {
    mMsgAbort = false;
    while (!mMessageQueue.empty()) {
        mMessageQueue.pop();
    }
    while (!mRecycledQueue.empty()) {
        mRecycledQueue.pop();
    }
}

int32_t MessageQueue::Push(int what, int arg1, int arg2, void *obj, int len) {
    std::lock_guard<std::mutex> lock(mMsgMutex);
    sp<AVMessage> msg;
    bool notify = false;
    if (mRecycledQueue.empty()) {
        try {
            msg = std::make_shared<AVMessage>(what, arg1, arg2, obj, len);
        } catch (const std::bad_alloc &e) {
            NEXT_LOGE(MQ_TAG, "alloc message error: %s!\n", e.what());
            return ERROR_OTHER_OOM;
        } catch (...) {
            NEXT_LOGE(MQ_TAG, "create message error!\n");
            return ERROR_OTHER_OOM;
        }
    } else {
        msg = mRecycledQueue.front();
        mRecycledQueue.pop();
        if (!msg) {
            NEXT_LOGE(MQ_TAG, "pop message error!\n");
            return ERROR_OTHER_OOM;
        }
        msg->mWhat = what;
        msg->mArg1 = arg1;
        msg->mArg2 = arg2;
        msg->mTime = CurrentTimeMs();
        if (obj && len > 0) {
            msg->mObj = av_mallocz(len * sizeof(uint8_t));
            if (msg->mObj) {
                memcpy(msg->mObj, obj, len);
            }
        }
    }

    notify = mMessageQueue.empty();
    mMessageQueue.push(msg);

    if (notify) {
        mMsgCondition.notify_one();
    }

    return RESULT_OK;
}

sp<AVMessage> MessageQueue::Pop(bool block) {
    std::unique_lock<std::mutex> lock(mMsgMutex);
    sp<AVMessage> ret;
    while (mMessageQueue.empty()) {
        if (!block || mMsgAbort) {
            return ret;
        } else {
            mMsgCondition.wait(lock);
        }
    }
    if (mMsgAbort) {
        return ret;
    }
    ret = mMessageQueue.front();
    mMessageQueue.pop();
    if (!ret) {
        NEXT_LOGI(MQ_TAG, "pop null message!\n");
    }
    return ret;
}

int32_t MessageQueue::Flush() {
    std::lock_guard<std::mutex> lock(mMsgMutex);
    while (!mMessageQueue.empty()) {
        auto msg = mMessageQueue.front();
        msg->Clear();
        mRecycledQueue.push(msg);
        mMessageQueue.pop();
    }
    return RESULT_OK;
}

int32_t MessageQueue::Remove(int what) {
    std::lock_guard<std::mutex> lock(mMsgMutex);
    while (!mMessageQueue.empty()) {
        auto msg = mMessageQueue.front();
        if (msg && msg->mWhat == what) {
            msg->Clear();
            mRecycledQueue.push(msg);
            mMessageQueue.pop();
            break;
        }
    }
    return RESULT_OK;
}

int32_t MessageQueue::Recycle(sp<AVMessage> &msg) {
    std::lock_guard<std::mutex> lock(mMsgMutex);
    if (!msg) {
        return RESULT_OK;
    }
    msg->Clear();
    mRecycledQueue.push(msg);
    return RESULT_OK;
}

int32_t MessageQueue::Abort() {
    std::lock_guard<std::mutex> lock(mMsgMutex);
    mMsgAbort = true;
    mMsgCondition.notify_one();
    return RESULT_OK;
}
