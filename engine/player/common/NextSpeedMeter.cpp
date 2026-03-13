/**
 * Note: speed detector
 * Date: 2026/3/13
 * Author: frank
 */

#include "NextSpeedMeter.h"

#include <cstdlib>
#include <cstring>

#include "CommonUtil.h"

void VideoSpeedMeter::reset() {
    mCount      = 0;
    mNextIndex  = 0;
    mFirstIndex = 0;
    memset(mSamples, 0, sizeof(mSamples));
}

float VideoSpeedMeter::add() {
    int64_t now = CurrentTimeMs();
    mSamples[mNextIndex] = now;
    mNextIndex++;
    mNextIndex %= mSampleSize;
    if (mCount + 1 >= mSampleSize) {
        mFirstIndex++;
        mFirstIndex %= mSampleSize;
    } else {
        mCount++;
    }

    if (mSampleSize < 2 || now - mSamples[mFirstIndex] <= 0) {
        return 0.0f;
    }

    return 1000.0f * static_cast<float>((mCount - 1) / (now - mSamples[mFirstIndex]));
}

void NetworkSpeedMeter::reset(int sampleRange) {
    mSampleRange        = sampleRange;
    mLastSampleTick     = CurrentTimeMs();
    mLastSampleSize     = 0;
    mLastSampleSpeed    = 0;
    mLastSampleDuration = 0;
}

int64_t NetworkSpeedMeter::add(int size) {
    if (size < 0)
        return 0;

    int64_t lastTick     = mLastSampleTick;
    int64_t sampleSize   = mLastSampleSize;
    int64_t sampleRange  = mSampleRange;
    int64_t lastDuration = mLastSampleDuration;
    int64_t now = CurrentTimeMs();
    int64_t elapsed = std::abs(now - lastTick);

    if ((elapsed < 0 || elapsed >= sampleRange) && sampleRange > 0) {
        mLastSampleTick     = now;
        mLastSampleSize     = size;
        mLastSampleSpeed    = size * 1000 / sampleRange;
        mLastSampleDuration = sampleRange;
        return mLastSampleSpeed;
    }

    int64_t currentSize = sampleSize + size;
    int64_t curDuration = lastDuration + elapsed;
    if (curDuration > sampleRange && curDuration > 0) {
        currentSize = currentSize * sampleRange / curDuration;
        curDuration = sampleRange;
    }

    mLastSampleTick     = now;
    mLastSampleDuration = curDuration;
    mLastSampleSize = currentSize;
    if (curDuration > 0) {
        mLastSampleSpeed = currentSize * 1000 / curDuration;
    }

    return mLastSampleSpeed;
}

int64_t NetworkSpeedMeter::getSpeed() const {
    int64_t lastSize     = mLastSampleSize;
    int64_t lastTick     = mLastSampleTick;
    int64_t sampleRange  = mSampleRange;
    int64_t lastDuration = mLastSampleDuration;
    int64_t now          = CurrentTimeMs();
    int64_t elapsed      = std::abs(now - lastTick);

    if (elapsed < 0 || elapsed >= sampleRange)
        return 0;

    int64_t currentSize = lastSize;
    int64_t curDuration = lastDuration + elapsed;
    if (curDuration > sampleRange && curDuration > 0) {
        currentSize = currentSize * sampleRange / curDuration;
        curDuration = sampleRange;
    }

    if (curDuration <= 0)
        return 0;

    return currentSize * 1000 / curDuration;
}

int64_t NetworkSpeedMeter::getLastSpeed() const {
    return mLastSampleSpeed;
}
