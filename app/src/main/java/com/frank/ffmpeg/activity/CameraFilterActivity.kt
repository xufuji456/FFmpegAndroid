package com.frank.ffmpeg.activity

import android.os.Bundle
import android.view.View
import com.frank.camerafilter.factory.BeautyFilterType
import com.frank.camerafilter.widget.BeautyCameraView
import com.frank.ffmpeg.R

class CameraFilterActivity : BaseActivity() {

    private var cameraView: BeautyCameraView ?= null

    private var index: Int = 0

    private val filterType: Array<BeautyFilterType> = arrayOf(
        BeautyFilterType.NONE,
        BeautyFilterType.SATURATION,
        BeautyFilterType.CONTRAST,
        BeautyFilterType.BRIGHTNESS,
        BeautyFilterType.SHARPEN,
        BeautyFilterType.BLUR,
        BeautyFilterType.HUE,
        BeautyFilterType.WHITE_BALANCE,
        BeautyFilterType.SKETCH,
        BeautyFilterType.OVERLAY,
        BeautyFilterType.BREATH_CIRCLE
    )

    override val layoutId: Int
        get() = R.layout.activity_camera_filter

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        initView()
    }

    fun initView() {
        cameraView = getView(R.id.surface_camera_filter)
        initViewsWithClick(R.id.btn_video_recorder)
        initViewsWithClick(R.id.btn_camera_filter)
    }

    override fun onViewClick(view: View) {
        if (view.id == R.id.btn_video_recorder) {
            val isRecording = cameraView!!.isRecording
            cameraView!!.isRecording = !isRecording
            if (!isRecording) {
                showToast("start recording...")
            } else {
                showToast("stop recording...")
            }
        } else if (view.id == R.id.btn_camera_filter) {
            index++
            if (index >= filterType.size)
                index = 0
            cameraView!!.setFilter(filterType[index])
            showToast(getString(filterTypeToNameId(filterType[index])))
        }
    }

    override fun onSelectedFile(filePath: String) {

    }

    fun filterTypeToNameId(type: BeautyFilterType): Int {
        return when (type) {
            BeautyFilterType.NONE -> R.string.camera_filter_none
            BeautyFilterType.BRIGHTNESS -> R.string.camera_filter_brightness
            BeautyFilterType.SATURATION -> R.string.camera_filter_saturation
            BeautyFilterType.CONTRAST -> R.string.camera_filter_contrast
            BeautyFilterType.SHARPEN -> R.string.camera_filter_sharpen
            BeautyFilterType.BLUR -> R.string.camera_filter_blur
            BeautyFilterType.HUE -> R.string.camera_filter_hue
            BeautyFilterType.WHITE_BALANCE -> R.string.camera_filter_balance
            BeautyFilterType.SKETCH -> R.string.camera_filter_sketch
            BeautyFilterType.OVERLAY -> R.string.camera_filter_overlay
            BeautyFilterType.BREATH_CIRCLE -> R.string.camera_filter_circle
            else -> R.string.camera_filter_none
        }
    }


}