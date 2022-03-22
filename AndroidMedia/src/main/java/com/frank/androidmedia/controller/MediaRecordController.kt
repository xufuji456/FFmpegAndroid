package com.frank.androidmedia.controller

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
    private var mMediaRecorder: MediaRecorder? = null

    private fun initMediaRecord(surface: Surface, outputPath: String) {
        // open camera
        mCamera = Camera.open()
        mCamera!!.setDisplayOrientation(90)
        mCamera!!.unlock()
        // Note: pay attention to calling order
        mMediaRecorder?.setCamera(mCamera)
        mMediaRecorder?.setAudioSource(MediaRecorder.AudioSource.MIC)
        mMediaRecorder?.setVideoSource(MediaRecorder.VideoSource.CAMERA)
        if (usingProfile) {
            // QUALITY_480P QUALITY_720P QUALITY_1080P QUALITY_2160P
            val profile = CamcorderProfile.get(CamcorderProfile.QUALITY_720P)
            mMediaRecorder?.setProfile(profile)
        } else {
            mMediaRecorder?.setOutputFormat(MediaRecorder.OutputFormat.MPEG_4)
            mMediaRecorder?.setAudioEncoder(MediaRecorder.AudioEncoder.AAC)
            mMediaRecorder?.setVideoEncoder(MediaRecorder.VideoEncoder.H264)
//            mMediaRecorder?.setVideoSize(640, 480)
//            mMediaRecorder?.setVideoEncodingBitRate(5000 * 1000)
//            mMediaRecorder?.setVideoFrameRate(25)
//            mMediaRecorder?.setAudioChannels(2)
//            mMediaRecorder?.setAudioSamplingRate(48000)
        }
        mMediaRecorder?.setOutputFile(outputPath)
        mMediaRecorder?.setPreviewDisplay(surface)
    }

    fun startRecord(surface: Surface, outputPath: String) {
        if (mMediaRecorder == null) {
            mMediaRecorder = MediaRecorder()
        }

        initMediaRecord(surface, outputPath)

        try {
            mMediaRecorder?.prepare()
            mMediaRecorder?.start()
        } catch (e: Exception) {
            Log.e("MediaRecorder", "start recorder error=$e")
        }
        Log.i("MediaRecorder", "startRecord...")
    }

    fun stopRecord() {
        if (mMediaRecorder != null) {
            mMediaRecorder?.stop()
            mMediaRecorder?.reset()
        }
        mCamera?.stopPreview()
        Log.i("MediaRecorder", "stopRecord...")
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