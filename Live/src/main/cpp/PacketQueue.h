
#ifndef PACKET_QUEUE_H
#define PACKET_QUEUE_H

#include <queue>
#include <thread>

template<typename T>
class PacketQueue {
    typedef void (*ReleaseCallback)(T &);

private:
    std::mutex m_mutex;
    std::condition_variable m_cond;
    std::queue<T> m_queue;
    bool m_running;

    ReleaseCallback releaseCallback;

public:

    void push(T new_value) {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_running) {
            m_queue.push(new_value);
            m_cond.notify_one();
        }
    }

    int pop(T &value) {
        int ret = 0;
        std::unique_lock<std::mutex> lock(m_mutex);
        if (!m_running) {
            return ret;
        }
        if (!m_queue.empty()) {
            value = m_queue.front();
            m_queue.pop();
            ret = 1;
        }
        return ret;
    }

    void clear() {
        std::lock_guard<std::mutex> lock(m_mutex);
        int size = m_queue.size();
        for (int i = 0; i < size; ++i) {
            T value = m_queue.front();
            releaseCallback(value);
            m_queue.pop();
        }
    }

    void setRunning(bool run) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_running = run;
    }

    bool empty() {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_queue.empty();
    }

    int size() {
        std::lock_guard<std::mutex> lock(m_mutex);
        return static_cast<int>(m_queue.size());
    }

    void setReleaseCallback(ReleaseCallback callback) {
        releaseCallback = callback;
    }

};

#endif // PACKET_QUEUE_H
