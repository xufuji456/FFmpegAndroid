package com.frank.ffmpeg.util;

import android.annotation.SuppressLint;

import com.frank.ffmpeg.format.VideoLayout;

import java.util.List;
import java.util.Locale;

/**
 * ffmpeg工具：拼接命令行处理音视频
 * Created by frank on 2018/1/23.
 */

public class FFmpegUtil {

    /**
     * 使用ffmpeg命令行进行音频转码
     *
     * @param srcFile    源文件
     * @param targetFile 目标文件（后缀指定转码格式）
     * @return 转码后的文件
     */
    public static String[] transformAudio(String srcFile, String targetFile) {
        String transformAudioCmd = "ffmpeg -i %s %s";
        transformAudioCmd = String.format(transformAudioCmd, srcFile, targetFile);
        return transformAudioCmd.split(" ");
    }

    /**
     * 使用ffmpeg命令行进行音频剪切
     *
     * @param srcFile    源文件
     * @param startTime  剪切的开始时间(单位为秒)
     * @param duration   剪切时长(单位为秒)
     * @param targetFile 目标文件
     * @return 剪切后的文件
     */
    @SuppressLint("DefaultLocale")
    public static String[] cutAudio(String srcFile, int startTime, int duration, String targetFile) {
        String cutAudioCmd = "ffmpeg -i %s -acodec copy -ss %d -t %d %s";
        cutAudioCmd = String.format(cutAudioCmd, srcFile, startTime, duration, targetFile);
        return cutAudioCmd.split(" ");
    }

    /**
     * 使用ffmpeg命令行进行音频合并
     *
     * @param fileList   合并列表
     * @param targetFile 目标文件
     * @return 合并后的文件
     */
    public static String[] concatAudio(List<String> fileList, String targetFile) {
//        ffmpeg -i concat:%s|%s -acodec copy %s
        if (fileList == null || fileList.size() == 0) {
            return null;
        }
        StringBuilder concatBuilder = new StringBuilder();
        concatBuilder.append("concat:");
        for (String file : fileList) {
            concatBuilder.append(file).append("|");
        }
        String concatStr = concatBuilder.substring(0, concatBuilder.length() - 1);
        String concatAudioCmd = "ffmpeg -i %s -acodec libmp3lame -ab 128k -ac 2 -ar 44100 %s";
        concatAudioCmd = String.format(concatAudioCmd, concatStr, targetFile);
        return concatAudioCmd.split(" ");
    }

    /**
     * 使用ffmpeg命令行进行音频混合
     *
     * @param srcFile    源文件
     * @param mixFile    待混合文件
     * @param targetFile 目标文件
     * @return 混合后的文件
     */
    public static String[] mixAudio(String srcFile, String mixFile, String targetFile) {
        //调节音量:使用-vol 50, 其中vol为0-100
        String mixAudioCmd = "ffmpeg -i %s -i %s -filter_complex amix=inputs=2:duration=first -strict -2 %s";
        mixAudioCmd = String.format(mixAudioCmd, srcFile, mixFile, targetFile);
        return mixAudioCmd.split(" ");
    }
    //混音公式：value = sample1 + sample2 - (sample1 * sample2 / (pow(2, 16-1) - 1))


    /**
     * 使用ffmpeg命令行进行音视频合成
     *
     * @param videoFile 视频文件
     * @param audioFile 音频文件
     * @param duration  视频时长
     * @param muxFile   目标文件
     * @return 合成后的文件
     */
    @SuppressLint("DefaultLocale")
    public static String[] mediaMux(String videoFile, String audioFile, int duration, String muxFile) {
        //-t:时长  如果忽略音视频时长，则把"-t %d"去掉
        String mixAudioCmd = "ffmpeg -i %s -i %s -t %d %s";
        mixAudioCmd = String.format(mixAudioCmd, videoFile, audioFile, duration, muxFile);
        return mixAudioCmd.split(" ");
    }

    /**
     * 使用ffmpeg命令行进行抽取音频
     *
     * @param srcFile    原文件
     * @param targetFile 目标文件
     * @return 抽取后的音频文件
     */
    public static String[] extractAudio(String srcFile, String targetFile) {
        //-vn:video not
        String mixAudioCmd = "ffmpeg -i %s -acodec copy -vn %s";
        mixAudioCmd = String.format(mixAudioCmd, srcFile, targetFile);
        return mixAudioCmd.split(" ");
    }

    /**
     * 使用ffmpeg命令行进行抽取视频
     *
     * @param srcFile    原文件
     * @param targetFile 目标文件
     * @return 抽取后的视频文件
     */
    public static String[] extractVideo(String srcFile, String targetFile) {
        //-an audio not
        String mixAudioCmd = "ffmpeg -i %s -vcodec copy -an %s";
        mixAudioCmd = String.format(mixAudioCmd, srcFile, targetFile);
        return mixAudioCmd.split(" ");
    }


    /**
     * 使用ffmpeg命令行进行视频转码
     *
     * @param srcFile    源文件
     * @param targetFile 目标文件（后缀指定转码格式）
     * @return 转码后的文件
     */
    public static String[] transformVideo(String srcFile, String targetFile) {
        //指定目标视频的帧率、码率、分辨率
//        String transformVideoCmd = "ffmpeg -i %s -r 25 -b 200 -s 1080x720 %s";
        String transformVideoCmd = "ffmpeg -i %s -vcodec copy -acodec copy %s";
        transformVideoCmd = String.format(transformVideoCmd, srcFile, targetFile);
        return transformVideoCmd.split(" ");
    }

    /**
     * 使用ffmpeg命令行进行视频剪切
     *
     * @param srcFile    源文件
     * @param startTime  剪切的开始时间(单位为秒)
     * @param duration   剪切时长(单位为秒)
     * @param targetFile 目标文件
     * @return 剪切后的文件
     */
    @SuppressLint("DefaultLocale")
    public static String[] cutVideo(String srcFile, int startTime, int duration, String targetFile) {
        //指定音视频编码器:ffmpeg -i %s -ss %d -t %d -acodec libmp3lame -vcodec libx264 %s
        String cutVideoCmd = "ffmpeg -i %s -ss %d -t %d -acodec copy -vcodec copy %s";
        cutVideoCmd = String.format(cutVideoCmd, srcFile, startTime, duration, targetFile);
        return cutVideoCmd.split(" ");
    }

    /**
     * 使用ffmpeg命令行进行视频截图
     *
     * @param srcFile    源文件
     * @param time       截图开始时间
     * @param targetFile 目标文件
     * @return 截图后的文件
     */
    public static String[] screenShot(String srcFile, int time, String targetFile) {
        String screenShotCmd = "ffmpeg -i %s -f image2 -ss %d -vframes 1 -an %s";
        screenShotCmd = String.format(Locale.getDefault(), screenShotCmd, srcFile, time, targetFile);
        return screenShotCmd.split(" ");
    }

    /**
     * 使用ffmpeg命令行给视频添加水印
     *
     * @param srcFile    源文件
     * @param waterMark  水印文件路径
     * @param targetFile 目标文件
     * @return 添加水印后的文件
     */
    public static String[] addWaterMark(String srcFile, String waterMark, String resolution, int bitRate, String targetFile) {
        String mBitRate = String.valueOf(bitRate) + "k";
        String waterMarkCmd = "ffmpeg -i %s -i %s -s %s -b:v %s -filter_complex overlay=0:0 %s";
        waterMarkCmd = String.format(waterMarkCmd, srcFile, waterMark, resolution, mBitRate, targetFile);
        return waterMarkCmd.split(" ");
    }

    /**
     * 使用ffmpeg命令行进行视频转成Gif动图
     *
     * @param srcFile    源文件
     * @param startTime  开始时间
     * @param duration   截取时长
     * @param targetFile 目标文件
     * @param resolution 分辨率
     * @param frameRate  帧率
     * @return Gif文件
     */
    @SuppressLint("DefaultLocale")
    public static String[] generateGif(String srcFile, int startTime, int duration,
                                       String resolution, int frameRate, String targetFile) {
        String generateGifCmd = "ffmpeg -i %s -ss %d -t %d -s %s -r %d -f gif %s";
        generateGifCmd = String.format(generateGifCmd, srcFile, startTime, duration,
                resolution, frameRate, targetFile);
        return generateGifCmd.split(" ");
    }

    /**
     * 使用ffmpeg命令行进行屏幕录制
     *
     * @param size       视频尺寸大小
     * @param recordTime 录屏时间
     * @param targetFile 目标文件
     * @return 屏幕录制文件
     */
    @SuppressLint("DefaultLocale")
    public static String[] screenRecord(String size, int recordTime, String targetFile) {
        //-vd x11:0,0 指录制所使用的偏移为 x=0 和 y=0
        //String screenRecordCmd = "ffmpeg -vcodec mpeg4 -b 1000 -r 10 -g 300 -vd x11:0,0 -s %s %s";
        String screenRecordCmd = "ffmpeg -vcodec mpeg4 -b 1000 -r 10 -g 300 -vd x11:0,0 -s %s -t %d %s";
        screenRecordCmd = String.format(screenRecordCmd, size, recordTime, targetFile);
        return screenRecordCmd.split(" ");
    }

    /**
     * 使用ffmpeg命令行进行图片合成视频
     *
     * @param srcFile    源文件
     * @param frameRate  合成视频帧率
     * @param targetFile 目标文件
     * @return 合成的视频文件
     */
    @SuppressLint("DefaultLocale")
    public static String[] pictureToVideo(String srcFile, int frameRate, String targetFile) {
        //-f image2：代表使用image2格式，需要放在输入文件前面
        String combineVideo = "ffmpeg -f image2 -r %d -i %simg#d.jpg -vcodec mpeg4 %s";
        combineVideo = String.format(combineVideo, frameRate, srcFile, targetFile);
        combineVideo = combineVideo.replace("#", "%");
        return combineVideo.split(" ");//以空格分割为字符串数组
    }

    /**
     * 转换图片尺寸大小
     *
     * @param srcFile    源文件
     * @param resolution  分辨率
     * @param targetFile 目标文件
     * @return 转换后的图片命令行
     */
    @SuppressLint("DefaultLocale")
    public static String[] convertResolution(String srcFile, String resolution, String targetFile) {
        String convertCmd = "ffmpeg -i %s -s %s %s";
        convertCmd = String.format(convertCmd, srcFile, resolution, targetFile);
        return convertCmd.split(" ");
    }

    /**
     * 音频编码
     *
     * @param srcFile    源文件pcm裸流
     * @param targetFile 编码后目标文件
     * @param sampleRate 采样率
     * @param channel    声道:单声道为1/立体声道为2
     * @return 音频编码的命令行
     */
    @SuppressLint("DefaultLocale")
    public static String[] encodeAudio(String srcFile, String targetFile, int sampleRate, int channel) {
        String combineVideo = "ffmpeg -f s16le -ar %d -ac %d -i %s %s";
        combineVideo = String.format(combineVideo, sampleRate, channel, srcFile, targetFile);
        return combineVideo.split(" ");
    }

    /**
     * 多画面拼接视频
     *
     * @param input1      输入文件1
     * @param input2      输入文件2
     * @param videoLayout 视频布局
     * @param targetFile  画面拼接文件
     * @return 画面拼接的命令行
     */
    public static String[] multiVideo(String input1, String input2, String targetFile, int videoLayout) {
//        String multiVideo = "ffmpeg -i %s -i %s -i %s -i %s -filter_complex " +
//                "\"[0:v]pad=iw*2:ih*2[a];[a][1:v]overlay=w[b];[b][2:v]overlay=0:h[c];[c][3:v]overlay=w:h\" %s";
        String multiVideo = "ffmpeg -i %s -i %s -filter_complex hstack %s";//hstack:水平拼接，默认
        if (videoLayout == VideoLayout.LAYOUT_VERTICAL) {//vstack:垂直拼接
            multiVideo = multiVideo.replace("hstack", "vstack");
        }
        multiVideo = String.format(multiVideo, input1, input2, targetFile);
        return multiVideo.split(" ");
    }

    /**
     * 视频反序倒播
     *
     * @param inputFile  输入文件
     * @param targetFile 反序文件
     * @return 视频反序的命令行
     */
    public static String[] reverseVideo(String inputFile, String targetFile) {
        //FIXME 音频也反序
//        String reverseVideo = "ffmpeg -i %s -filter_complex [0:v]reverse[v];[0:a]areverse[a] -map [v] -map [a] %s";
        String reverseVideo = "ffmpeg -i %s -filter_complex [0:v]reverse[v] -map [v] %s";//单纯视频反序
        reverseVideo = String.format(reverseVideo, inputFile, targetFile);
        return reverseVideo.split(" ");
    }

    /**
     * 视频降噪
     *
     * @param inputFile  输入文件
     * @param targetFile 输出文件
     * @return 视频降噪的命令行
     */
    public static String[] denoiseVideo(String inputFile, String targetFile) {
        String reverseVideo = "ffmpeg -i %s -nr 500 %s";
        reverseVideo = String.format(reverseVideo, inputFile, targetFile);
        return reverseVideo.split(" ");
    }

    /**
     * 视频抽帧转成图片
     *
     * @param inputFile  输入文件
     * @param startTime  开始时间
     * @param duration   持续时间
     * @param frameRate  帧率
     * @param targetFile 输出文件
     * @return 视频抽帧的命令行
     */
    public static String[] videoToImage(String inputFile, int startTime, int duration, int frameRate, String targetFile) {
        //-ss：开始时间，单位为秒
        //-t：持续时间，单位为秒
        //-r：帧率，每秒抽多少帧
        String toImage = "ffmpeg -i %s -ss %s -t %s -r %s %s";
        toImage = String.format(Locale.CHINESE, toImage, inputFile, startTime, duration, frameRate, targetFile);
        toImage = toImage + "%3d.jpg";
        return toImage.split(" ");
    }

    /**
     * 视频叠加成画中画
     *
     * @param inputFile1 输入文件
     * @param inputFile2 输入文件
     * @param targetFile 输出文件
     * @param x          小视频起点x坐标
     * @param y          小视频起点y坐标
     * @return 视频画中画的命令行
     */
    @SuppressLint("DefaultLocale")
    public static String[] picInPicVideo(String inputFile1, String inputFile2, int x, int y, String targetFile) {
        String reverseVideo = "ffmpeg -i %s -i %s -filter_complex overlay=%d:%d %s";
        reverseVideo = String.format(reverseVideo, inputFile1, inputFile2, x, y, targetFile);
        return reverseVideo.split(" ");
    }

    /**
     * mp4视频的moov往mdat前面移动
     *
     * @param inputFile  inputFile
     * @param outputFile outputFile
     * @return 移动moov命令行
     */
    public static String[] moveMoovAhead(String inputFile, String outputFile) {
        String moovCmd = "ffmpeg -i %s -movflags faststart -acodec copy -vcodec copy %s";
        moovCmd = String.format(Locale.getDefault(), moovCmd, inputFile, outputFile);
        return moovCmd.split(" ");
    }

    /**
     * 使用ffprobe探测多媒体格式
     *
     * @param inputFile  inputFile
     * @return 多媒体格式数据
     */
    public static String[] probeFormat(String inputFile) {
        //show format:ffprobe -i %s -show_format -print_format json
        //show stream:ffprobe -i %s -show_streams
        String ffprobeCmd = "ffprobe -i %s -show_streams -show_format -print_format json";
        ffprobeCmd = String.format(Locale.getDefault(), ffprobeCmd, inputFile);
        return ffprobeCmd.split(" ");
    }

}
