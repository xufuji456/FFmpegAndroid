package com.frank.ffmpeg.activity

import android.os.Bundle
import android.view.SurfaceView
import android.view.View
import com.frank.camerafilter.widget.BeautyCameraView
import com.frank.ffmpeg.FFMediaPlayer
import com.frank.ffmpeg.R

class CameraFilterActivity : BaseActivity() {

    private var cameraView: BeautyCameraView ?= null

    override val layoutId: Int
        get() = R.layout.activity_camera_filter

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        initView()
    }

    fun initView() {
        cameraView = getView(R.id.surface_camera_filter)
    }

    override fun onViewClick(view: View) {

    }

    override fun onSelectedFile(filePath: String) {

    }

}