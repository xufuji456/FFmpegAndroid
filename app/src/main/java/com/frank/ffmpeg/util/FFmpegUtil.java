package com.frank.ffmpeg.util;

import android.annotation.SuppressLint;

import com.frank.ffmpeg.FFmpegApplication;
import com.frank.ffmpeg.format.VideoLayout;

import java.util.List;
import java.util.Locale;

/**
 * ffmpeg tool: assemble the complete command
 * Created by frank on 2018/1/23.
 */

public class FFmpegUtil {

    /**
     * transform audio, according to your assigning the output format
     *
     * @param srcFile    input file
     * @param targetFile output file
     * @return transform success or not
     */
    public static String[] transformAudio(String srcFile, String targetFile) {
        String transformAudioCmd = "ffmpeg -i %s %s";
        transformAudioCmd = String.format(transformAudioCmd, srcFile, targetFile);
        return transformAudioCmd.split(" ");
    }

    public static String[] transformAudio(String srcFile, String acodec, String targetFile) {
        String transformAudioCmd = "ffmpeg -i %s -acodec %s -ac 2 -ar 44100 %s";
        transformAudioCmd = String.format(transformAudioCmd, srcFile, acodec, targetFile);
        return transformAudioCmd.split(" ");
    }

    /**
     * cut audio, you could assign the startTime and duration which you want to
     *
     * @param srcFile    input file
     * @param startTime  start time in the audio(unit is second)
     * @param duration   start time(unit is second)
     * @param targetFile output file
     * @return cut success or not
     */
    @SuppressLint("DefaultLocale")
    public static String[] cutAudio(String srcFile, int startTime, int duration, String targetFile) {
        String cutAudioCmd = "ffmpeg -i %s -acodec copy -vn -ss %d -t %d %s";
        cutAudioCmd = String.format(cutAudioCmd, srcFile, startTime, duration, targetFile);
        return cutAudioCmd.split(" ");
    }

    /**
     * concat all the audio together
     *
     * @param fileList   list of files
     * @param targetFile output file
     * @return concat success or not
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
        String concatAudioCmd = "ffmpeg -i %s -acodec copy %s";
        concatAudioCmd = String.format(concatAudioCmd, concatStr, targetFile);
        return concatAudioCmd.split(" ");
    }

    /**
     * mix one and another audio
     *
     * @param srcFile    input file
     * @param mixFile    background music
     * @param targetFile output file
     * @return mix success or not
     */
    public static String[] mixAudio(String srcFile, String mixFile, String targetFile) {
        //adjust volume:using '-vol 50', which is form 0 to 100
        String mixAudioCmd = "ffmpeg -i %s -i %s -filter_complex amix=inputs=2:duration=first -strict -2 %s";
        mixAudioCmd = String.format(mixAudioCmd, srcFile, mixFile, targetFile);
        return mixAudioCmd.split(" ");
    }
    //mixing formula: value = sample1 + sample2 - (sample1 * sample2 / (pow(2, 16-1) - 1))


    /**
     * mux audio and video together
     *
     * @param videoFile the file of pure video
     * @param audioFile the file of pure audio
     * @param duration  the duration of video
     * @param muxFile   output file
     * @return mux success or not
     */
    @SuppressLint("DefaultLocale")
    public static String[] mediaMux(String videoFile, String audioFile, int duration, String muxFile) {
        String mixAudioCmd = "ffmpeg -i %s -i %s -t %d %s";
        mixAudioCmd = String.format(mixAudioCmd, videoFile, audioFile, duration, muxFile);
        return mixAudioCmd.split(" ");
    }

    /**
     * extract audio from media file
     *
     * @param srcFile    input file
     * @param targetFile output file
     * @return demux audio success or not
     */
    public static String[] extractAudio(String srcFile, String targetFile) {
        //-vn: disable video
        String mixAudioCmd = "ffmpeg -i %s -acodec copy -vn %s";
        mixAudioCmd = String.format(mixAudioCmd, srcFile, targetFile);
        return mixAudioCmd.split(" ");
    }

    /**
     * extract pure video from media file
     *
     * @param srcFile    input file
     * @param targetFile output file
     * @return demux video success or not
     */
    public static String[] extractVideo(String srcFile, String targetFile) {
        //-an: disable audio
        String mixAudioCmd = "ffmpeg -i %s -vcodec copy -an %s";
        mixAudioCmd = String.format(mixAudioCmd, srcFile, targetFile);
        return mixAudioCmd.split(" ");
    }


    /**
     * transform video, according to your assigning the output format
     *
     * @param srcFile    input file
     * @param targetFile output file
     * @return transform video success or not
     */
    public static String[] transformVideo(String srcFile, String targetFile) {
        //just copy codec
//        String transformVideoCmd = "ffmpeg -i %s -vcodec copy -acodec copy %s";
        // assign the frameRate, bitRate and resolution
//        String transformVideoCmd = "ffmpeg -i %s -r 25 -b 200 -s 1080x720 %s";
        // assign the encoder
        String transformVideoCmd = "ffmpeg -i %s -vcodec libx264 -acodec libmp3lame %s";
        transformVideoCmd = String.format(transformVideoCmd, srcFile, targetFile);
        return transformVideoCmd.split(" ");
    }

    /**
     * Using FFmpeg to transform video, with re-encode
     *
     * @param srcFile the source file
     * @param targetFile target file
     * @return transform video success or not
     */
    public static String[] transformVideoWithEncode(String srcFile, String targetFile) {
        return transformVideoWithEncode(srcFile, 0, 0, targetFile);
    }

    /**
     * Using FFmpeg to transform video, with re-encode and specific resolution
     *
     * @param srcFile  the source file
     * @param width the width of video
     * @param height the height of video
     * @param targetFile target file
     * @return transform video success or not
     */
    public static String[] transformVideoWithEncode(String srcFile, int width, int height, String targetFile) {
        String transformVideoCmd;
        if (width > 0 && height > 0) {
            String scale = "-vf scale=" + width + ":" + height;
            transformVideoCmd = "ffmpeg -i %s -vcodec libx264 -acodec aac " + scale + " %s";
        } else {
            transformVideoCmd = "ffmpeg -i %s -vcodec libx264 -acodec aac " + "%s";
        }
        transformVideoCmd = String.format(transformVideoCmd, srcFile, targetFile);
        return transformVideoCmd.split(" ");
    }

    /**
     * joint every single video together
     * @param fileListPath the path file list
     * @param targetPath output path
     * @return joint video success or not
     */
    public static String[] jointVideo(String fileListPath, String targetPath) {
        String jointVideoCmd = "ffmpeg -f concat -safe 0 -i %s -c copy %s";
        jointVideoCmd = String.format(jointVideoCmd, fileListPath, targetPath);
        return jointVideoCmd.split(" ");
    }

    /**
     * cut video, you could assign the startTime and duration which you want to
     *
     * @param srcFile    input file
     * @param startTime  startTime in the video(unit is second)
     * @param duration   duration
     * @param targetFile output file
     * @return cut video success or not
     */
    @SuppressLint("DefaultLocale")
    public static String[] cutVideo(String srcFile, int startTime, int duration, String targetFile) {
        //assign encoders: ffmpeg -i %s -ss %d -t %d -acodec libmp3lame -vcodec libx264 %s
        String cutVideoCmd = "ffmpeg -i %s -ss %d -t %d -acodec copy -vcodec copy %s";
        cutVideoCmd = String.format(cutVideoCmd, srcFile, startTime, duration, targetFile);
        return cutVideoCmd.split(" ");
    }

    /**
     * screenshot from video, you could assign the specific time
     *
     * @param srcFile    input file
     * @param time       which time you want to shot
     * @param targetFile output file
     * @return screenshot success or not
     */
    public static String[] screenShot(String srcFile, int time, String targetFile) {
        String screenShotCmd = "ffmpeg -i %s -f image2 -ss %d -vframes 1 -an %s";
        screenShotCmd = String.format(Locale.getDefault(), screenShotCmd, srcFile, time, targetFile);
        return screenShotCmd.split(" ");
    }

    /**
     * add watermark with image to video, you could assign the location and bitRate
     *
     * @param srcFile    input file
     * @param imgPath    the path of the image
     * @param location   the location in the video(1:top left 2:top right 3:bottom left 4:bottom right)
     * @param bitRate    bitRate
     * @param offsetXY   the offset of x and y in the video
     * @param targetFile output file
     * @return add watermark success or not
     */
    public static String[] addWaterMarkImg(String srcFile, String imgPath, int location, int bitRate, int offsetXY, String targetFile) {
        String mBitRate = bitRate + "k";
        String overlay;
        int offset = ScreenUtil.dp2px(FFmpegApplication.getInstance(), offsetXY);
        if (location == 1) {
            overlay = "overlay='" + offset + ":" + offset + "'";
        } else if (location == 2) {
            overlay = "overlay='(main_w-overlay_w)-" + offset + ":" + offset + "'";
        } else if (location == 3) {
            overlay = "overlay='" + offset + ":(main_h-overlay_h)-" + offset + "'";
        } else if (location == 4) {
            overlay = "overlay='(main_w-overlay_w)-" + offset + ":(main_h-overlay_h)-" + offset + "'";
        } else {
            overlay = "overlay='(main_w-overlay_w)-" + offset + ":" + offset + "'";
        }
        String waterMarkCmd = "ffmpeg -i %s -i %s -b:v %s -filter_complex %s -preset:v superfast %s";
        waterMarkCmd = String.format(waterMarkCmd, srcFile, imgPath, mBitRate, overlay, targetFile);
        return waterMarkCmd.split(" ");
    }

    /**
     * add watermark with gif to video, you could assign the location and bitRate
     *
     * @param srcFile    input file
     * @param imgPath    the path of the gif
     * @param location   the location in the video(1:top left 2:top right 3:bottom left 4:bottom right)
     * @param bitRate    bitRate
     * @param offsetXY   the offset of x and y in the video
     * @param targetFile output file
     * @return add watermark success or not
     */
    public static String[] addWaterMarkGif(String srcFile, String imgPath, int location, int bitRate, int offsetXY, String targetFile) {
        String mBitRate = bitRate + "k";
        String overlay;
        int offset = ScreenUtil.dp2px(FFmpegApplication.getInstance(), offsetXY);
        if (location == 1) {
            overlay = "overlay='" + offset + ":" + offset + ":shortest=1'";
        } else if (location == 2) {
            overlay = "overlay='(main_w-overlay_w)-" + offset + ":" + offset + ":shortest=1'";
        } else if (location == 3) {
            overlay = "overlay='" + offset + ":(main_h-overlay_h)-" + offset + ":shortest=1'";
        } else if (location == 4) {
            overlay = "overlay='(main_w-overlay_w)-" + offset + ":(main_h-overlay_h)-" + offset + ":shortest=1'";
        } else {
            overlay = "overlay='(main_w-overlay_w)-" + offset + ":" + offset + ":shortest=1'";
        }
        String waterMarkCmd = "ffmpeg -i %s -ignore_loop 0 -i %s -b:v %s -filter_complex %s -preset:v superfast %s";
        waterMarkCmd = String.format(waterMarkCmd, srcFile, imgPath, mBitRate, overlay, targetFile);
        return waterMarkCmd.split(" ");
    }

    /**
     * convert video into gif
     *
     * @param srcFile    input file
     * @param startTime  startTime in the video
     * @param duration   duration, how long you want to
     * @param targetFile output file
     * @param resolution resolution of the gif
     * @param frameRate  frameRate of the gif
     * @return convert gif success or not
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
     * screen record
     *
     * @param size       size of video
     * @param recordTime startTime in the video
     * @param targetFile output file
     * @return record success or not
     */
    @SuppressLint("DefaultLocale")
    public static String[] screenRecord(String size, int recordTime, String targetFile) {
        String screenRecordCmd = "ffmpeg -vcodec mpeg4 -b 1000 -r 10 -g 300 -vd x11:0,0 -s %s -t %d %s";
        screenRecordCmd = String.format(screenRecordCmd, size, recordTime, targetFile);
        return screenRecordCmd.split(" ");
    }

    /**
     * convert s series of pictures into video
     *
     * @param srcFile    input file
     * @param frameRate  frameRate
     * @param targetFile output file
     * @return convert success or not
     */
    @SuppressLint("DefaultLocale")
    public static String[] pictureToVideo(String srcFile, int frameRate, String targetFile) {
        //-f: stand for format
        String combineVideo = "ffmpeg -f image2 -r %d -i %simg#d.jpg -vcodec mpeg4 %s";
        combineVideo = String.format(combineVideo, frameRate, srcFile, targetFile);
        combineVideo = combineVideo.replace("#", "%");
        return combineVideo.split(" ");
    }

    /**
     * convert resolution
     *
     * @param srcFile    input file
     * @param resolution  resolution
     * @param targetFile output file
     * @return convert success or not
     */
    @SuppressLint("DefaultLocale")
    public static String[] convertResolution(String srcFile, String resolution, String targetFile) {
        String convertCmd = "ffmpeg -i %s -s %s -pix_fmt yuv420p %s";
        convertCmd = String.format(convertCmd, srcFile, resolution, targetFile);
        return convertCmd.split(" ");
    }

    /**
     * encode audio, you could assign the sampleRate and channel
     *
     * @param srcFile    pcm raw audio
     * @param targetFile output file
     * @param sampleRate sampleRate
     * @param channel    sound channel: mono channel is 1, stereo channel is 2
     * @return encode audio success or not
     */
    @SuppressLint("DefaultLocale")
    public static String[] encodeAudio(String srcFile, String targetFile, int sampleRate, int channel) {
        String combineVideo = "ffmpeg -f s16le -ar %d -ac %d -i %s %s";
        combineVideo = String.format(combineVideo, sampleRate, channel, srcFile, targetFile);
        return combineVideo.split(" ");
    }

    /**
     * join multi videos together
     *
     * @param input1      input one
     * @param input2      input two
     * @param videoLayout the layout of video, which could be horizontal or vertical
     * @param targetFile  output file
     * @return join success or not
     */
    public static String[] multiVideo(String input1, String input2, String targetFile, int videoLayout) {
//        String multiVideo = "ffmpeg -i %s -i %s -i %s -i %s -filter_complex " +
//                "\"[0:v]pad=iw*2:ih*2[a];[a][1:v]overlay=w[b];[b][2:v]overlay=0:h[c];[c][3:v]overlay=w:h\" %s";
        String multiVideo = "ffmpeg -i %s -i %s -filter_complex hstack %s";//hstack: horizontal
        if (videoLayout == VideoLayout.LAYOUT_VERTICAL) {//vstack: vertical
            multiVideo = multiVideo.replace("hstack", "vstack");
        }
        multiVideo = String.format(multiVideo, input1, input2, targetFile);
        return multiVideo.split(" ");
    }

    /**
     * reverse video
     *
     * @param inputFile  input file
     * @param targetFile output file
     * @return reverse success or not
     */
    public static String[] reverseVideo(String inputFile, String targetFile) {
        //-vf reverse: only video reverse, -an: disable audio
        //tip: reverse will cost a lot of time, only short video are recommended
        String reverseVideo = "ffmpeg -i %s -vf reverse -an %s";
        reverseVideo = String.format(reverseVideo, inputFile, targetFile);
        return reverseVideo.split(" ");
    }

    /**
     * noise reduction with video
     *
     * @param inputFile  input file
     * @param targetFile output file
     * @return noise reduction success or not
     */
    public static String[] denoiseVideo(String inputFile, String targetFile) {
        String denoiseVideo = "ffmpeg -i %s -nr 500 %s";
        denoiseVideo = String.format(denoiseVideo, inputFile, targetFile);
        return denoiseVideo.split(" ");
    }

    /**
     * covert video into a series of pictures
     *
     * @param inputFile  input file
     * @param startTime  startTime in the video
     * @param duration   duration, how long you want
     * @param frameRate  frameRate
     * @param targetFile output file
     * @return convert success or not
     */
    public static String[] videoToImage(String inputFile, int startTime, int duration, int frameRate, String targetFile) {
        //-ss：start time
        //-t：duration
        //-r：frame rate
        String toImage = "ffmpeg -i %s -ss %s -t %s -r %s %s";
        toImage = String.format(Locale.CHINESE, toImage, inputFile, startTime, duration, frameRate, targetFile);
        toImage = toImage + "%3d.jpg";
        return toImage.split(" ");
    }

    /**
     * convert videos into picture-in-picture mode
     *
     * @param inputFile1 input one
     * @param inputFile2 input two
     * @param targetFile output file
     * @param x          x coordinate point
     * @param y          y coordinate point
     * @return convert success or not
     */
    @SuppressLint("DefaultLocale")
    public static String[] picInPicVideo(String inputFile1, String inputFile2, int x, int y, String targetFile) {
        String pipVideo = "ffmpeg -i %s -i %s -filter_complex overlay=%d:%d %s";
        pipVideo = String.format(pipVideo, inputFile1, inputFile2, x, y, targetFile);
        return pipVideo.split(" ");
    }

    /**
     * move moov box in the front of mdat box, when moox box is behind mdat box(only mp4)
     *
     * @param inputFile  inputFile
     * @param outputFile outputFile
     * @return move success or not
     */
    public static String[] moveMoovAhead(String inputFile, String outputFile) {
        String moovCmd = "ffmpeg -i %s -movflags faststart -acodec copy -vcodec copy %s";
        moovCmd = String.format(Locale.getDefault(), moovCmd, inputFile, outputFile);
        return moovCmd.split(" ");
    }

    /**
     * using FFprobe to parse the media format
     *
     * @param inputFile  inputFile
     * @return probe success or not
     */
    public static String[] probeFormat(String inputFile) {
        //show format:ffprobe -i %s -show_format -print_format json
        //show stream:ffprobe -i %s -show_streams
        String ffprobeCmd = "ffprobe -i %s -show_streams -show_format -print_format json";
        ffprobeCmd = String.format(Locale.getDefault(), ffprobeCmd, inputFile);
        return ffprobeCmd.split(" ");
    }

    /**
     * Changing the speed of playing, speed range at 0.5-2 in audio-video mode.
     * However, in pure video mode, the speed range at 0.25-4
     * @param inputFile the inputFile of normal speed
     * @param outputFile the outputFile which you want to change speed
     * @param speed speed of playing
     * @param pureVideo whether pure video or not, default false
     * @return change speed success or not
     */
    public static String[] changeSpeed(String inputFile, String outputFile, float speed, boolean pureVideo) {
        //audio atempo: 0.5--2
        //video pts:0.25--4
        if (pureVideo) {
            if (speed > 4 || speed < 0.25) {
                throw new IllegalArgumentException("speed range is 0.25--4");
            }
        } else {
            if (speed > 2 || speed < 0.5) {
                throw new IllegalArgumentException("speed range is 0.5--2");
            }
        }
        float ptsFactor = 1/speed;
        String speedCmd;
        if (pureVideo) {
            speedCmd = "ffmpeg -i %s -filter_complex [0:v]setpts=%.2f*PTS[v] -map [v] %s";
            speedCmd = String.format(Locale.getDefault(), speedCmd, inputFile, ptsFactor, outputFile);
        } else {
            speedCmd = "ffmpeg -i %s -filter_complex [0:v]setpts=%.2f*PTS[v];[0:a]atempo=%.2f[a] -map [v] -map [a] %s";
            speedCmd = String.format(Locale.getDefault(), speedCmd, inputFile, ptsFactor, speed, outputFile);
        }
        return speedCmd.split(" ");
    }

}
