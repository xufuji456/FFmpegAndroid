package com.frank.androidmedia.controller

import android.media.AudioFormat
import android.media.AudioRecord
import android.media.MediaRecorder
import android.util.Log
import com.frank.androidmedia.util.WavUtil
import java.io.File
import java.io.FileOutputStream
import java.io.IOException
import java.lang.Exception

/**
 * Using AudioRecord to record an audio segment.
 * See also MediaRecord, which used to record media.
 *
 * @author frank
 * @date 2022/3/22
 */
open class AudioRecordController {

    companion object {
        val TAG: String = AudioTrackController::class.java.simpleName
    }

    private var minBufferSize = 0
    private var mAudioRecord: AudioRecord? = null
    private var mRecordThread: RecordThread? = null

    private val enableAudioProcessor = false
    private var mAudioProcessController: AudioProcessController? = null

    private fun initAudioRecord() {
        val sampleRate    = 44100
        val audioFormat   = AudioFormat.ENCODING_PCM_16BIT
        val channelConfig = AudioFormat.CHANNEL_IN_STEREO
        minBufferSize     = AudioRecord.getMinBufferSize(sampleRate, channelConfig, audioFormat)
        mAudioRecord      = AudioRecord( MediaRecorder.AudioSource.MIC,
                                         sampleRate,
                                         channelConfig,
                                         audioFormat,
                                         minBufferSize)

        if (enableAudioProcessor) {
            mAudioProcessController = AudioProcessController()
            var result:Boolean? = mAudioProcessController?.initAEC(mAudioRecord?.audioSessionId!!)
            Log.e(TAG, "init AEC result=$result")
            result = mAudioProcessController?.initAGC(mAudioRecord?.audioSessionId!!)
            Log.e(TAG, "init AGC result=$result")
            result = mAudioProcessController?.initNS(mAudioRecord?.audioSessionId!!)
            Log.e(TAG, "init NS result=$result")
        }
    }

    private class RecordThread(recordPath: String, audioRecord: AudioRecord, bufferSize: Int) : Thread() {

        var isRecording = false
        private val lock = Object()
        private var mPath: String? = null
        private lateinit var mData: ByteArray
        private var mBufferSize = 0
        private var mAudioRecord: AudioRecord? = null
        private var mOutputStream: FileOutputStream? = null

        init {
            mPath = recordPath
            isRecording = true
            mBufferSize = bufferSize
            mAudioRecord = audioRecord
        }

        override fun run() {
            super.run()

            try {
                mData = ByteArray(mBufferSize)
                mOutputStream = FileOutputStream(mPath)
            } catch (e: Exception) {
                Log.e(TAG, "open file error=$e")
                isRecording = false
            }

            while (isRecording) {
                synchronized(lock) {
                    if (isRecording) {
                        val size: Int = mAudioRecord?.read(mData, 0, mBufferSize)!!
                        if (size > 0) {
                            mOutputStream?.write(mData, 0, size)
                        } else if (size < 0) {
                            Log.e(TAG, "read data error, size=$size")
                        }
                    }
                }
            }

            if (mOutputStream != null) {
                try {
                    mOutputStream?.close()
                } catch (e: IOException) {
                    e.printStackTrace()
                }
            }
            // convert pcm to wav
            val wavPath = File(mPath).parent + "/test.wav"
            WavUtil.makePCMToWAVFile(mPath, wavPath, true)
        }
    }

    fun startRecord(recordPath: String) {
        if (mAudioRecord == null) {
            try {
                initAudioRecord()
            } catch (e: Exception) {
                Log.e(TAG, "init AudioRecord error=$e")
                return
            }
        }

        if (mAudioRecord!!.recordingState == AudioRecord.RECORDSTATE_RECORDING) {
            Log.e(TAG, "is recording audio...")
            return
        }

        let {
            Log.i(TAG, "start record...")
            mAudioRecord!!.startRecording()
            mRecordThread = RecordThread(recordPath, mAudioRecord!!, minBufferSize)
            mRecordThread!!.start()
        }
    }

    fun stopRecord() {
        Log.i(TAG, "stop record...")
        if (mRecordThread != null) {
            mRecordThread!!.isRecording = false
            mRecordThread!!.interrupt()
            mRecordThread = null
        }
        if (mAudioRecord != null) {
            mAudioRecord!!.stop()
        }
    }

    fun release() {
        if (mAudioRecord != null) {
            mAudioRecord!!.release()
            mAudioRecord = null
        }
        if (mAudioProcessController != null) {
            mAudioProcessController!!.release()
            mAudioProcessController = null
        }
    }
}