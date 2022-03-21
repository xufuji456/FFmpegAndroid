package com.frank.androidmedia.controller

import android.media.MediaCodec
import android.media.MediaExtractor
import android.media.MediaFormat
import android.media.MediaMuxer
import android.util.Log
import java.lang.Exception
import java.nio.ByteBuffer

/**
 * Using MediaExtractor to demux media format.
 * Using MediaMuxer to mux media format again.
 * @author frank
 * @date 2022/3/21
 */
open class MediaMuxController {

    fun muxMediaFile(inputPath: String, outputPath: String): Boolean {
        if (inputPath.isEmpty() || outputPath.isEmpty()) {
            return false
        }
        var happenError = false
        val mediaMuxer = MediaMuxer(outputPath, MediaMuxer.OutputFormat.MUXER_OUTPUT_MPEG_4)
        val mediaExtractor = MediaExtractor()
        try {
            var videoIndex = 0
            var audioIndex = 0
            var audioFormat: MediaFormat? = null
            var videoFormat: MediaFormat? = null
            mediaExtractor.setDataSource(inputPath)
            for (i in 0 until mediaExtractor.trackCount) {
                val mediaFormat = mediaExtractor.getTrackFormat(i)
                val mimeType = mediaFormat.getString(MediaFormat.KEY_MIME)
                if (mimeType != null && mimeType.startsWith("video")) {
                    videoIndex = i
                    videoFormat = mediaFormat
                    mediaExtractor.selectTrack(i)
                } else if (mimeType != null && mimeType.startsWith("audio") && audioFormat == null) {
                    audioIndex = i
                    audioFormat = mediaFormat
                    mediaExtractor.selectTrack(i)
                }
            }
            if (videoFormat != null) {
                mediaMuxer.addTrack(videoFormat)
            }
            if (audioFormat != null) {
                mediaMuxer.addTrack(audioFormat)
            }

            var finished = false
            val bufferInfo = MediaCodec.BufferInfo()
            val inputBuffer = ByteBuffer.allocate(2 * 1024 * 1024)

            mediaMuxer.start()

            while (!finished) {
                val sampleSize = mediaExtractor.readSampleData(inputBuffer, 0)
                if (sampleSize > 0) {
                    bufferInfo.size = sampleSize
                    bufferInfo.flags = mediaExtractor.sampleFlags
                    bufferInfo.presentationTimeUs = mediaExtractor.sampleTime
                    if (mediaExtractor.sampleTrackIndex == videoIndex) {
                        mediaMuxer.writeSampleData(videoIndex, inputBuffer, bufferInfo)
                    } else if (mediaExtractor.sampleTrackIndex == audioIndex) {
                        mediaMuxer.writeSampleData(audioIndex, inputBuffer, bufferInfo)
                    }
                    inputBuffer.flip()
                    mediaExtractor.advance()
                } else if (sampleSize < 0) {
                    finished = true
                }
            }

        } catch (e: Exception) {
            Log.e("MediaMuxController", "mux error=$e")
            happenError = true
        } finally {
            try {
                mediaMuxer.release()
                mediaExtractor.release()
            } catch (e1: Exception) {
                Log.e("MediaMuxController", "release error=$e1")
            }
            return !happenError
        }
    }

}