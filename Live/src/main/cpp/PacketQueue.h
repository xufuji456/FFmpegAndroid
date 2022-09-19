
#ifndef SAFE_QUEUE_H
#define SAFE_QUEUE_H

#include <queue>
#include <thread>

using namespace std;

template<typename T>
class PacketQueue {
    typedef void (*ReleaseCallback)(T &);

    typedef void (*SyncHandle)(queue<T> &);

public:

    void push(T new_value) {
        lock_guard<mutex> lk(mt);
        if (running) {
            q.push(new_value);
            cv.notify_one();
        }
    }

    int pop(T &value) {
        int ret = 0;
        unique_lock<mutex> lk(mt);
        if (!running) {
            return ret;
        }
        if (!q.empty()) {
            value = q.front();
            q.pop();
            ret = 1;
        }
        return ret;
    }

    void setRunning(bool run) {
        lock_guard<mutex> lk(mt);
        this->running = run;
    }

    int empty() {
        return q.empty();
    }

    int size() {
        return static_cast<int>(q.size());
    }

    void clear() {
        lock_guard<mutex> lk(mt);
        int size = q.size();
        for (int i = 0; i < size; ++i) {
            T value = q.front();
            releaseCallback(value);
            q.pop();
        }
    }

    void sync() {
        lock_guard<mutex> lk(mt);
        syncHandle(q);
    }

    void setReleaseCallback(ReleaseCallback r) {
        releaseCallback = r;
    }

private:

    mutex mt;
    condition_variable cv;

    queue<T> q;
    bool running;
    ReleaseCallback releaseCallback;
    SyncHandle syncHandle;
};

#endif
