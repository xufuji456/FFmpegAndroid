#ifndef MEDIA_CLOCK_H
#define MEDIA_CLOCK_H

#include <mutex>

enum AVCLockType {
    CLOCK_AUDIO    = 0,
    CLOCK_VIDEO    = 1,
    CLOCK_EXTERNAL = 2
};

class MediaClock {
public:
    MediaClock();

    void SetClock(double pts);

    double GetClock();

    void SetClockSerial(int serial);

    int GetClockSerial();

    void SetSpeed(double speed);

    void SetPause(bool paused);

private:
    static double GetCurrentTime();

private:
    std::mutex mLock;
    int mSerial;
    bool bPause;
    double mSpeed;
    double mPtsDrift;
    double mLastUpdateTime;

};

#endif // MEDIA_CLOCK_H
