/**
 * Note: media clock of player
 * Date: 2026/3/17
 * Author: frank
 */

#include "MediaClock.h"

#include <chrono>
#include <cmath>

using namespace std::chrono;

MediaClock::MediaClock()
        : mSerial(0),
          bPause(true),
          mSpeed(1.0f) {
    mLastUpdateTime = GetCurrentTime();
    mPtsDrift = 0 - mLastUpdateTime;
}

void MediaClock::SetClock(double pts) {
    std::lock_guard<std::mutex> lock(mLock);
    double now = GetCurrentTime();
    mPtsDrift = pts - now;
    mLastUpdateTime = now;
}

double MediaClock::GetClock() {
    std::lock_guard<std::mutex> lock(mLock);
    double now = GetCurrentTime();
    if (bPause)
        now = mLastUpdateTime;
    return mPtsDrift + now - (now - mLastUpdateTime) * (1.0f - mSpeed);
}

void MediaClock::SetClockSerial(int serial) {
    std::lock_guard<std::mutex> lck(mLock);
    mSerial = serial;
}

int MediaClock::GetClockSerial() {
    std::lock_guard<std::mutex> lock(mLock);
    return mSerial;
}

void MediaClock::SetSpeed(double speed) {
    std::lock_guard<std::mutex> lock(mLock);
    mSpeed = speed;
}

void MediaClock::SetPause(bool paused) {
    std::lock_guard<std::mutex> lock(mLock);
    bPause = paused;
}

double MediaClock::GetCurrentTime() {
    return static_cast<double>(duration_cast<milliseconds >(
            system_clock::now().time_since_epoch()).count()) / 1000.0;
}
