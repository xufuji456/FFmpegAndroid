package com.frank.androidmedia.listener

/**
 * @author xufulong
 * @date 3/18/22 2:25 PM
 * @desc
 */
interface PlayerCallback {

    fun onPrepare()

    fun onError(what :Int, extra :Int) :Boolean

    fun onRenderFirstFrame()

    fun onCompleteListener()

}