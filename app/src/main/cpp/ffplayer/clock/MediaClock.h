
#ifndef FFMPEGANDROID_MEDIACLOCK_H
#define FFMPEGANDROID_MEDIACLOCK_H

#include <math.h>

#ifdef __cplusplus
extern "C" {
#endif
#include <libavutil/time.h>
#ifdef __cplusplus
}
#endif

class MediaClock {

private:
    bool m_pause;
    double m_pts;
    double m_speed;
    double m_pts_drift;
    double m_last_update;

public:

    MediaClock();

    void setClock(double pts);

    void setClock(double pts, double time);

    double getClock() const;

    void setSpeed(double speed);

    double getSpeed() const;

    void syncToSlave(MediaClock *slave);

    virtual ~MediaClock();

};


#endif //FFMPEGANDROID_MEDIACLOCK_H
