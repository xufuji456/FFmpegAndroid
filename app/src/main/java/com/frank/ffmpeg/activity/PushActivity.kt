package com.frank.ffmpeg.activity

import android.os.Bundle
import android.text.TextUtils
import android.util.Log
import android.view.View
import android.widget.EditText

import com.frank.ffmpeg.Pusher
import com.frank.ffmpeg.R

import java.io.File

/**
 * Using FFmpeg to push stream
 * Created by frank on 2018/2/2.
 */
class PushActivity : BaseActivity() {

    private var editFilePath: EditText? = null

    private var editLiveURL: EditText? = null

    override val layoutId: Int
        get() = R.layout.activity_push

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        hideActionBar()
        initView()
    }

    private fun initView() {
        editFilePath = getView(R.id.edit_file_path)
        editLiveURL = getView(R.id.edit_live_url)
        editFilePath!!.setText(FILE_PATH)
        editLiveURL!!.setText(LIVE_URL)

        initViewsWithClick(R.id.btn_push_stream)
    }

    private fun startPushStreaming() {
        //TODO the video format is flv
        val filePath = editFilePath!!.text.toString()
        val liveUrl = editLiveURL!!.text.toString()
        Log.i(TAG, "filePath=$filePath")
        Log.i(TAG, "liveUrl=$liveUrl")

        if (!TextUtils.isEmpty(filePath) && !TextUtils.isEmpty(filePath)) {
            val file = File(filePath)
            if (file.exists()) {
                Thread(Runnable {
                    Pusher().pushStream(filePath, liveUrl)
                }).start()
            } else {
                showToast(getString(R.string.file_not_found))
            }
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

        private val TAG = PushActivity::class.java.simpleName
        private const val FILE_PATH = "storage/emulated/0/hello.flv"
        private const val LIVE_URL = "rtmp://192.168.1.104/live/stream"
    }
}
