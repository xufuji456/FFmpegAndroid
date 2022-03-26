package com.frank.androidmedia.controller

import android.content.Context
import android.content.Intent
import android.hardware.Camera
import android.media.CamcorderProfile
import android.media.MediaRecorder
import android.util.Log
import android.view.Surface

/**
 * Using MediaRecorder to record a media file.
 *
 * @author frank
 * @date 2022/3/21
 */
open class MediaRecordController {

    private val usingProfile = true
    private var mCamera: Camera? = null
    private var mOutputPath: String? = null
    private var mMediaRecorder: MediaRecorder? = null
    private var mMediaProjectionController: MediaProjectionController? = null

    private fun initMediaRecord(videoSource: Int, surface: Surface?, outputPath: String) {
        if (videoSource == MediaRecorder.VideoSource.CAMERA
                || videoSource == MediaRecorder.VideoSource.DEFAULT) {
            // open camera
            mCamera = Camera.open()
            mCamera!!.setDisplayOrientation(90)
            mCamera!!.unlock()
            mMediaRecorder?.setCamera(mCamera)
        }
        // Note: pay attention to calling order
        mMediaRecorder?.setVideoSource(videoSource)
        mMediaRecorder?.setAudioSource(MediaRecorder.AudioSource.MIC)
        if (usingProfile) {
            // QUALITY_480P QUALITY_720P QUALITY_1080P QUALITY_2160P
            val profile = CamcorderProfile.get(CamcorderProfile.QUALITY_720P)
            mMediaRecorder?.setProfile(profile)
        } else {
            mMediaRecorder?.setOutputFormat(MediaRecorder.OutputFormat.MPEG_4)
            mMediaRecorder?.setAudioEncoder(MediaRecorder.AudioEncoder.AAC)
            mMediaRecorder?.setVideoEncoder(MediaRecorder.VideoEncoder.H264)
            mMediaRecorder?.setVideoSize(640, 480)
            mMediaRecorder?.setVideoEncodingBitRate(5000 * 1000)
            mMediaRecorder?.setVideoFrameRate(25)
            mMediaRecorder?.setAudioChannels(2)
            mMediaRecorder?.setAudioSamplingRate(48000)
        }
        mMediaRecorder?.setOutputFile(outputPath)
        if (surface != null && (videoSource == MediaRecorder.VideoSource.CAMERA
                        || videoSource == MediaRecorder.VideoSource.DEFAULT)) {
            mMediaRecorder?.setPreviewDisplay(surface)
        }
    }

    private fun startRecordInternal(videoSource: Int, surface: Surface?, outputPath: String) {
        initMediaRecord(videoSource, surface, outputPath)
        try {
            mMediaRecorder?.prepare()
            if (videoSource == MediaRecorder.VideoSource.SURFACE) {
                mMediaProjectionController?.createVirtualDisplay(mMediaRecorder?.surface!!)
            }
            mMediaRecorder?.start()
        } catch (e: Exception) {
            Log.e("MediaRecorder", "start recorder error=$e")
        }
    }

    /**
     * Start record camera or screen
     * @param videoSource the source of video, see {@link MediaRecorderVideoSource.CAMERA}
     *                    or {@link MediaRecorder.VideoSource.SURFACE}
     * @param surface     the Surface to preview, when videoSource = MediaRecorderVideoSource.CAMERA
     * @param context     the Context of Activity
     * @param outputPath  the output path to save media file
     */
    fun startRecord(videoSource: Int, surface: Surface?, context: Context, outputPath: String) {
        if (mMediaRecorder == null) {
            mMediaRecorder = MediaRecorder()
        }

        if (videoSource == MediaRecorder.VideoSource.SURFACE) {
            mOutputPath = outputPath
            mMediaProjectionController = MediaProjectionController(MediaProjectionController.TYPE_SCREEN_RECORD)
            mMediaProjectionController?.startScreenRecord(context)
            return
        }

        startRecordInternal(videoSource, surface, outputPath)
        Log.i("MediaRecorder", "startRecord...")
    }

    /**
     * Stop recording camera or screen,
     * and release everything.
     */
    fun stopRecord() {
        if (mMediaRecorder != null) {
            mMediaRecorder?.stop()
            mMediaRecorder?.reset()
        }
        if (mCamera != null) {
            mCamera!!.stopPreview()
        }
        if (mMediaProjectionController != null) {
            mMediaProjectionController!!.stopScreenRecord()
        }
        Log.i("MediaRecorder", "stopRecord...")
    }

    fun onActivityResult(requestCode: Int, resultCode: Int, data: Intent) {
        if (requestCode == mMediaProjectionController?.getRequestCode()) {
            mMediaProjectionController?.onActivityResult(resultCode, data)
            startRecordInternal(MediaRecorder.VideoSource.SURFACE, mMediaRecorder?.surface, mOutputPath!!)
        }
    }

    fun release() {
        if (mMediaRecorder != null) {
            mMediaRecorder?.release()
            mMediaRecorder = null
        }
        if (mCamera != null) {
            mCamera!!.release()
            mCamera!!.lock()
        }
    }
}