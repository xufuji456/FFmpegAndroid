/**
 * Note: executor of base thread
 * Date: 2026/3/18
 * Author: frank
 */

#include "BaseThread.h"

#include "NextLog.h"

#define TAG "BaseThread"

BaseThread::BaseThread() {
    bAbort = false;
}

BaseThread::BaseThread(const char *threadName) {
    bAbort = false;
    mThreadName = std::string (threadName);
}

BaseThread::~BaseThread() {
    if (mThread.joinable()) {
        mThread.join();
    }
}

void BaseThread::Start() {
    try {
        mThread = std::thread(StartThread, this);
    } catch (const std::system_error &e) {
        NEXT_LOGE(TAG, "thread exception: %s!\n", e.what());
    }
}

void* BaseThread::StartThread(void *ptr) {
    auto thread = (BaseThread*) ptr;
#if defined(__APPLE__)
    pthread_setname_np(thread->mThreadName.c_str());
#elif defined(__ANDROID__) || defined(__linux__)
    pthread_setname_np(pthread_self(), thread->mThreadName.c_str());
#endif
    NEXT_LOGI(TAG, "%s thread start...", thread->mThreadName.c_str());
    thread->ExecuteTask();
    NEXT_LOGI(TAG, "%s thread end...", thread->mThreadName.c_str());
    return nullptr;
}
