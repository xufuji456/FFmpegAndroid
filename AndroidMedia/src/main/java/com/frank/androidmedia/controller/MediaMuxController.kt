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
        // 1、create MediaMuxer
        val mediaMuxer = MediaMuxer(outputPath, MediaMuxer.OutputFormat.MUXER_OUTPUT_MPEG_4)
        val mediaExtractor = MediaExtractor()
        try {
            var videoIndex = 0
            var audioIndex = 0
            var audioFormat: MediaFormat? = null
            var videoFormat: MediaFormat? = null
            var finished = false
            val bufferInfo = MediaCodec.BufferInfo()
            val inputBuffer = ByteBuffer.allocate(2 * 1024 * 1024)
            mediaExtractor.setDataSource(inputPath)
            // select track with mimetype
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
            // 2、add MediaFormat into track
            if (videoFormat != null) {
                mediaMuxer.addTrack(videoFormat)
            }
            if (audioFormat != null) {
                mediaMuxer.addTrack(audioFormat)
            }
            // 3、start the muxer
            mediaMuxer.start()

            while (!finished) {
                // demux media stream
                val sampleSize = mediaExtractor.readSampleData(inputBuffer, 0)
                if (sampleSize > 0) {
                    bufferInfo.size = sampleSize
                    bufferInfo.flags = mediaExtractor.sampleFlags
                    bufferInfo.presentationTimeUs = mediaExtractor.sampleTime
                    // 4、call MediaMuxer to mux media stream
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
            // 5、release resource
            mediaMuxer.release()
            mediaExtractor.release()
            return !happenError
        }
    }

}