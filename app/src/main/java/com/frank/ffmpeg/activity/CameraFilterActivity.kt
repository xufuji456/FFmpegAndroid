package com.frank.ffmpeg.activity

import android.os.Bundle
import android.view.View
import com.frank.camerafilter.factory.BeautyFilterType
import com.frank.camerafilter.widget.BeautyCameraView
import com.frank.ffmpeg.R
import com.frank.ffmpeg.util.FilterTypeUtil

class CameraFilterActivity : BaseActivity() {

    private var cameraView: BeautyCameraView ?= null

    private var index: Int = 0

    private val filterType: Array<BeautyFilterType> = arrayOf(
        BeautyFilterType.NONE,
        BeautyFilterType.BLUR,
        BeautyFilterType.COLOR_INVERT,
        BeautyFilterType.HUE,
        BeautyFilterType.WHITE_BALANCE,
        BeautyFilterType.SKETCH,
        BeautyFilterType.OVERLAY
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
            showToast(getString(FilterTypeUtil.filterTypeToNameId(filterType[index])))
        }
    }

    override fun onSelectedFile(filePath: String) {

    }

}