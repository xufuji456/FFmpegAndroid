package com.frank.ffmpeg.listener

interface OnSeeBarListener {
    fun onProgress(index: Int, progress: Int)
}