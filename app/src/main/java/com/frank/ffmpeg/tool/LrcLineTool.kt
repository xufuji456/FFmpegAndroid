package com.frank.ffmpeg.tool

import com.frank.ffmpeg.model.LrcLine
import com.frank.ffmpeg.util.TimeUtil

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
                val startTime = TimeUtil.timeStrToLong(temp)
                mLrcLine.startTime = startTime
                listTimes.add(mLrcLine)
            }
            return listTimes
        } catch (e: Exception) {
            e.printStackTrace()
            return null
        }

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
