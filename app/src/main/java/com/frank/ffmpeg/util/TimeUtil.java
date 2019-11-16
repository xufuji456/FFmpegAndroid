package com.frank.ffmpeg.util;

import java.text.ParseException;
import java.text.SimpleDateFormat;
import java.util.Date;
import java.util.Locale;

/**
 * 时间转换工具类
 * Created by frank on 2018/11/12.
 */

public class TimeUtil {

    private static final String YMDHMS= "yyyy-MM-dd HH:mm:ss";

    /**
     * 时间戳年月日时分秒
     * @param time time
     * @return 年月日时分秒 yyyy/MM/dd HH:mm:ss
     */
    public static String getDetailTime(long time) {
        SimpleDateFormat format = new SimpleDateFormat(YMDHMS, Locale.getDefault());
        Date date = new Date(time);
        return format.format(date);
    }

    /**
     * 时间转为时间戳
     * @param time time
     * @return 时间戳
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

    private static String addZero(int time){
        if (time >= 0 && time < 10){
            return  "0" + time;
        }else if(time >= 10){
            return "" + time;
        }else {
            return "";
        }
    }

    /**
     * 获取视频时长
     * @param time time
     * @return 视频时长
     */
    public static String getVideoTime(long time){
        if (time <= 0)
            return null;
        time = time / 1000;
        int second, minute=0, hour=0;
        second = (int)time % 60;
        time = time / 60;
        if (time > 0){
            minute = (int)time % 60;
            hour = (int)time / 60;
        }
        if (hour > 0){
            return addZero(hour) + ":" + addZero(minute) + ":" + addZero(second);
        }else if (minute > 0){
            return addZero(minute) + ":" + addZero(second);
        }else {
            return "00:" + addZero(second);
        }
    }

}
