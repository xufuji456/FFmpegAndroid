package com.frank.ffmpeg.model


class LrcLine : Comparable<LrcLine> {

    var timeString: String? = null

    var startTime: Long = 0

    var endTime: Long = 0

    var content: String? = null

    override fun compareTo(another: LrcLine): Int {
        return (this.startTime - another.startTime).toInt()
    }

}