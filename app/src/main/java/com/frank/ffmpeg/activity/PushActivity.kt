package com.frank.ffmpeg.activity

import android.os.Bundle
import android.text.TextUtils
import android.util.Log
import android.view.View
import android.widget.EditText

import com.frank.ffmpeg.FFmpegPusher
import com.frank.ffmpeg.R

import java.io.File

/**
 * Using FFmpeg to push http-flv stream
 * Created by frank on 2018/2/2.
 */
class PushActivity : BaseActivity() {

    private var editInputPath: EditText? = null

    private var editLiveURL: EditText? = null

    override val layoutId: Int
        get() = R.layout.activity_push

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        hideActionBar()
        initView()
    }

    private fun initView() {
        editInputPath = getView(R.id.edit_file_path)
        editLiveURL   = getView(R.id.edit_live_url)
        editInputPath!!.setText(INPUT_PATH)
        editLiveURL!!.setText(LIVE_URL)

        initViewsWithClick(R.id.btn_push_stream)
    }

    private fun startPushStreaming() {
        val filePath = editInputPath!!.text.toString()
        val liveUrl  = editLiveURL!!.text.toString()

        if (!TextUtils.isEmpty(filePath) && !TextUtils.isEmpty(filePath)) {
            Thread(Runnable {
                FFmpegPusher().pushStream(filePath, liveUrl)
            }).start()
        }
    }

    override fun onViewClick(view: View) {
        if (view.id == R.id.btn_push_stream) {
            startPushStreaming()
        }
    }

    override fun onSelectedFile(filePath: String) {

    }

    companion object {

        // storage/emulated/0/beyond.mp4
        private const val INPUT_PATH = "http://clips.vorwaerts-gmbh.de/big_buck_bunny.mp4"
        private const val LIVE_URL = "rtmp://192.168.17.168/live/stream"
    }
}
