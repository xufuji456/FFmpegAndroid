package com.frank.androidmedia.controller

import android.app.Activity
import android.content.Context
import android.content.Intent
import android.graphics.Bitmap
import android.graphics.PixelFormat
import android.hardware.display.DisplayManager
import android.hardware.display.VirtualDisplay
import android.media.ImageReader
import android.media.MediaCodec
import android.media.MediaCodecInfo
import android.media.MediaFormat
import android.media.projection.MediaProjection
import android.media.projection.MediaProjectionManager
import android.os.Environment
import android.util.DisplayMetrics
import android.util.Log
import android.view.Surface
import android.view.WindowManager
import com.frank.androidmedia.listener.VideoEncodeCallback
import java.io.FileOutputStream
import java.lang.Exception

/**
 * Using MediaProjectionManager to screenshot,
 * and using MediaProjection recording screen.
 *
 * @author frank
 * @date 2022/3/25
 */
open class MediaProjectionController(type: Int) {

    companion object {
        const val TYPE_SCREEN_SHOT   = 0
        const val TYPE_SCREEN_RECORD = 1
        const val TYPE_SCREEN_LIVING = 2
    }

    private var type = TYPE_SCREEN_SHOT
    private val requestCode = 123456
    private var virtualDisplay: VirtualDisplay? = null
    private var displayMetrics: DisplayMetrics? = null
    private var mediaProjection: MediaProjection? = null
    private var mediaProjectionManager: MediaProjectionManager? = null

    private var encodeThread: Thread? = null
    private var videoEncoder: MediaCodec? = null
    private var isVideoEncoding = false
    private var videoEncodeData: ByteArray? = null
    private var videoEncodeCallback: VideoEncodeCallback? = null

    init {
        this.type = type
    }

    fun setVideoEncodeListener(encodeCallback: VideoEncodeCallback) {
        videoEncodeCallback = encodeCallback
    }

    fun startScreenRecord(context: Context) {
        val windowManager = context.getSystemService(Context.WINDOW_SERVICE) as WindowManager
        displayMetrics = DisplayMetrics()
        windowManager.defaultDisplay.getMetrics(displayMetrics)
        mediaProjectionManager = context.getSystemService(Context.MEDIA_PROJECTION_SERVICE) as MediaProjectionManager
        val intent = mediaProjectionManager?.createScreenCaptureIntent()
        (context as Activity).startActivityForResult(intent, requestCode)
    }

    fun createVirtualDisplay(surface: Surface) {
        virtualDisplay = mediaProjection?.createVirtualDisplay("hello", displayMetrics!!.widthPixels,
                displayMetrics!!.heightPixels, displayMetrics!!.densityDpi,
                DisplayManager.VIRTUAL_DISPLAY_FLAG_AUTO_MIRROR,
                surface, null, null)
    }

    private fun saveBitmap(bitmap: Bitmap?, path: String) {
        if (path.isEmpty() || bitmap == null)
            return
        var outputStream: FileOutputStream? = null
        try {
            outputStream = FileOutputStream(path)
            bitmap.compress(Bitmap.CompressFormat.JPEG, 100, outputStream)
        } catch (e: Exception) {

        } finally {
            outputStream?.close()
        }
    }

    private fun getBitmap() {
        val imageReader = ImageReader.newInstance(displayMetrics!!.widthPixels,
                displayMetrics!!.heightPixels, PixelFormat.RGBA_8888, 3)
        createVirtualDisplay(imageReader.surface)
        imageReader.setOnImageAvailableListener ({ reader: ImageReader ->

            val image = reader.acquireNextImage()
            val planes = image.planes
            val buffer = planes[0].buffer
            val pixelStride = planes[0].pixelStride
            val rowStride = planes[0].rowStride
            val rowPadding = rowStride - pixelStride * image.width
            val bitmap = Bitmap.createBitmap(image.width + rowPadding / pixelStride,
                    image.height, Bitmap.Config.ARGB_8888)
            bitmap.copyPixelsFromBuffer(buffer)
            val filePath = Environment.getExternalStorageDirectory().path + "/hello.jpg"
            saveBitmap(bitmap, filePath)
            image.close()
            imageReader.close()
        }, null)
    }

    private fun initMediaCodec(width: Int, height: Int) {
        val mediaFormat = MediaFormat.createVideoFormat(MediaFormat.MIMETYPE_VIDEO_AVC, width, height)
        mediaFormat.setInteger(MediaFormat.KEY_WIDTH, width)
        mediaFormat.setInteger(MediaFormat.KEY_HEIGHT, height)
        mediaFormat.setInteger(MediaFormat.KEY_FRAME_RATE, 20)
        mediaFormat.setInteger(MediaFormat.KEY_BIT_RATE, width * height)
        mediaFormat.setInteger(MediaFormat.KEY_I_FRAME_INTERVAL, 3)
        mediaFormat.setInteger(MediaFormat.KEY_COLOR_FORMAT, MediaCodecInfo.CodecCapabilities.COLOR_FormatSurface)

        videoEncoder = MediaCodec.createEncoderByType(MediaFormat.MIMETYPE_VIDEO_AVC)
        videoEncoder?.configure(mediaFormat, null, null, MediaCodec.CONFIGURE_FLAG_ENCODE)
        createVirtualDisplay(videoEncoder!!.createInputSurface())
    }

    private fun startVideoEncoder() {
        if (videoEncoder == null || isVideoEncoding)
            return
        encodeThread = Thread {
            try {
                val bufferInfo = MediaCodec.BufferInfo()
                videoEncoder?.start()

                while (isVideoEncoding && !Thread.currentThread().isInterrupted) {
                    val outputIndex = videoEncoder!!.dequeueOutputBuffer(bufferInfo, 30 * 1000)
                    if (outputIndex >= 0) {
                        val byteBuffer = videoEncoder!!.getOutputBuffer(outputIndex)
                        if (videoEncodeData == null || videoEncodeData!!.size < bufferInfo.size) {
                            videoEncodeData = ByteArray(bufferInfo.size)
                        }
                        if (videoEncodeCallback != null && byteBuffer != null) {
                            byteBuffer.get(videoEncodeData, bufferInfo.offset, bufferInfo.size)
                            videoEncodeCallback!!.onVideoEncodeData(videoEncodeData!!, bufferInfo.size,
                                    bufferInfo.flags,  bufferInfo.presentationTimeUs)
                        }
                        videoEncoder!!.releaseOutputBuffer(outputIndex, false)
                    } else {
                        Log.e("EncodeThread", "invalid index=$outputIndex")
                    }
                }
            } catch (e: Exception) {
                isVideoEncoding = false
                Log.e("EncodeThread", "encode error=$e")
            }
        }
        isVideoEncoding = true
        encodeThread?.start()
    }

    fun onActivityResult(resultCode: Int, data: Intent) {
        mediaProjection = mediaProjectionManager?.getMediaProjection(resultCode, data)
        if (type == TYPE_SCREEN_SHOT) {
            getBitmap()
        } else if (type == TYPE_SCREEN_LIVING) {
            initMediaCodec(displayMetrics!!.widthPixels, displayMetrics!!.heightPixels)
            startVideoEncoder()
        }
    }

    fun getRequestCode(): Int {
        return requestCode
    }


    fun stopScreenRecord() {
        mediaProjection?.stop()
        virtualDisplay?.release()
        isVideoEncoding = false
        encodeThread?.interrupt()
        videoEncoder?.release()
    }

}