#ifndef BASE_THREAD_H
#define BASE_THREAD_H

#include <atomic>
#include <thread>

class BaseThread {
public:
    BaseThread();

    explicit BaseThread(const char *threadName);

    virtual ~BaseThread();

    virtual void executeTask() = 0;

    void start();

protected:
    std::atomic_int mSerial{0};
    std::atomic_bool bAbort{false};
    std::thread mThread;
    std::string mThreadName;

private:
    static void* startThread(void* ptr);

};

#endif // BASE_THREAD_H
