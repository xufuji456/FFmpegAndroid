package com.frank.ffmpeg.activity

import android.annotation.SuppressLint
import android.os.Bundle
import android.os.Environment
import android.os.Handler
import android.os.Message
import android.util.Log
import android.view.View
import android.widget.LinearLayout
import android.widget.TextView

import com.frank.ffmpeg.FFmpegCmd
import com.frank.ffmpeg.R
import com.frank.ffmpeg.handler.FFmpegHandler
import com.frank.ffmpeg.util.FFmpegUtil
import com.frank.ffmpeg.util.FileUtil
import com.frank.ffmpeg.util.ThreadPoolUtil

import java.io.File
import java.util.Locale

import com.frank.ffmpeg.handler.FFmpegHandler.MSG_BEGIN
import com.frank.ffmpeg.handler.FFmpegHandler.MSG_FINISH
import com.frank.ffmpeg.handler.FFmpegHandler.MSG_PROGRESS

/**
 * using ffmpeg to handle media
 * Created by frank on 2018/1/23.
 */
class MediaHandleActivity : BaseActivity() {
    private val audioFile = PATH + File.separator + "tiger.mp3"

    private var layoutProgress: LinearLayout? = null
    private var txtProgress: TextView? = null
    private var viewId: Int = 0
    private var layoutMediaHandle: LinearLayout? = null
    private var ffmpegHandler: FFmpegHandler? = null

    @SuppressLint("HandlerLeak")
    private val mHandler = object : Handler() {
        override fun handleMessage(msg: Message) {
            super.handleMessage(msg)
            when (msg.what) {
                MSG_BEGIN -> {
                    layoutProgress!!.visibility = View.VISIBLE
                    layoutMediaHandle!!.visibility = View.GONE
                }
                MSG_FINISH -> {
                    layoutProgress!!.visibility = View.GONE
                    layoutMediaHandle!!.visibility = View.VISIBLE
                }
                MSG_PROGRESS -> {
                    val progress = msg.arg1
                    if (progress > 0) {
                        txtProgress!!.visibility = View.VISIBLE
                        txtProgress!!.text = String.format(Locale.getDefault(), "%d%%", progress)
                    } else {
                        txtProgress!!.visibility = View.INVISIBLE
                    }
                }
                else -> {
                }
            }
        }
    }

    override val layoutId: Int
        get() = R.layout.activity_media_handle

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        hideActionBar()
        initView()
        ffmpegHandler = FFmpegHandler(mHandler)
    }

    private fun initView() {
        layoutProgress = getView(R.id.layout_progress)
        txtProgress = getView(R.id.txt_progress)
        layoutMediaHandle = getView(R.id.layout_media_handle)
        initViewsWithClick(
                R.id.btn_mux,
                R.id.btn_extract_audio,
                R.id.btn_extract_video,
                R.id.btn_dubbing
        )
    }

    override fun onViewClick(view: View) {
        viewId = view.id
        selectFile()
    }

    override fun onSelectedFile(filePath: String) {
        doHandleMedia(filePath)
    }

    /**
     * Using ffmpeg cmd to handle media
     *
     * @param srcFile srcFile
     */
    private fun doHandleMedia(srcFile: String) {
        var commandLine: Array<String>? = null
        if (!FileUtil.checkFileExist(srcFile)) {
            return
        }
        if (!FileUtil.isVideo(srcFile)) {
            showToast(getString(R.string.wrong_video_format))
            return
        }

        when (viewId) {
            R.id.btn_mux//mux:pure video and pure audio
            -> {
                ThreadPoolUtil.executeSingleThreadPool (Runnable { mediaMux(srcFile) })
                return
            }
            R.id.btn_extract_audio//extract audio
            -> {
                val extractAudio = PATH + File.separator + "extractAudio.aac"
                commandLine = FFmpegUtil.extractAudio(srcFile, extractAudio)
            }
            R.id.btn_extract_video//extract video
            -> {
                val extractVideo = PATH + File.separator + "extractVideo.mp4"
                commandLine = FFmpegUtil.extractVideo(srcFile, extractVideo)
            }
            R.id.btn_dubbing//dubbing
            -> {
                ThreadPoolUtil.executeSingleThreadPool (Runnable{ mediaDubbing(srcFile) })
                return
            }
            else -> {
            }
        }
        if (ffmpegHandler != null) {
            ffmpegHandler!!.executeFFmpegCmd(commandLine)
        }
    }

    private fun muxVideoAndAudio(videoPath: String, outputPath: String) {
        var commandLine = FFmpegUtil.mediaMux(videoPath, audioFile, true, outputPath)
        var result = FFmpegCmd.executeSync(commandLine)
        if (result != 0) {
            commandLine = FFmpegUtil.mediaMux(videoPath, audioFile, false, outputPath)
            result = FFmpegCmd.executeSync(commandLine)
            Log.e(TAG, "mux audio and video result=$result")
        }
    }

    private fun mediaMux(srcFile: String) {
        mHandler.sendEmptyMessage(MSG_BEGIN)
        val suffix = FileUtil.getFileSuffix(srcFile)
        val muxPath = PATH + File.separator + "mux" + suffix
        Log.e(TAG, "muxPath=$muxPath")
        muxVideoAndAudio(srcFile, muxPath)
        mHandler.sendEmptyMessage(MSG_FINISH)
    }

    private fun mediaDubbing(srcFile: String) {
        mHandler.sendEmptyMessage(MSG_BEGIN)
        val dubbingSuffix = FileUtil.getFileSuffix(srcFile)
        val dubbingPath = PATH + File.separator + "dubbing" + dubbingSuffix
        val temp = PATH + File.separator + "temp" + dubbingSuffix
        val commandLine1 = FFmpegUtil.extractVideo(srcFile, temp)
        val dubbingResult = FFmpegCmd.executeSync(commandLine1)
        if (dubbingResult != 0) {
            Log.e(TAG, "extract video fail, result=$dubbingResult")
            return
        }
        muxVideoAndAudio(temp, dubbingPath)
        FileUtil.deleteFile(temp)
        mHandler.sendEmptyMessage(MSG_FINISH)
    }

    override fun onDestroy() {
        super.onDestroy()
        mHandler.removeCallbacksAndMessages(null)
    }

    companion object {

        private val TAG = MediaHandleActivity::class.java.simpleName
        private val PATH = Environment.getExternalStorageDirectory().path
    }
}
