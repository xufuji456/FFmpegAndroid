package com.frank.ffmpeg.listener

import com.frank.ffmpeg.model.LrcLine

interface OnLrcListener {

    fun onLrcSeek(position: Int, lrcLine: LrcLine)
}
