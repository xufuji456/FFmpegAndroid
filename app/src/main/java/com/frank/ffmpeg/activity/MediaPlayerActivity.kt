package com.frank.ffmpeg.activity

import android.os.Bundle
import android.util.Log
import android.view.SurfaceHolder
import android.view.SurfaceView
import android.view.View
import android.widget.Button

import com.frank.ffmpeg.MediaPlayer
import com.frank.ffmpeg.R
import com.frank.ffmpeg.util.FileUtil

/**
 * mediaPlayer, which decode by software
 * Created by frank on 2018/2/12.
 */

class MediaPlayerActivity : BaseActivity(), SurfaceHolder.Callback {

    private var surfaceHolder: SurfaceHolder? = null

    private var mediaPlayer: MediaPlayer? = null

    private var surfaceCreated: Boolean = false

    private var btnSelectFile: Button? = null

    override val layoutId: Int
        get() = R.layout.activity_media_player

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        hideActionBar()
        initView()
        initPlayer()
    }

    private fun initView() {
        val surfaceView = getView<SurfaceView>(R.id.surface_media)
        surfaceHolder = surfaceView.holder
        surfaceHolder!!.addCallback(this)
        btnSelectFile = getView(R.id.btn_select_file)
        initViewsWithClick(R.id.btn_select_file)
    }

    private fun initPlayer() {
        mediaPlayer = MediaPlayer()
    }

    override fun surfaceCreated(holder: SurfaceHolder) {
        surfaceCreated = true
    }

    override fun surfaceChanged(holder: SurfaceHolder, format: Int, width: Int, height: Int) {
        Log.i(TAG, "surfaceChanged")
    }

    override fun surfaceDestroyed(holder: SurfaceHolder) {
        Log.i(TAG, "surfaceDestroyed")
    }

    override fun onDestroy() {
        super.onDestroy()
        if (mediaPlayer != null) {
            mediaPlayer!!.release()
            mediaPlayer = null
        }
    }

    private fun startPlay(filePath: String) {
        Thread(Runnable {
            val result = mediaPlayer!!.setup(filePath, surfaceHolder!!.surface)
            if (result < 0) {
                Log.e(TAG, "mediaPlayer setup error!")
                return@Runnable
            }
            mediaPlayer!!.play()
        }).start()
    }

    override fun onSelectedFile(filePath: String) {
        if (!FileUtil.checkFileExist(filePath)) {
            return
        }
        if (surfaceCreated) {
            btnSelectFile!!.visibility = View.GONE
            startPlay(filePath)
        }
    }

    override fun onViewClick(view: View) {
        if (view.id == R.id.btn_select_file) {
            selectFile()
        }
    }

    companion object {

        private val TAG = MediaPlayerActivity::class.java.simpleName
    }
}
