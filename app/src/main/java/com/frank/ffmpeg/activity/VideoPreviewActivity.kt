package com.frank.ffmpeg.activity

import android.annotation.SuppressLint
import android.media.MediaPlayer
import android.os.Bundle
import android.os.Handler
import android.os.Message
import android.text.TextUtils
import android.util.Log
import android.view.Surface
import android.view.SurfaceHolder
import android.view.SurfaceView
import android.view.View

import com.frank.ffmpeg.R
import com.frank.ffmpeg.view.VideoPreviewBar

import java.io.IOException

import com.frank.ffmpeg.handler.FFmpegHandler.MSG_TOAST

/**
 * Preview the thumbnail of video when seeking
 * Created by frank on 2019/11/16.
 */

class VideoPreviewActivity : BaseActivity(), VideoPreviewBar.PreviewBarCallback {

    private var mediaPlayer: MediaPlayer? = null
    private var surfaceVideo: SurfaceView? = null
    private var videoPreviewBar: VideoPreviewBar? = null

    @SuppressLint("HandlerLeak")
    private val mHandler = object : Handler() {
        override fun handleMessage(msg: Message) {
            super.handleMessage(msg)
            if (msg.what == MSG_UPDATE) {
                if (videoPreviewBar != null && mediaPlayer != null) {
                    videoPreviewBar!!.updateProgress(mediaPlayer!!.currentPosition)
                }
                this.sendEmptyMessageDelayed(MSG_UPDATE, TIME_UPDATE.toLong())
            } else if (msg.what == MSG_TOAST) {
                showToast(getString(R.string.please_click_select))
            }
        }
    }

    override val layoutId: Int
        get() = R.layout.activity_preview

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        initView()
        mHandler.sendEmptyMessageDelayed(MSG_TOAST, 500)
    }

    private fun initView() {
        surfaceVideo = getView(R.id.surface_view)
        videoPreviewBar = getView(R.id.preview_video)
    }

    private fun setPlayCallback(filePath: String, surfaceView: SurfaceView) {
        surfaceView.holder.addCallback(object : SurfaceHolder.Callback {
            override fun surfaceCreated(holder: SurfaceHolder) {
                doPlay(filePath, holder.surface)
            }

            override fun surfaceChanged(holder: SurfaceHolder, format: Int, width: Int, height: Int) {

            }

            override fun surfaceDestroyed(holder: SurfaceHolder) {

            }
        })
    }

    private fun setPrepareListener() {
        if (mediaPlayer == null) {
            return
        }
        mediaPlayer!!.setOnPreparedListener {
            Log.i(TAG, "onPrepared...")
            mediaPlayer!!.start()
            mHandler.sendEmptyMessage(MSG_UPDATE)
        }
    }

    private fun doPlay(filePath: String, surface: Surface?) {
        if (surface == null || TextUtils.isEmpty(filePath)) {
            return
        }
        releasePlayer()
        try {
            mediaPlayer = MediaPlayer()
            setPrepareListener()
            mediaPlayer!!.setDataSource(filePath)
            mediaPlayer!!.setSurface(surface)
            mediaPlayer!!.prepareAsync()
        } catch (e: IOException) {
            e.printStackTrace()
        }

    }

    override fun onViewClick(view: View) {

    }

    override fun onSelectedFile(filePath: String) {
        setPlayCallback(filePath, surfaceVideo!!)
        videoPreviewBar!!.init(filePath, this)
    }

    override fun onStopTracking(progress: Long) {
        if (mediaPlayer != null) {
            Log.i(TAG, "onStopTracking progress=$progress")
            mediaPlayer!!.seekTo(progress.toInt())
        }
    }

    private fun releasePlayer() {
        if (mediaPlayer != null) {
            mediaPlayer!!.stop()
            mediaPlayer!!.release()
            mediaPlayer = null
        }
    }

    override fun onDestroy() {
        super.onDestroy()
        releasePlayer()
        if (videoPreviewBar != null) {
            videoPreviewBar!!.release()
        }
    }

    companion object {

        private val TAG = VideoPreviewActivity::class.java.simpleName
        private const val TIME_UPDATE = 1000
        private const val MSG_UPDATE = 1234
    }

}
