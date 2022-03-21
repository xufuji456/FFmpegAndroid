package com.frank.androidmedia.controller

import android.media.*
import android.os.SystemClock
import android.util.Log
import java.lang.Exception

/**
 * 1. Using MediaExtractor to demux audio.
 * 2. Using MediaCodec to decode audio.
 * 3. Using AudioTrack to play audio.
 * See also AAduio, oboe, openSL ES.
 *
 * @author frank
 * @date 2022/3/21
 */
open class AudioTrackController {

    companion object {
        val TAG: String = AudioTrackController::class.java.simpleName

        private const val DEQUEUE_TIME = (10 * 1000).toLong()
        private const val SLEEP_TIME: Long = 20
    }

    private var audioTrack: AudioTrack? = null
    private var mediaCodec: MediaCodec? = null
    private var mediaExtractor: MediaExtractor? = null

    private fun parseAudioFormat(path: String): MediaFormat? {
        mediaExtractor = MediaExtractor()
        try {
            mediaExtractor?.setDataSource(path)
            for (i in 0 until mediaExtractor!!.trackCount) {
                val mediaFormat = mediaExtractor!!.getTrackFormat(i)
                val mimeType = mediaFormat.getString(MediaFormat.KEY_MIME)
                if (mimeType != null && mimeType.startsWith("audio")) {
                    mediaExtractor!!.selectTrack(i)
                    return mediaFormat
                }
            }
        } catch (e: Exception) {
            Log.e(TAG, "parseAudioFormat err=$e")
        }
        return null
    }

    private fun initMediaCodec(mediaFormat: MediaFormat): Boolean {
        val mimeType = mediaFormat.getString(MediaFormat.KEY_MIME)
        mediaCodec = MediaCodec.createDecoderByType(mimeType)
        return try {
            mediaCodec!!.configure(mediaFormat, null, null, 0)
            mediaCodec!!.start()
            true
        } catch (e: Exception) {
            Log.e(TAG, "initMediaCodec err=$e")
            false
        }
    }

    private fun initAudioTrack(mediaFormat: MediaFormat): Boolean {
        val sampleRate = mediaFormat.getInteger(MediaFormat.KEY_SAMPLE_RATE)
        val channelCount = mediaFormat.getInteger(MediaFormat.KEY_CHANNEL_COUNT)
        val channelConfig = if (channelCount == 1) {
            AudioFormat.CHANNEL_OUT_MONO
        } else  {
            AudioFormat.CHANNEL_OUT_STEREO
        }
        val audioFormat = AudioFormat.ENCODING_PCM_16BIT
        val bufferSize = AudioTrack.getMinBufferSize(sampleRate, channelConfig, audioFormat)
        Log.e(TAG, "sampleRate=$sampleRate, channelCount=$channelCount, bufferSize=$bufferSize")

        try {
            audioTrack = AudioTrack(AudioManager.STREAM_MUSIC, sampleRate, channelConfig, audioFormat, bufferSize, AudioTrack.MODE_STREAM)
            audioTrack!!.play()
//            audioTrack!!.attachAuxEffect()
        } catch (e: Exception) {
            Log.e(TAG, "initAudioTrack err=$e")
            return false
        }
        return true
    }

    private fun release() {
        if (mediaExtractor != null) {
            mediaExtractor!!.release()
            mediaExtractor = null
        }
        if (mediaCodec != null) {
            mediaCodec!!.release()
            mediaCodec = null
        }
        if (audioTrack != null) {
            audioTrack!!.release()
            audioTrack = null
        }
    }

    fun playAudio(path: String) {
        val data = ByteArray(10 * 1024)
        var finished = false
        val bufferInfo = MediaCodec.BufferInfo()
        val mediaFormat = parseAudioFormat(path) ?: return release()
        var result = initMediaCodec(mediaFormat)
        if (!result) {
            return release()
        }
        result = initAudioTrack(mediaFormat)
        if (!result) {
            return release()
        }

        while (!finished) {
            val inputIndex = mediaCodec!!.dequeueInputBuffer(DEQUEUE_TIME)
            if (inputIndex >= 0) {
                val inputBuffer = mediaCodec!!.getInputBuffer(inputIndex)
                // demux
                val sampleSize = mediaExtractor!!.readSampleData(inputBuffer!!, 0)
                Log.e(TAG, "sampleSize=$sampleSize")
                // decode
                if (sampleSize < 0) {
                    mediaCodec!!.queueInputBuffer(inputIndex, 0, 0, 0, MediaCodec.BUFFER_FLAG_END_OF_STREAM)
                    finished = true
                } else {
                    mediaCodec!!.queueInputBuffer(inputIndex, 0, sampleSize, mediaExtractor!!.sampleTime, mediaExtractor!!.sampleFlags)
                    mediaExtractor!!.advance()
                }
            }

            val outputIndex = mediaCodec!!.dequeueOutputBuffer(bufferInfo, DEQUEUE_TIME)
            // play
            if (outputIndex >= 0) {
                val outputBuffer = mediaCodec!!.getOutputBuffer(outputIndex)
                val size = outputBuffer!!.limit()
                Log.e(TAG, "outputIndex=$outputIndex, position=" + outputBuffer.position() + " ,limit=" + size)
                outputBuffer.get(data, outputBuffer.position(), size)
                audioTrack!!.write(data, 0, size)
                mediaCodec!!.releaseOutputBuffer(outputIndex, false)
                SystemClock.sleep(SLEEP_TIME)
            }
        }

        release()
    }

}