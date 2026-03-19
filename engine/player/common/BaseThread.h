#ifndef BASE_THREAD_H
#define BASE_THREAD_H

#include <thread>

class BaseThread {
public:
    BaseThread();

    explicit BaseThread(const char *threadName);

    virtual ~BaseThread();

    virtual void ExecuteTask() = 0;

    void Start();

protected:
    int mSerial = 0;
    bool bAbort = false;
    std::thread mThread;
    std::string mThreadName;

private:
    static void* StartThread(void* ptr);

};

#endif // BASE_THREAD_H
