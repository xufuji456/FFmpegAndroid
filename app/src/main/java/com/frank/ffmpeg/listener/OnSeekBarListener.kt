package com.frank.ffmpeg.listener

interface OnSeekBarListener {
    fun onProgress(index: Int, progress: Int)
}