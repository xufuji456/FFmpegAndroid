package com.frank.ffmpeg.tool

import com.frank.ffmpeg.model.LrcInfo
import com.frank.ffmpeg.model.LrcLine
import com.frank.ffmpeg.util.TimeUtil

import java.io.BufferedInputStream
import java.io.BufferedReader
import java.io.FileInputStream
import java.io.IOException
import java.io.InputStream
import java.io.InputStreamReader
import java.util.*
import java.util.regex.Pattern

/**
 * Parse lrc format of lyrics
 */
class LrcParser {

    private val lrcInfo = LrcInfo()

    private val lrcLineList = ArrayList<LrcLine>()

    fun readLrc(path: String): LrcInfo? {
        var inputStream: InputStream? = null
        var inputStreamReader: InputStreamReader? = null
        try {
            var charsetName: String? = getCharsetName(path)
            inputStream = FileInputStream(path)
            if (charsetName!!.toLowerCase(Locale.getDefault()) == "utf-8") {
                inputStream = UnicodeInputStream(inputStream, charsetName)
                charsetName = inputStream.getEncoding()
            }
            inputStreamReader = if (!charsetName.isNullOrEmpty()) {
                InputStreamReader(inputStream, charsetName)
            } else {
                InputStreamReader(inputStream)
            }
            val reader = BufferedReader(inputStreamReader)
            while (true) {
                val str: String? = reader.readLine() ?: break
                if (!str.isNullOrEmpty()) {
                    decodeLine(str)
                }
            }
            lrcInfo.lrcLineList = lrcLineList
            return lrcInfo
        } catch (e: IOException) {
            return null
        } finally {
            try {
                inputStreamReader?.close()
                inputStream?.close()
            } catch (e: IOException) {
                e.printStackTrace()
            }

        }
    }

    private fun getCharsetName(path: String): String {
        var code = "GBK"
        var bin: BufferedInputStream? = null
        try {
            bin = BufferedInputStream(FileInputStream(path))
            when ((bin.read() shl 8) + bin.read()) {
                0xefbb -> code = "UTF-8"
                0xfffe -> code = "Unicode"
                0xfeff -> code = "UTF-16BE"
                else -> {
                }
            }
        } catch (e: Exception) {
            e.printStackTrace()
        } finally {
            if (bin != null) {
                try {
                    bin.close()
                } catch (e: IOException) {
                    e.printStackTrace()
                }

            }
        }
        return code
    }

    private fun match(line: String): Boolean {
        return line.length > 4 && line.lastIndexOf("]") > 4
    }

    private fun decodeLine(str: String) {
        if (str.startsWith("[ti:")) {
            if (match(str))
                lrcInfo.title = str.substring(4, str.lastIndexOf("]"))
        } else if (str.startsWith("[ar:")) {
            if (match(str))
                lrcInfo.artist = str.substring(4, str.lastIndexOf("]"))
        } else if (str.startsWith("[al:")) {
            if (match(str))
                lrcInfo.album = str.substring(4, str.lastIndexOf("]"))
        } else if (str.startsWith("[au:")) {
            if (match(str))
                lrcInfo.author = str.substring(4, str.lastIndexOf("]"))
        } else if (str.startsWith("[by:")) {
            if (match(str))
                lrcInfo.creator = str.substring(4, str.lastIndexOf("]"))
        } else if (str.startsWith("[re:")) {
            if (match(str))
                lrcInfo.encoder = str.substring(4, str.lastIndexOf("]"))
        } else if (str.startsWith("[ve:")) {
            if (match(str))
                lrcInfo.version = str.substring(4, str.lastIndexOf("]"))
        } else if (str.startsWith("[offset:")) {
            if (str.lastIndexOf("]") > 8) {
                val offset = str.substring(8, str.lastIndexOf("]"))
                try {
                    lrcInfo.offset = Integer.parseInt(offset)
                } catch (e: NumberFormatException) {
                    e.printStackTrace()
                }

            }
        } else {
            val timeExpress = "\\[(\\d{1,2}:\\d{1,2}\\.\\d{1,2})]|\\[(\\d{1,2}:\\d{1,2})]"
            val pattern = Pattern.compile(timeExpress)
            val matcher = pattern.matcher(str)
            var currentTime: Long = 0
            while (matcher.find()) {
                val groupCount = matcher.groupCount()
                var currentTimeStr: String? = ""
                for (index in 0 until groupCount) {
                    val timeStr = matcher.group(index)
                    if (index == 0) {
                        currentTimeStr = timeStr.substring(1, timeStr.length - 1)
                        currentTime = TimeUtil.timeStrToLong(currentTimeStr)
                    }
                }
                val content = pattern.split(str)
                var currentContent: String? = ""
                if (content.isNotEmpty()) {
                    currentContent = content[content.size - 1]
                }
                val lrcLine = LrcLine()
                lrcLine.timeString = currentTimeStr
                lrcLine.startTime = currentTime
                lrcLine.endTime = currentTime + 10 * 1000
                lrcLine.content = currentContent
                lrcLineList.add(lrcLine)
            }
        }
    }

}
