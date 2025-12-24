package com.frank.ffmpeg.util

import android.text.TextUtils
import android.util.Log

import java.io.File
import java.io.FileOutputStream
import java.io.IOException
import java.util.Locale.getDefault

/**
 * file tool
 * Created by frank on 2018/5/9.
 */

object FileUtil {

    private const val TYPE_MP3 = "mp3"
    private const val TYPE_AAC = "aac"
    private const val TYPE_AMR = "amr"
    private const val TYPE_FLAC = "flac"
    private const val TYPE_M4A = "m4a"
    private const val TYPE_WMA = "wma"
    private const val TYPE_WAV = "wav"
    private const val TYPE_OGG = "ogg"
    private const val TYPE_AC3 = "ac3"
    private const val TYPE_RAW = "pcm"
    private const val TYPE_OPUS = "opus"

    private const val TYPE_MP4 = "mp4"
    private const val TYPE_MKV = "mkv"
    private const val TYPE_WEBM = "webm"
    private const val TYPE_AVI = "avi"
    private const val TYPE_WMV = "wmv"
    private const val TYPE_FLV = "flv"
    private const val TYPE_TS = "ts"
    private const val TYPE_M3U8 = "m3u8"
    private const val TYPE_3GP = "3gp"
    private const val TYPE_MOV = "mov"
    private const val TYPE_MPG = "mpg"

    /**
     * check the file exist or not
     *
     * @param path the path of file
     * @return result of exist or not
     */
    fun checkFileExist(path: String): Boolean {
        if (TextUtils.isEmpty(path)) {
            return false
        }
        val file = File(path)
        if (!file.exists()) {
            Log.e("FileUtil", "$path is not exist!")
            return false
        }
        return true
    }

    fun isAudio(filePath: String): Boolean {
        var path = filePath
        if (TextUtils.isEmpty(path)) {
            return false
        }
        path = path.lowercase(getDefault())
        return (path.endsWith(TYPE_MP3)
                || path.endsWith(TYPE_AAC)
                || path.endsWith(TYPE_AMR)
                || path.endsWith(TYPE_FLAC)
                || path.endsWith(TYPE_M4A)
                || path.endsWith(TYPE_WMA)
                || path.endsWith(TYPE_WAV)
                || path.endsWith(TYPE_OGG)
                || path.endsWith(TYPE_AC3)
                || path.endsWith(TYPE_RAW)
                || path.endsWith(TYPE_OPUS))
    }

    fun isVideo(filePath: String): Boolean {
        var path = filePath
        if (TextUtils.isEmpty(path)) {
            return false
        }
        path = path.lowercase(getDefault())
        return (path.endsWith(TYPE_MP4)
                || path.endsWith(TYPE_MKV)
                || path.endsWith(TYPE_WEBM)
                || path.endsWith(TYPE_WMV)
                || path.endsWith(TYPE_AVI)
                || path.endsWith(TYPE_FLV)
                || path.endsWith(TYPE_3GP)
                || path.endsWith(TYPE_TS)
                || path.endsWith(TYPE_M3U8)
                || path.endsWith(TYPE_MOV)
                || path.endsWith(TYPE_MPG))
    }

    fun getFileSuffix(fileName: String): String? {
        return if (TextUtils.isEmpty(fileName) || !fileName.contains(".")) {
            null
        } else fileName.substring(fileName.lastIndexOf("."))
    }

    fun createListFile(listPath: String, fileList: List<String>?): String? {
        if (TextUtils.isEmpty(listPath) || fileList == null || fileList.isEmpty()) {
            return null
        }
        var outputStream: FileOutputStream? = null
        try {
            val listFile = File(listPath)
            if (listFile.parentFile?.exists() == false) {
                if (!listFile.mkdirs()) {
                    return null
                }
            }
            if (!listFile.exists()) {
                if (!listFile.createNewFile()) {
                    return null
                }
            }
            outputStream = FileOutputStream(listFile)
            val fileBuilder = StringBuilder()
            for (file in fileList) {
                fileBuilder
                        .append("file")
                        .append(" ")
                        .append("'")
                        .append(file)
                        .append("'")
                        .append("\n")
            }
            val fileData = fileBuilder.toString().toByteArray()
            outputStream.write(fileData, 0, fileData.size)
            outputStream.flush()
            return listFile.absolutePath
        } catch (e: IOException) {
            e.printStackTrace()
        } finally {
            if (outputStream != null) {
                try {
                    outputStream.close()
                } catch (e: IOException) {
                    e.printStackTrace()
                }

            }
        }
        return null
    }

    fun deleteFile(path: String): Boolean {
        if (TextUtils.isEmpty(path)) {
            return false
        }
        val file = File(path)
        return file.exists() && file.delete()
    }

    fun deleteFolder(path: String): Boolean {
        if (TextUtils.isEmpty(path)) {
            return false
        }
        var result = true
        val tempFile = File(path)
        if (!tempFile.exists()) {
            return tempFile.mkdir()
        }
        if (tempFile.isDirectory && tempFile.listFiles() != null) {
            val files = tempFile.listFiles() ?: return false
            for (file in files) {
                result = result and file.delete()
            }
        }
        return result
    }

}
