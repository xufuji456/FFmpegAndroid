package com.frank.ffmpeg.util

import java.text.ParseException
import java.text.SimpleDateFormat
import java.util.Date
import java.util.Locale

/**
 * the tool of time transforming
 * Created by frank on 2018/11/12.
 */

object TimeUtil {

    private const val YMDHMS = "yyyy-MM-dd HH:mm:ss"

    /**
     * convert timestramp into String
     *
     * @param time time
     * @return yyyy/MM/dd HH:mm:ss
     */
    fun getDetailTime(time: Long): String {
        val format = SimpleDateFormat(YMDHMS, Locale.getDefault())
        val date = Date(time)
        return format.format(date)
    }

    /**
     * convert normal time into timestamp
     *
     * @param time time
     * @return timestamp
     */
    fun getLongTime(time: String, locale: Locale): Long {
        val simpleDateFormat = SimpleDateFormat(YMDHMS, locale)
        try {
            val dt = simpleDateFormat.parse(time)
            return dt.time
        } catch (e: ParseException) {
            e.printStackTrace()
        }

        return 0
    }

    private fun addZero(time: Int): String {
        return when {
            time in 0..9 -> "0$time"
            time >= 10 -> "" + time
            else -> ""
        }
    }

    /**
     * convert timestamp into video time
     *
     * @param t time
     * @return video time
     */
    fun getVideoTime(t: Long): String? {
        var time = t
        if (time <= 0)
            return null
        time /= 1000
        val second: Int
        var minute = 0
        var hour = 0
        second = time.toInt() % 60
        time /= 60
        if (time > 0) {
            minute = time.toInt() % 60
            hour = time.toInt() / 60
        }
        return when {
            hour > 0 -> addZero(hour) + ":" + addZero(minute) + ":" + addZero(second)
            minute > 0 -> addZero(minute) + ":" + addZero(second)
            else -> "00:" + addZero(second)
        }
    }

    /**
     * string time to milliseconds
     */
    fun timeStrToLong(timeStr: String): Long {
        var timeString = timeStr
        timeString = timeString.replace('.', ':')
        val times = timeString.split(":".toRegex()).dropLastWhile { it.isEmpty() }.toTypedArray()
        return (Integer.valueOf(times[0]) * 60 * 1000 +
                Integer.valueOf(times[1]) * 1000 +
                Integer.valueOf(times[2])).toLong()
    }
}
