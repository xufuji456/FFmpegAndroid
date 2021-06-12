package com.frank.ffmpeg.listener

/**
 * listener of FFmpeg processing
 * Created by frank on 2019/11/11.
 */
interface OnHandleListener {
    fun onBegin()
    fun onMsg(msg: String)
    fun onProgress(progress: Int, duration: Int)
    fun onEnd(resultCode: Int, resultMsg: String)
}
