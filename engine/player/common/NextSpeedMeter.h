#ifndef NEXT_SPEED_METER_H
#define NEXT_SPEED_METER_H

#include <cstdint>

#define DEFAULT_SAMPLE_SIZE 10

// decode and render rate
class VideoSpeedMeter {
public:
    VideoSpeedMeter() = default;

    ~VideoSpeedMeter() = default;

    void reset();

    float add();

private:
    int mCount      = 0;
    int mNextIndex  = 0;
    int mFirstIndex = 0;
    int mSampleSize = DEFAULT_SAMPLE_SIZE;
    int64_t mSamples[DEFAULT_SAMPLE_SIZE] = {0};
};

// transfer speed
class NetworkSpeedMeter {
public:
    NetworkSpeedMeter() = default;

    ~NetworkSpeedMeter() = default;

    void reset(int sampleRange);

    int64_t add(int size);

    int64_t getSpeed() const;

    int64_t getLastSpeed() const;

private:
    int64_t mSampleRange        = 0;
    int64_t mLastSampleTick     = 0;
    int64_t mLastSampleSize     = 0;
    int64_t mLastSampleSpeed    = 0;
    int64_t mLastSampleDuration = 0;
};

#endif // NEXT_SPEED_METER_H
