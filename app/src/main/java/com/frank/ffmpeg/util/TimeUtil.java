package com.frank.ffmpeg.util;

import java.text.ParseException;
import java.text.SimpleDateFormat;
import java.util.Date;
import java.util.Locale;

/**
 * the tool of time transforming
 * Created by frank on 2018/11/12.
 */

public class TimeUtil {

    private static final String YMDHMS = "yyyy-MM-dd HH:mm:ss";

    /**
     * convert timestramp into String
     *
     * @param time time
     * @return yyyy/MM/dd HH:mm:ss
     */
    public static String getDetailTime(long time) {
        SimpleDateFormat format = new SimpleDateFormat(YMDHMS, Locale.getDefault());
        Date date = new Date(time);
        return format.format(date);
    }

    /**
     * convert normal time into timestamp
     *
     * @param time time
     * @return timestamp
     */
    public static long getLongTime(String time, Locale locale) {
        SimpleDateFormat simpleDateFormat = new SimpleDateFormat(YMDHMS, locale);
        try {
            Date dt = simpleDateFormat.parse(time);
            return dt.getTime();
        } catch (ParseException e) {
            e.printStackTrace();
        }
        return 0;
    }

    private static String addZero(int time) {
        if (time >= 0 && time < 10) {
            return "0" + time;
        } else if (time >= 10) {
            return "" + time;
        } else {
            return "";
        }
    }

    /**
     * convert timestamp into video time
     *
     * @param time time
     * @return video time
     */
    public static String getVideoTime(long time) {
        if (time <= 0)
            return null;
        time = time / 1000;
        int second, minute = 0, hour = 0;
        second = (int) time % 60;
        time = time / 60;
        if (time > 0) {
            minute = (int) time % 60;
            hour = (int) time / 60;
        }
        if (hour > 0) {
            return addZero(hour) + ":" + addZero(minute) + ":" + addZero(second);
        } else if (minute > 0) {
            return addZero(minute) + ":" + addZero(second);
        } else {
            return "00:" + addZero(second);
        }
    }

}
