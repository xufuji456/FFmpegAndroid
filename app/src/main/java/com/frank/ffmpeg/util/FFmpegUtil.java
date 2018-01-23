package com.frank.ffmpeg.util;

import android.annotation.SuppressLint;

/**
 * ffmpeg工具：拼接命令行处理音频
 * Created by frank on 2018/1/23.
 */

public class FFmpegUtil {

    /**
     * 使用ffmpeg命令行进行音频转码
     * @param srcFile 源文件
     * @param targetFile 目标文件（后缀指定转码格式）
     * @return 转码后的文件
     */
    public static String[] transformAudio(String srcFile, String targetFile){
        String transformAudioCmd = "ffmpeg -i %s %s";
        transformAudioCmd = String.format(transformAudioCmd, srcFile, targetFile);
        return transformAudioCmd.split(" ");//以空格分割为字符串数组
    }

    /**
     * 使用ffmpeg命令行进行音频剪切
     * @param srcFile 源文件
     * @param startTime 剪切的开始时间(单位为秒)
     * @param duration 剪切时长(单位为秒)
     * @param targetFile 目标文件
     * @return 剪切后的文件
     */
    @SuppressLint("DefaultLocale")
    public static  String[] cutAudio(String srcFile, int startTime, int duration, String targetFile){
        String cutAudioCmd = "ffmpeg -i %s -ss %d -t %d %s";
        cutAudioCmd = String.format(cutAudioCmd, srcFile, startTime, duration, targetFile);
        return cutAudioCmd.split(" ");//以空格分割为字符串数组
    }

    /**
     * 使用ffmpeg命令行进行音频合并
     * @param srcFile 源文件
     * @param appendFile 待追加的文件
     * @param targetFile 目标文件
     * @return 合并后的文件
     */
    public static  String[] concatAudio(String srcFile, String appendFile, String targetFile){
        String concatAudioCmd = "ffmpeg -i concat:%s|%s -acodec copy %s";
        concatAudioCmd = String.format(concatAudioCmd, srcFile, appendFile, targetFile);
        return concatAudioCmd.split(" ");//以空格分割为字符串数组
    }

    /**
     * 使用ffmpeg命令行进行音频混合
     * @param srcFile 源文件
     * @param mixFile 待混合文件
     * @param targetFile 目标文件
     * @return 混合后的文件
     */
    public static  String[] mixAudio(String srcFile, String mixFile, String targetFile){
        String mixAudioCmd = "ffmpeg -i %s -i %s -filter_complex amix=inputs=2:duration=first -strict -2 %s";
        mixAudioCmd = String.format(mixAudioCmd, srcFile, mixFile, targetFile);
        return mixAudioCmd.split(" ");//以空格分割为字符串数组
    }
    //混音公式：value = sample1 + sample2 - (sample1 * sample2 / (pow(2, 16-1) - 1))


    /**
     * 使用ffmpeg命令行进行音视频合成
     * @param videoFile 视频文件
     * @param audioFile 音频文件
     * @param duration 视频时长
     * @param muxFile 目标文件
     * @return 合成后的文件
     */
    @SuppressLint("DefaultLocale")
    public static  String[] mediaMux(String videoFile, String audioFile, int duration, String muxFile){
        //-t:时长  如果忽略音视频时长，则把"-t %d"去掉
        String mixAudioCmd = "ffmpeg -i %s -i %s -t %d -acodec copy %s";
        mixAudioCmd = String.format(mixAudioCmd, videoFile, audioFile, duration, muxFile);
        return mixAudioCmd.split(" ");//以空格分割为字符串数组
    }

    /**
     * 使用ffmpeg命令行进行抽取音频
     * @param srcFile 原文件
     * @param targetFile 目标文件
     * @return 抽取后的音频文件
     */
    public static  String[] extractAudio(String srcFile, String targetFile){
        //-vn:video not
        String mixAudioCmd = "ffmpeg -i %s -acodec copy -vn %s";
        mixAudioCmd = String.format(mixAudioCmd, srcFile, targetFile);
        return mixAudioCmd.split(" ");//以空格分割为字符串数组
    }

    /**
     * 使用ffmpeg命令行进行抽取视频
     * @param srcFile 原文件
     * @param targetFile 目标文件
     * @return 抽取后的视频文件
     */
    public static  String[] extractVideo(String srcFile, String targetFile){
        //-an audio not
        String mixAudioCmd = "ffmpeg -i %s -vcodec copy -an %s";
        mixAudioCmd = String.format(mixAudioCmd, srcFile, targetFile);
        return mixAudioCmd.split(" ");//以空格分割为字符串数组
    }

}
