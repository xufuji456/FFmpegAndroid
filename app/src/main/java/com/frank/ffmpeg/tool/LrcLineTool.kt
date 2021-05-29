package com.frank.ffmpeg.tool

import com.frank.ffmpeg.model.LrcLine

import java.util.ArrayList
import java.util.Collections

object LrcLineTool {

    private fun createLine(lrcLine: String?): List<LrcLine>? {
        try {
            if (lrcLine == null || lrcLine.isEmpty() ||
                    lrcLine.indexOf("[") != 0 || lrcLine.indexOf("]") != 9) {
                return null
            }

            val lastIndexOfRightBracket = lrcLine.lastIndexOf("]")
            val content = lrcLine.substring(lastIndexOfRightBracket + 1)
            val times = lrcLine.substring(0, lastIndexOfRightBracket + 1).replace("[", "-")
                    .replace("]", "-")
            if (times.isEmpty() || !Character.isDigit(times[1])) return null
            val arrTimes = times.split("-".toRegex()).dropLastWhile { it.isEmpty() }.toTypedArray()
            val listTimes = ArrayList<LrcLine>()
            for (temp in arrTimes) {
                if (temp.trim { it <= ' ' }.isEmpty()) {
                    continue
                }
                val mLrcLine = LrcLine()
                mLrcLine.content = content
                mLrcLine.timeString = temp
                val startTime = timeConvert(temp)
                mLrcLine.startTime = startTime
                listTimes.add(mLrcLine)
            }
            return listTimes
        } catch (e: Exception) {
            e.printStackTrace()
            return null
        }

    }

    /**
     * string time to milliseconds
     */
    private fun timeConvert(timeStr: String): Long {
        var timeString = timeStr
        timeString = timeString.replace('.', ':')
        val times = timeString.split(":".toRegex()).dropLastWhile { it.isEmpty() }.toTypedArray()
        return (Integer.valueOf(times[0]) * 60 * 1000 +
                Integer.valueOf(times[1]) * 1000 +
                Integer.valueOf(times[2])).toLong()
    }

    fun getLrcLine(line: String?): List<LrcLine>? {
        if (line == null || line.isEmpty()) {
            return null
        }
        val rows = ArrayList<LrcLine>()
        try {
            val lrcLines = createLine(line)
            if (lrcLines != null && lrcLines.isNotEmpty()) {
                rows.addAll(lrcLines)
            }
        } catch (e: Exception) {
            return null
        }

        return rows
    }

    fun sortLyrics(lrcList: List<LrcLine>): List<LrcLine> {
        Collections.sort(lrcList)
        if (lrcList.isNotEmpty()) {
            val size = lrcList.size
            for (i in 0 until size) {
                val lrcLine = lrcList[i]
                if (i < size - 1) {
                    lrcLine.endTime = lrcList[i + 1].startTime
                } else {
                    lrcLine.endTime = lrcLine.startTime + 10 * 1000
                }
            }
        }
        return lrcList
    }

}
