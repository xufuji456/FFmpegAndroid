package com.frank.androidmedia.controller

import android.graphics.Bitmap
import android.graphics.BitmapFactory
import android.media.MediaMetadataRetriever
import android.util.Log
import java.lang.Exception

/**
 * Retrieve media metadata from video or audio,
 * and get thumbnail/frame from video or audio
 * @author frank
 * @date 2022/3/18
 */

open class MediaMetadataController {

    private var title: String? = null
    private var duration: Long = 0
    private var bitrate: Int = 0

    private var width: Int = 0
    private var height: Int = 0
    private var frameRate: Float = 0.0f

    private var thumbnail: Bitmap? = null

    private var mRetriever: MediaMetadataRetriever? = null


    fun retrieveMetadata(path: String) {
        val retriever = MediaMetadataRetriever()
        try {
            retriever.setDataSource(path)
            title = retriever.extractMetadata(MediaMetadataRetriever.METADATA_KEY_TITLE)
            if (title != null) {
                Log.i(TAG, "title=$title")
            }
            val durationStr = retriever.extractMetadata(MediaMetadataRetriever.METADATA_KEY_DURATION)
            if (durationStr != null) {
                duration = durationStr.toLong()
                Log.i(TAG, "duration=$duration")
            }
            val bitrateStr = retriever.extractMetadata(MediaMetadataRetriever.METADATA_KEY_BITRATE)
            if (bitrateStr != null) {
                bitrate = bitrateStr.toInt()
            }
            val widthStr = retriever.extractMetadata(MediaMetadataRetriever.METADATA_KEY_VIDEO_WIDTH)
            if (widthStr != null) {
                width = widthStr.toInt()
            }
            val heightStr = retriever.extractMetadata(MediaMetadataRetriever.METADATA_KEY_VIDEO_HEIGHT)
            if (heightStr != null) {
                height = heightStr.toInt()
                Log.i(TAG, "video width=$width,height=$height")
            }
            try {
                if (android.os.Build.VERSION.SDK_INT >= android.os.Build.VERSION_CODES.M) {
                    val frameRateStr = retriever.extractMetadata(MediaMetadataRetriever.METADATA_KEY_CAPTURE_FRAMERATE)
                    frameRate = frameRateStr.toFloat()
                    Log.i(TAG, "frameRate=$frameRate")
                }
            } catch (e: Exception) {
                Log.e(TAG, "retrieve frameRate error=$e")
            }
            val hasVideoStr = retriever.extractMetadata(MediaMetadataRetriever.METADATA_KEY_HAS_VIDEO)
            val hasAudioStr = retriever.extractMetadata(MediaMetadataRetriever.METADATA_KEY_HAS_AUDIO)
            if (hasVideoStr != null && "yes" == hasVideoStr) {
                thumbnail = retriever.getFrameAtTime(0)
            } else if (hasAudioStr != null && "yes" == hasAudioStr) {
                val byteArray = retriever.embeddedPicture
                thumbnail = BitmapFactory.decodeByteArray(byteArray, 0, byteArray.size, null)
            }
            if (thumbnail != null) {
                Log.i(TAG, "thumbnail width=${thumbnail?.width}, height=${thumbnail?.height}")
            }
        } catch (e: Exception) {
            Log.e(TAG, "retrieve error=$e")
        } finally {
            retriever.release()
        }
    }

    fun initRetriever(path: String) {
        mRetriever = MediaMetadataRetriever()
        try {
            mRetriever?.setDataSource(path)
        } catch (e: Exception) {
            Log.e(TAG, "initRetriever error=$e")
        }
    }

    fun getFrameAtTime(timeUs: Long) : Bitmap? {
        if (mRetriever == null)
            return null
        return mRetriever!!.getFrameAtTime(timeUs)
    }

    fun releaseRetriever() {
        if (mRetriever != null) {
            mRetriever?.release()
        }
    }

    companion object {
        val TAG: String = MediaMetadataController::class.java.simpleName
    }

}