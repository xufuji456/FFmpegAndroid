package com.frank.ffmpeg.listener

/**
 *
 * @author frank
 * @date 2022/3/18
 */
interface PlayerCallback {

    fun onPrepare()

    fun onError(what :Int, extra :Int) :Boolean

    fun onRenderFirstFrame()

    fun onCompleteListener()

}