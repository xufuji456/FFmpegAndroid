
#ifndef SAFE_QUEUE_H
#define SAFE_QUEUE_H

#include <queue>
#include <pthread.h>

#ifdef C11
#include <thread>
#endif

using namespace std;

template<typename T>
class SafeQueue {
    typedef void (*ReleaseCallback)(T &);

    typedef void (*SyncHandle)(queue<T> &);

public:
    SafeQueue() {
#ifdef C11

#else
        pthread_mutex_init(&mutex, NULL);
        pthread_cond_init(&cond, NULL);
#endif

    }

    ~SafeQueue() {
#ifdef C11
#else
        pthread_cond_destroy(&cond);
        pthread_mutex_destroy(&mutex);
#endif

    }

    void push(T new_value) {
#ifdef C11
        lock_guard<mutex> lk(mt);
        if (work) {
            q.push(new_value);
            cv.notify_one();
        }
#else
        pthread_mutex_lock(&mutex);
        if (work) {
            q.push(new_value);
            pthread_cond_signal(&cond);
        } else {
            releaseCallback(new_value);
        }
        pthread_mutex_unlock(&mutex);
#endif

    }


    int pop(T &value) {
        int ret = 0;
#ifdef C11
        unique_lock<mutex> lk(mt);
        cv.wait(lk,[this]{return !work || !q.empty();});
        if (!q.empty()) {
            value = q.front();
            q.pop();
            ret = 1;
        }
#else
        pthread_mutex_lock(&mutex);

        while (work && q.empty()) {
            pthread_cond_wait(&cond, &mutex);
        }
        if (!q.empty()) {
            value = q.front();
            q.pop();
            ret = 1;
        }
        pthread_mutex_unlock(&mutex);
#endif
        return ret;
    }

    void setWork(int work) {
#ifdef C11
        lock_guard<mutex> lk(mt);
        this->work = work;
#else
        pthread_mutex_lock(&mutex);
        this->work = work;
        pthread_cond_signal(&cond);
        pthread_mutex_unlock(&mutex);
#endif

    }

    int empty() {
        return q.empty();
    }

    int size() {
        return static_cast<int>(q.size());
    }

    void clear() {
#ifdef C11
        lock_guard<mutex> lk(mt);
        int size = q.size();
        for (int i = 0; i < size; ++i) {
            T value = q.front();
            releaseHandle(value);
            q.pop();
        }
#else
        pthread_mutex_lock(&mutex);
        int size = static_cast<int>(q.size());
        for (int i = 0; i < size; ++i) {
            T value = q.front();
            releaseCallback(value);
            q.pop();
        }
        pthread_mutex_unlock(&mutex);
#endif

    }

    void sync() {
#ifdef C11
        lock_guard<mutex> lk(mt);
        syncHandle(q);
#else
        pthread_mutex_lock(&mutex);
        syncHandle(q);
        pthread_mutex_unlock(&mutex);
#endif

    }

    void setReleaseCallback(ReleaseCallback r) {
        releaseCallback = r;
    }

    void setSyncHandle(SyncHandle s) {
        syncHandle = s;
    }

private:

#ifdef C11
    mutex mt;
    condition_variable cv;
#else
    pthread_cond_t cond;
    pthread_mutex_t mutex;
#endif

    queue<T> q;
    int work;
    ReleaseCallback releaseCallback;
    SyncHandle syncHandle;
};

#endif
