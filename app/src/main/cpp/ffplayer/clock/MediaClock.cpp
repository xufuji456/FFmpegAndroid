
#include <ffplay_define.h>
#include "MediaClock.h"

MediaClock::MediaClock() {
    m_speed = 1.0;
    m_pause = false;
    setClock(NAN);
}

void MediaClock::setClock(double pts) {
    double time = (double) av_gettime_relative() / 1000000.0;
    setClock(pts, time);
}

void MediaClock::setClock(double pts, double time) {
    m_pts         = pts;
    m_last_update = time;
    m_pts_drift   = m_pts - time;
}

double MediaClock::getClock() const {
    if (m_pause) {
        return m_pts;
    } else {
        double time = (double) av_gettime_relative() / 1000000.0;
        return m_pts_drift + time - (time - m_last_update) * (1.0 - m_speed);
    }
}

void MediaClock::setSpeed(double speed) {
    setClock(getClock());
    m_speed = speed;
}

double MediaClock::getSpeed() const {
    return m_speed;
}

void MediaClock::syncToSlave(MediaClock *slave) {
    double clock = getClock();
    double slave_clock = slave->getClock();
    if (!isnan(slave_clock) && (isnan(clock) || fabs(clock - slave_clock) > AV_NOSYNC_THRESHOLD)) {
        setClock(slave_clock);
    }
}

MediaClock::~MediaClock() {

}