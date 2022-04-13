package com.frank.ffmpeg.handler

import android.content.Context
import android.util.Log
import android.view.OrientationEventListener

/**
 * Handler of orientation rotate event
 * Created by frank on 2022/4/13.
 */

class OrientationHandler(context: Context) {

    companion object {
        private const val TAG = "OrientationHandler"
        private const val OFFSET_ANGLE = 5
    }

    private var lastOrientationDegree = 0
    private var onOrientationListener: OnOrientationListener? = null
    private var orientationEventListener: OrientationEventListener? = null

    interface OnOrientationListener {
        fun onOrientation(orientation: Int)
    }

    init {
        initOrientation(context)
    }

    fun setOnOrientationListener(onOrientationListener: OnOrientationListener) {
        this.onOrientationListener = onOrientationListener
    }

    private fun initOrientation(context: Context) {
        orientationEventListener = object : OrientationEventListener(context.applicationContext) {
            override fun onOrientationChanged(orientation: Int) {
                if (orientation == ORIENTATION_UNKNOWN)
                    return

                if (orientation >= 0 - OFFSET_ANGLE && orientation <= OFFSET_ANGLE) {
                    if (lastOrientationDegree != 0) {
                        Log.i(TAG, "0, portrait down")
                        lastOrientationDegree = 0
                        onOrientationListener?.onOrientation(lastOrientationDegree)
                    }
                } else if (orientation >= 90 - OFFSET_ANGLE && orientation <= 90 + OFFSET_ANGLE) {
                    if (lastOrientationDegree != 90) {
                        Log.i(TAG, "90, landscape right")
                        lastOrientationDegree = 90
                        onOrientationListener?.onOrientation(lastOrientationDegree)
                    }
                } else if (orientation >= 180 - OFFSET_ANGLE && orientation <= 180 + OFFSET_ANGLE) {
                    if (lastOrientationDegree != 180) {
                        Log.i(TAG, "180, portrait up")
                        lastOrientationDegree = 180
                        onOrientationListener?.onOrientation(lastOrientationDegree)
                    }
                } else if (orientation >= 270 - OFFSET_ANGLE && orientation <= 270 + OFFSET_ANGLE) {
                    if (lastOrientationDegree !=270) {
                        Log.i(TAG, "270, landscape left")
                        lastOrientationDegree = 270
                        onOrientationListener?.onOrientation(lastOrientationDegree)
                    }
                }
            }
        }
    }

    fun enable() {
        if (orientationEventListener?.canDetectOrientation()!!) {
            orientationEventListener?.enable()
        }
    }

    fun disable() {
        if (orientationEventListener?.canDetectOrientation()!!) {
            orientationEventListener?.disable()
        }
    }

}