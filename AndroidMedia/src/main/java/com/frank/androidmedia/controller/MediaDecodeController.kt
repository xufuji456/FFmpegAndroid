package com.frank.androidmedia.controller

import android.media.MediaCodec
import android.media.MediaExtractor
import android.media.MediaFormat
import android.os.SystemClock
import android.util.Log
import android.view.Surface

/**
 * The controller of MediaExtractor and MediaCodec.
 * Use MediaExtractor to extract data of media tracks.
 * And use MediaCodec to decode video frame from extractor.
 *
 * @author frank
 * @date 2022/3/18
 */

open class MediaDecodeController(val mSurface: Surface, val mFilePath: String, val mCallback: OnDataCallback?) {

    /**
     *
     * MediaExtractor extractor = new MediaExtractor();
     * extractor.setDataSource(...);
     * int numTracks = extractor.getTrackCount();
     * for (int i = 0; i &lt; numTracks; ++i) {
     *   MediaFormat format = extractor.getTrackFormat(i);
     *   String mime = format.getString(MediaFormat.KEY_MIME);
     *   if (weAreInterestedInThisTrack) {
     *     extractor.selectTrack(i);
     *   }
     * }
     * ByteBuffer inputBuffer = ByteBuffer.allocate(...)
     * while (extractor.readSampleData(inputBuffer, ...) != 0) {
     *   int trackIndex = extractor.getSampleTrackIndex();
     *   long presentationTimeUs = extractor.getSampleTime();
     *   ...
     *   extractor.advance();
     * }
     * extractor.release();
     * extractor = null;
     */

/*
    // MediaCodec is typically used like this in asynchronous mode:
    MediaCodec codec = MediaCodec.createByCodecName(name);
    MediaFormat mOutputFormat;
    codec.setCallback(new MediaCodec.Callback() {
        @Override
        void onInputBufferAvailable(MediaCodec mc, int inputBufferId) {
            ByteBuffer inputBuffer = codec.getInputBuffer(inputBufferId);
            codec.queueInputBuffer(inputBufferId, …);
        }

        @Override
        void onOutputBufferAvailable(MediaCodec mc, int outputBufferId, …) {
            ByteBuffer outputBuffer = codec.getOutputBuffer(outputBufferId);
            MediaFormat bufferFormat = codec.getOutputFormat(outputBufferId);
            codec.releaseOutputBuffer(outputBufferId, …);
        }

        @Override
        void onOutputFormatChanged(MediaCodec mc, MediaFormat format) {
            mOutputFormat = format;
        }

        @Override
        void onError(…) {

        }
    });
    codec.configure(format, …);
    mOutputFormat = codec.getOutputFormat();
    codec.start();
    // wait for processing to complete
    codec.stop();
    codec.release();
*/

    private var videoDecodeThread: VideoDecodeThread? = null

    interface OnDataCallback {
        fun onData(duration: Long)
    }

    fun decode() {
        videoDecodeThread = VideoDecodeThread()
        videoDecodeThread!!.start()
    }

    fun seekTo(seekPosition: Long) {
        if (videoDecodeThread != null && !videoDecodeThread!!.isInterrupted) {
            videoDecodeThread!!.seekTo(seekPosition)
        }
    }

    fun setPreviewing(previewing: Boolean) {
        if (videoDecodeThread != null) {
            videoDecodeThread!!.setPreviewing(previewing)
        }
    }

    fun release() {
        if (videoDecodeThread != null && !videoDecodeThread!!.isInterrupted) {
            videoDecodeThread!!.interrupt()
            videoDecodeThread!!.release()
            videoDecodeThread = null
        }
    }

    private inner class VideoDecodeThread : Thread() {

        private var mediaExtractor: MediaExtractor? = null

        private var mediaCodec: MediaCodec? = null

        private var isPreviewing: Boolean = false

        fun setPreviewing(previewing: Boolean) {
            this.isPreviewing = previewing
        }

        fun seekTo(seekPosition: Long) {
            try {
                if (mediaExtractor != null) {
                    mediaExtractor!!.seekTo(seekPosition, MediaExtractor.SEEK_TO_CLOSEST_SYNC)
                }
            } catch (e: IllegalStateException) {
                Log.e(TAG, "seekTo error=$e")
            }

        }

        fun release() {
            try {
                if (mediaCodec != null) {
                    mediaCodec!!.stop()
                    mediaCodec!!.release()
                }
                if (mediaExtractor != null) {
                    mediaExtractor!!.release()
                }
            } catch (e: Exception) {
                Log.e(TAG, "release error=$e")
            }

        }

        /**
         * setting the preview resolution according to video aspect ratio
         *
         * @param mediaFormat mediaFormat
         */
        private fun setPreviewRatio(mediaFormat: MediaFormat?) {
            if (mediaFormat == null) {
                return
            }
            val videoWidth = mediaFormat.getInteger(MediaFormat.KEY_WIDTH)
            val videoHeight = mediaFormat.getInteger(MediaFormat.KEY_HEIGHT)
            val previewRatio: Int
            if (videoWidth >= RATIO_1080) {
                previewRatio = 10
            } else if (videoWidth >= RATIO_480) {
                previewRatio = 6
            } else if (videoWidth >= RATIO_240) {
                previewRatio = 4
            } else {
                previewRatio = 1
            }
            val previewWidth = videoWidth / previewRatio
            val previewHeight = videoHeight / previewRatio
            Log.e(TAG, "videoWidth=" + videoWidth + "--videoHeight=" + videoHeight
                    + "--previewWidth=" + previewWidth + "--previewHeight=" + previewHeight)
            mediaFormat.setInteger(MediaFormat.KEY_WIDTH, previewWidth)
            mediaFormat.setInteger(MediaFormat.KEY_HEIGHT, previewHeight)
        }

        override fun run() {
            super.run()

            mediaExtractor = MediaExtractor()
            var mediaFormat: MediaFormat? = null
            var mimeType: String? = ""
            try {
                mediaExtractor!!.setDataSource(mFilePath)
                for (i in 0 until mediaExtractor!!.trackCount) {
                    mediaFormat = mediaExtractor!!.getTrackFormat(i)
                    mimeType = mediaFormat!!.getString(MediaFormat.KEY_MIME)
                    if (mimeType != null && mimeType.startsWith("video/")) {
                        mediaExtractor!!.selectTrack(i)
                        break
                    }
                }
                if (mediaFormat == null || mimeType == null) {
                    return
                }
                val width = mediaFormat.getInteger(MediaFormat.KEY_WIDTH)
                val height = mediaFormat.getInteger(MediaFormat.KEY_HEIGHT)
                val duration = mediaFormat.getLong(MediaFormat.KEY_DURATION)
                mCallback?.onData(duration)
                Log.i(TAG, "width=$width--height=$height--duration==$duration")

                //setting preview resolution
                setPreviewRatio(mediaFormat)
                Log.i(TAG, "mediaFormat=$mediaFormat")

                //config MediaCodec, and start
                mediaCodec = MediaCodec.createDecoderByType(mimeType)
                mediaCodec!!.configure(mediaFormat, mSurface, null, 0)
                mediaCodec!!.start()
                val bufferInfo = MediaCodec.BufferInfo()
                val inputBuffers = mediaCodec!!.inputBuffers

                while (!isInterrupted) {
                    if (!isPreviewing) {
                        SystemClock.sleep(SLEEP_TIME.toLong())
                        continue
                    }
                    //dequeue from input buffer
                    val inputIndex = mediaCodec!!.dequeueInputBuffer(DEQUEUE_TIME)
                    if (inputIndex >= 0) {
                        val inputBuffer = inputBuffers[inputIndex]
                        val sampleSize = mediaExtractor!!.readSampleData(inputBuffer, 0)
                        //enqueue to input buffer
                        if (sampleSize < 0) {
                            mediaCodec!!.queueInputBuffer(inputIndex, 0, 0, 0, MediaCodec.BUFFER_FLAG_END_OF_STREAM)
                        } else {
                            mediaCodec!!.queueInputBuffer(inputIndex, 0, sampleSize, mediaExtractor!!.sampleTime, 0)
                            mediaExtractor!!.advance()
                        }
                    }

                    //dequeue from output buffer
                    val outputIndex = mediaCodec!!.dequeueOutputBuffer(bufferInfo, DEQUEUE_TIME)
                    when (outputIndex) {
                        MediaCodec.INFO_OUTPUT_FORMAT_CHANGED -> Log.i(TAG, "output format changed...")
                        MediaCodec.INFO_TRY_AGAIN_LATER -> Log.i(TAG, "try again later...")
                        MediaCodec.INFO_OUTPUT_BUFFERS_CHANGED -> Log.i(TAG, "output buffer changed...")
                        else ->
                            //render to surface
                            mediaCodec!!.releaseOutputBuffer(outputIndex, true)
                    }
                }
            } catch (e: Exception) {
                Log.e(TAG, "setDataSource error=$e")
            }

        }
    }

    companion object {

        private val TAG = MediaPlayController::class.java.simpleName

        private const val DEQUEUE_TIME = (10 * 1000).toLong()
        private const val SLEEP_TIME = 10

        private const val RATIO_1080 = 1080
        private const val RATIO_480 = 480
        private const val RATIO_240 = 240
    }

}