package com.frank.androidmedia.listener

/**
 * @author xufulong
 * @date 4/1/22 1:44 PM
 * @desc
 */
interface VideoEncodeCallback {

    fun onVideoEncodeData(data: ByteArray, size: Int, flag: Int, timestamp: Long)

}