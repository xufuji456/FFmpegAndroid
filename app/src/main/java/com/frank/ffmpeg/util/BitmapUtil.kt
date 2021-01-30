package com.frank.ffmpeg.util

import android.graphics.Bitmap
import android.graphics.Canvas
import android.graphics.Paint
import android.text.TextUtils

import java.io.File
import java.io.FileOutputStream
import java.io.IOException

/**
 * bitmap tool
 * Created by frank on 2018/1/24.
 */

object BitmapUtil {

    /**
     * convert text to bitmap
     *
     * @param text text
     * @return bitmap of teh text
     */
    private fun textToBitmap(text: String, textColor: Int, textSize: Int): Bitmap? {
        if (TextUtils.isEmpty(text) || textSize <= 0) {
            return null
        }
        val paint = Paint()
        paint.textSize = textSize.toFloat()
        paint.textAlign = Paint.Align.LEFT
        paint.color = textColor
        paint.isDither = true
        paint.isAntiAlias = true
        val fm = paint.fontMetricsInt
        val width = paint.measureText(text).toInt()
        val height = fm.descent - fm.ascent

        val bitmap = Bitmap.createBitmap(width, height, Bitmap.Config.ARGB_8888)
        val canvas = Canvas(bitmap)
        canvas.drawText(text, 0f, (fm.leading - fm.ascent).toFloat(), paint)
        canvas.save()
        return bitmap
    }

    /**
     * convert text to picture
     *
     * @param filePath filePath
     * @param text     text
     * @return result of generating picture
     */
    fun textToPicture(filePath: String, text: String, textColor: Int, textSize: Int): Boolean {
        val bitmap = textToBitmap(text, textColor, textSize)
        if (bitmap == null || TextUtils.isEmpty(filePath)) {
            return false
        }
        var outputStream: FileOutputStream? = null
        try {
            outputStream = FileOutputStream(filePath)
            bitmap.compress(Bitmap.CompressFormat.PNG, 100, outputStream)
            outputStream.flush()
        } catch (e: IOException) {
            e.printStackTrace()
            return false
        } finally {
            try {
                outputStream?.close()
            } catch (e: IOException) {
                e.printStackTrace()
            }

        }
        return true
    }

    /**
     * delete file
     *
     * @param filePath filePath
     * @return result of deletion
     */
    fun deleteTextFile(filePath: String): Boolean {
        val file = File(filePath)
        return file.exists() && file.delete()
    }

}
