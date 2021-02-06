package com.frank.mp3

import android.annotation.TargetApi
import android.media.MediaCodec
import android.media.MediaCodec.BufferInfo
import android.media.MediaExtractor
import android.media.MediaFormat
import android.os.Build
import android.util.Log

import java.io.BufferedOutputStream
import java.io.File
import java.io.FileOutputStream
import java.io.IOException
import java.nio.ByteBuffer
import java.nio.ByteOrder
import java.util.concurrent.LinkedBlockingDeque

@TargetApi(Build.VERSION_CODES.JELLY_BEAN)
open class Mp3Converter {

    private var mMediaCodec: MediaCodec? = null
    private var mediaExtractor: MediaExtractor? = null
    private var bufferInfo: BufferInfo? = null
    private var rawInputBuffers: Array<ByteBuffer>? = null
    private var encodedOutputBuffers: Array<ByteBuffer>? = null
    private var inSampleRate: Int = 0
    private var channels: Int = 0
    private val writeQueue = LinkedBlockingDeque<BufferEncoded>(DEFAULT_QUEUE_SIZE)
    private val encodeQueue = LinkedBlockingDeque<BufferDecoded>(DEFAULT_QUEUE_SIZE)
    private var mp3buf: ByteArray? = null

    private var decodeFinished: Boolean = false
    private var encodeFinished: Boolean = false

    private var readSize: Long = 0
    private var decodeSize: Long = 0
    private var encodeSize: Long = 0

    private var mp3Lame: Mp3Lame? = null
    private var writeThread: WriteThread? = null
    private var encodeThread: EncodeThread? = null
    private var lastPts: Long = 0

    private inner class BufferDecoded {
        internal var channels: Int = 0
        internal var leftBuffer: ShortArray? = null
        internal var rightBuffer: ShortArray? = null
        internal var pcm: ShortArray? = null
        internal var pts: Long = 0
    }

    private inner class BufferEncoded {
        internal var buffer: ByteArray? = null
        internal var length: Int = 0
    }

    private inner class WriteThread internal constructor(private val mp3Path: String) : Thread() {
        private var fos: FileOutputStream? = null
        private var bos: BufferedOutputStream? = null

        override fun run() {
            try {
                Log.i(TAG, "WriteThread start")

                fos = FileOutputStream(File(mp3Path))
                bos = BufferedOutputStream(fos, 200 * 1024)

                while (true) {
                    if (encodeFinished && writeQueue.size == 0) {
                        break
                    }

                    var buffer: BufferEncoded? = null
                    try {
                        buffer = writeQueue.take()
                    } catch (e: InterruptedException) {
                        Log.e(TAG, "WriteThread InterruptedException=$e")
                    }

                    if (buffer != null) {
                        bos!!.write(buffer.buffer, 0, buffer.length)
                    }
                }
                bos!!.flush()
                bos!!.close()
            } catch (e: Exception) {
                e.printStackTrace()
            } finally {
                try {
                    bos!!.flush()
                    bos!!.close()
                } catch (e: IOException) {
                    e.printStackTrace()
                }

            }

            Log.i(TAG, "WriteThread end")
        }
    }

    private inner class EncodeThread : Thread() {
        override fun run() {
            Log.i(TAG, "EncodeThread start")

            while (true) {
                if (decodeFinished && encodeQueue.size == 0) {
                    break
                }
                var buffer: BufferDecoded? = null
                try {
                    buffer = encodeQueue.take()
                } catch (e: InterruptedException) {
                    Log.e(TAG, "EncodeThread InterruptedException=$e")
                }

                if (buffer != null) {
                    encodeToMp3(buffer)
                }
            }
            encodeFinished = true

            writeThread!!.interrupt()

            Log.i(TAG, "EncodeThread end")
        }
    }

    private inner class DecodeThread : Thread() {
        override fun run() {
            val startTime = System.currentTimeMillis()
            try {
                Log.i(TAG, "DecodeThread start")

                while (true) {
                    val outputBufIndex = mMediaCodec!!.dequeueOutputBuffer(bufferInfo!!, -1)
                    if (outputBufIndex >= 0) {
                        val buffer = encodedOutputBuffers!![outputBufIndex]
                        decodeSize += bufferInfo!!.size.toLong()
                        val shortBuffer = buffer.order(ByteOrder.nativeOrder()).asShortBuffer()

                        var leftBuffer: ShortArray? = null
                        var rightBuffer: ShortArray? = null
                        var pcm: ShortArray? = null

                        if (channels == 2) {
                            pcm = ShortArray(shortBuffer.remaining())
                            shortBuffer.get(pcm)
                        } else {
                            leftBuffer = ShortArray(shortBuffer.remaining())
                            rightBuffer = leftBuffer
                            shortBuffer.get(leftBuffer)
                            Log.e(TAG, "single channel leftBuffer.length = " + leftBuffer.size)
                        }

                        buffer.clear()

                        val bufferDecoded = BufferDecoded()
                        bufferDecoded.leftBuffer = leftBuffer
                        bufferDecoded.rightBuffer = rightBuffer
                        bufferDecoded.pcm = pcm
                        bufferDecoded.channels = channels
                        bufferDecoded.pts = bufferInfo!!.presentationTimeUs
                        encodeQueue.put(bufferDecoded)

                        mMediaCodec!!.releaseOutputBuffer(outputBufIndex, false)

                        if (bufferInfo!!.flags and MediaCodec.BUFFER_FLAG_END_OF_STREAM != 0) {
                            Log.e(TAG, "DecodeThread get BUFFER_FLAG_END_OF_STREAM")
                            decodeFinished = true
                            break
                        }
                    } else if (outputBufIndex == MediaCodec.INFO_OUTPUT_BUFFERS_CHANGED) {
                        encodedOutputBuffers = mMediaCodec!!.outputBuffers
                    } else if (outputBufIndex == MediaCodec.INFO_OUTPUT_FORMAT_CHANGED) {
                        val oformat = mMediaCodec!!.outputFormat
                        Log.d(TAG, "Output format has changed to $oformat")
                    }
                }
            } catch (e: Exception) {
                e.printStackTrace()
            }

            encodeThread!!.interrupt()
            val endTime = System.currentTimeMillis()
            Log.i(TAG, "DecodeThread finished time=" + (endTime - startTime) / 1000)
        }
    }

    fun convertToMp3(srcPath: String, mp3Path: String) {

        val startTime = System.currentTimeMillis()
        val endTime: Long

        encodeThread = EncodeThread()
        writeThread = WriteThread(mp3Path)
        val decodeThread = DecodeThread()

        encodeThread!!.start()
        writeThread!!.start()
        prepareDecode(srcPath)
        decodeThread.start()
        readSampleData()

        try {
            writeThread!!.join()
        } catch (e: InterruptedException) {
            Log.e(TAG, "convertToMp3 InterruptedException=$e")
        }

        val mReadSize = readSize.toDouble() / 1024.0 / 1024.0
        val mDecodeSize = decodeSize.toDouble() / 1024.0 / 1024.0
        val mEncodeSize = encodeSize.toDouble() / 1024.0 / 1024.0
        Log.i(TAG, "readSize=$mReadSize, decodeSize=$mDecodeSize,encodeSize=$mEncodeSize")

        endTime = System.currentTimeMillis()
        Log.i(TAG, "convertToMp3 finish time=" + (endTime - startTime) / 1000)
    }

    private fun prepareDecode(path: String) {
        try {
            mediaExtractor = MediaExtractor()
            mediaExtractor!!.setDataSource(path)
            for (i in 0 until mediaExtractor!!.trackCount) {
                val mMediaFormat = mediaExtractor!!.getTrackFormat(i)
                Log.i(TAG, "prepareDecode get mMediaFormat=$mMediaFormat")

                val mime = mMediaFormat.getString(MediaFormat.KEY_MIME)
                if (mime.startsWith("audio")) {
                    mMediaCodec = MediaCodec.createDecoderByType(mime)
                    mMediaCodec!!.configure(mMediaFormat, null, null, 0)
                    mediaExtractor!!.selectTrack(i)
                    inSampleRate = mMediaFormat.getInteger(MediaFormat.KEY_SAMPLE_RATE)
                    channels = mMediaFormat.getInteger(MediaFormat.KEY_CHANNEL_COUNT)
                    break
                }
            }
            mMediaCodec!!.start()

            bufferInfo = BufferInfo()
            rawInputBuffers = mMediaCodec!!.inputBuffers
            encodedOutputBuffers = mMediaCodec!!.outputBuffers
            Log.i(TAG, "--channel=$channels--sampleRate=$inSampleRate")

            mp3Lame = Mp3LameBuilder()
                    .setInSampleRate(inSampleRate)
                    .setOutChannels(channels)
                    .setOutBitrate(128)
                    .setOutSampleRate(inSampleRate)
                    .setQuality(9)
                    .setVbrMode(Mp3LameBuilder.VbrMode.VBR_MTRH)
                    .build()

        } catch (e: IOException) {
            e.printStackTrace()
        }

    }

    private fun readSampleData() {
        var rawInputEOS = false

        while (!rawInputEOS) {
            for (i in rawInputBuffers!!.indices) {
                val inputBufIndex = mMediaCodec!!.dequeueInputBuffer(-1)
                if (inputBufIndex >= 0) {
                    val buffer = rawInputBuffers!![inputBufIndex]
                    var sampleSize = mediaExtractor!!.readSampleData(buffer, 0)
                    var presentationTimeUs: Long = 0
                    if (sampleSize < 0) {
                        rawInputEOS = true
                        sampleSize = 0
                    } else {
                        readSize += sampleSize.toLong()
                        presentationTimeUs = mediaExtractor!!.sampleTime
                    }
                    mMediaCodec!!.queueInputBuffer(inputBufIndex, 0,
                            sampleSize, presentationTimeUs,
                            if (rawInputEOS) MediaCodec.BUFFER_FLAG_END_OF_STREAM else 0)
                    if (!rawInputEOS) {
                        mediaExtractor!!.advance()
                    } else {
                        break
                    }
                } else {
                    Log.e(TAG, "wrong inputBufIndex=$inputBufIndex")
                }
            }
        }
    }

    private fun encodeToMp3(buffer: BufferDecoded?) {

        if (buffer == null || buffer.pts == lastPts) {
            return
        }
        lastPts = buffer.pts

        val bufferLength = buffer.pcm!!.size / 2
        if (mp3buf == null) {
            mp3buf = ByteArray((bufferLength * 1.25 + 7200).toInt())
        }
        if (bufferLength > 0) {
            val bytesEncoded: Int
            if (channels == 2) {
                bytesEncoded = mp3Lame!!.encodeBufferInterLeaved(buffer.pcm!!, bufferLength, mp3buf!!)
            } else {
                bytesEncoded = mp3Lame!!.encode(buffer.leftBuffer!!, buffer.leftBuffer!!, bufferLength, mp3buf!!)
            }
            Log.d(TAG, "mp3Lame encodeSize=$bytesEncoded")

            if (bytesEncoded > 0) {
                val be = BufferEncoded()
                be.buffer = mp3buf
                be.length = bytesEncoded
                try {
                    writeQueue.put(be)
                } catch (e: InterruptedException) {
                    e.printStackTrace()
                }

                encodeSize += bytesEncoded.toLong()
            }
        }
    }

    companion object {

        private val TAG = Mp3Converter::class.java.simpleName

        private const val DEFAULT_QUEUE_SIZE = 512
    }

}
