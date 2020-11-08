package com.frank.ffmpeg.gif

import android.graphics.BitmapFactory
import android.os.Environment
import android.text.TextUtils
import android.util.Log
import com.frank.ffmpeg.FFmpegCmd
import com.frank.ffmpeg.util.FFmpegUtil
import com.frank.ffmpeg.util.FileUtil
import java.io.ByteArrayOutputStream
import java.io.File
import java.io.FileOutputStream
import java.io.IOException

/**
 * @author frank
 * @date 2020-11-08 23:45
 * @desc Extract frames, and convert to GIF
 */
class HighQualityGif(width: Int, height: Int, rotateDegree: Int) {

    companion object {
        private val TAG = HighQualityGif::class.java.simpleName

        private const val TARGET_WIDTH = 320

        private const val TARGET_HEIGHT = 180
    }

    private var mWidth: Int = 0
    private var mHeight: Int = 0
    private var mRotateDegree: Int = 0

    init {
        this.mWidth = width
        this.mHeight = height
        this.mRotateDegree = rotateDegree
    }

    private fun chooseWidth(width: Int, height: Int): Int {
        if (width <= 0 || height <= 0) {
            return TARGET_WIDTH
        }
        return if (mRotateDegree == 0 || mRotateDegree == 180) {//landscape
            if (width > TARGET_WIDTH) {
                TARGET_WIDTH
            } else {
                width
            }
        } else {//portrait
            if (height > TARGET_HEIGHT) {
                TARGET_HEIGHT
            } else {
                height
            }
        }
    }

    @Throws(IllegalArgumentException::class)
    private fun generateGif(filePath: String, startTime: Int, duration: Int, frameRate: Int): ByteArray? {
        if (TextUtils.isEmpty(filePath)) {
            return null
        }
        val folderPath = Environment.getExternalStorageDirectory().toString() + "/gif_frames/"
        FileUtil.deleteFolder(folderPath)
        val targetWidth = chooseWidth(mWidth, mHeight)
        val commandLine = FFmpegUtil.videoToImageWithScale(filePath, startTime, duration, frameRate, targetWidth, folderPath)
        FFmpegCmd.executeSync(commandLine)
        val fileFolder = File(folderPath)
        if (!fileFolder.exists() || fileFolder.listFiles() == null) {
            return null
        }
        val files = fileFolder.listFiles()

        val outputStream = ByteArrayOutputStream()
        val gifEncoder = BeautyGifEncoder()
        gifEncoder.setFrameRate(10f)
        gifEncoder.start(outputStream)

        for (file in files) {
            val bitmap = BitmapFactory.decodeFile(file.absolutePath)
            if (bitmap != null) {
                gifEncoder.addFrame(bitmap)
            }
        }
        gifEncoder.finish()
        return outputStream.toByteArray()
    }

    private fun saveGif(data: ByteArray?, gifPath: String): Boolean {
        if (data == null || data.isEmpty() || TextUtils.isEmpty(gifPath)) {
            return false
        }
        var result = true
        var outputStream: FileOutputStream? = null
        try {
            outputStream = FileOutputStream(gifPath)
            outputStream.write(data)
            outputStream.flush()
        } catch (e: IOException) {
            result = false
        } finally {
            if (outputStream != null) {
                try {
                    outputStream.close()
                } catch (e: IOException) {
                    e.printStackTrace()
                }

            }
        }
        return result
    }

    /**
     * convert video into GIF
     * @param gifPath gifPath
     * @param filePath filePath
     * @param startTime where starts from the video(Unit: second)
     * @param duration how long you want to convert(Unit: second)
     * @param frameRate how much frames you want in a second
     * @return convert GIF success or not
     */
    fun convertGIF(gifPath: String, filePath: String, startTime: Int, duration: Int, frameRate: Int): Boolean {
        val data: ByteArray?
        try {
            data = generateGif(filePath, startTime, duration, frameRate)
        } catch (e: IllegalArgumentException) {
            Log.e(TAG, "generateGif error=$e")
            return false
        } catch (e: OutOfMemoryError) {
            Log.e(TAG, "generateGif error=$e")
            return false
        }

        return saveGif(data, gifPath)
    }
}