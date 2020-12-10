package com.frank.ffmpeg.util;

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
     * @param inputPath  input file
     * @param outputPath output file
     * @return transform success or not
     */
    public static String[] transformAudio(String inputPath, String outputPath) {
        String transformAudioCmd = "ffmpeg -i %s %s";
        transformAudioCmd = String.format(transformAudioCmd, inputPath, outputPath);
        return transformAudioCmd.split(" ");
    }

    public static String[] transformAudio(String inputPath, String acodec, String outputPath) {
        String transformAudioCmd = "ffmpeg -i %s -acodec %s -ac 2 -ar 44100 %s";
        transformAudioCmd = String.format(transformAudioCmd, inputPath, acodec, outputPath);
        return transformAudioCmd.split(" ");
    }

    /**
     * cut audio, you could assign the startTime and duration which you want to
     *
     * @param inputPath  input file
     * @param startTime  start time in the audio(unit is second)
     * @param duration   start time(unit is second)
     * @param outputPath output file
     * @return cut success or not
     */
    public static String[] cutAudio(String inputPath, int startTime, int duration, String outputPath) {
        String cutAudioCmd = "ffmpeg -ss %d -accurate_seek -t %d -i %s -acodec copy -vn %s";
        cutAudioCmd = String.format(Locale.getDefault(), cutAudioCmd, startTime, duration, inputPath, outputPath);
        return cutAudioCmd.split(" ");
    }

    /**
     * concat all the audio together
     *
     * @param fileList   list of files
     * @param outputPath output file
     * @return concat success or not
     */
    public static String[] concatAudio(List<String> fileList, String outputPath) {
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
        concatAudioCmd = String.format(concatAudioCmd, concatStr, outputPath);
        return concatAudioCmd.split(" ");
    }

    /**
     * mix one and another audio
     *
     * @param inputPath  input file
     * @param mixFile    background music
     * @param outputPath output file
     * @return mix success or not
     */
    public static String[] mixAudio(String inputPath, String mixFile, String outputPath) {
        //adjust volume:using '-vol 50', which is form 0 to 100
        String mixAudioCmd = "ffmpeg -i %s -i %s -filter_complex amix=inputs=2:duration=first -strict -2 %s";
        mixAudioCmd = String.format(mixAudioCmd, inputPath, mixFile, outputPath);
        return mixAudioCmd.split(" ");
    }
    //mixing formula: value = sample1 + sample2 - (sample1 * sample2 / (pow(2, 16-1) - 1))

    /**
     * mux audio and video together
     *
     * @param videoFile the file of pure video
     * @param audioFile the file of pure audio
     * @param copy      copy codec
     * @param muxFile   output file
     * @return mux success or not
     */
    public static String[] mediaMux(String videoFile, String audioFile, boolean copy, String muxFile) {
        String mixAudioCmd;
        if (copy) {
            mixAudioCmd = "ffmpeg -i %s -i %s -codec copy -y %s";
        } else {
            mixAudioCmd = "ffmpeg -i %s -i %s -y %s";
        }
        mixAudioCmd = String.format(Locale.getDefault(), mixAudioCmd, videoFile, audioFile, muxFile);
        return mixAudioCmd.split(" ");
    }

    /**
     * extract audio from media file
     *
     * @param inputPath  input file
     * @param outputPath output file
     * @return demux audio success or not
     */
    public static String[] extractAudio(String inputPath, String outputPath) {
        //-vn: disable video
        String mixAudioCmd = "ffmpeg -i %s -acodec copy -vn %s";
        mixAudioCmd = String.format(mixAudioCmd, inputPath, outputPath);
        return mixAudioCmd.split(" ");
    }

    /**
     * extract pure video from media file
     *
     * @param inputPath  input file
     * @param outputPath output file
     * @return demux video success or not
     */
    public static String[] extractVideo(String inputPath, String outputPath) {
        //-an: disable audio
        String mixAudioCmd = "ffmpeg -i %s -vcodec copy -an %s";
        mixAudioCmd = String.format(mixAudioCmd, inputPath, outputPath);
        return mixAudioCmd.split(" ");
    }

    /**
     * transform video, according to your assigning the output format
     *
     * @param inputPath  input file
     * @param outputPath output file
     * @return transform video success or not
     */
    public static String[] transformVideo(String inputPath, String outputPath) {
        //just copy codec
//        String transformVideoCmd = "ffmpeg -i %s -vcodec copy -acodec copy %s";
        // assign the frameRate, bitRate and resolution
//        String transformVideoCmd = "ffmpeg -i %s -r 25 -b 200 -s 1080x720 %s";
        // assign the encoder
        String transformVideoCmd = "ffmpeg -i %s -vcodec libx264 -acodec libmp3lame %s";
        transformVideoCmd = String.format(transformVideoCmd, inputPath, outputPath);
        return transformVideoCmd.split(" ");
    }

    /**
     * Using FFmpeg to transform video, with re-encode
     *
     * @param inputPath  the source file
     * @param outputPath target file
     * @return transform video success or not
     */
    public static String[] transformVideoWithEncode(String inputPath, String outputPath) {
        return transformVideoWithEncode(inputPath, 0, 0, outputPath);
    }

    /**
     * Using FFmpeg to transform video, with re-encode and specific resolution
     *
     * @param inputPath  the source file
     * @param width      the width of video
     * @param height     the height of video
     * @param outputPath target file
     * @return transform video success or not
     */
    public static String[] transformVideoWithEncode(String inputPath, int width, int height, String outputPath) {
        String transformVideoCmd;
        if (width > 0 && height > 0) {
            String scale = "-vf scale=" + width + ":" + height;
            transformVideoCmd = "ffmpeg -i %s -vcodec libx264 -acodec aac " + scale + " %s";
        } else {
            transformVideoCmd = "ffmpeg -i %s -vcodec libx264 -acodec aac " + "%s";
        }
        transformVideoCmd = String.format(transformVideoCmd, inputPath, outputPath);
        return transformVideoCmd.split(" ");
    }

    /**
     * joint every single video together
     * @param fileListPath the path file list
     * @param outputPath   output path
     * @return joint video success or not
     */
    public static String[] jointVideo(String fileListPath, String outputPath) {
        String jointVideoCmd = "ffmpeg -f concat -safe 0 -i %s -c copy %s";
        jointVideoCmd = String.format(jointVideoCmd, fileListPath, outputPath);
        return jointVideoCmd.split(" ");
    }

    /**
     * cut video, you could assign the startTime and duration which you want to
     *
     * @param inputPath  input file
     * @param startTime  startTime in the video(unit is second)
     * @param duration   duration
     * @param outputPath output file
     * @return cut video success or not
     */
    public static String[] cutVideo(String inputPath, int startTime, int duration, String outputPath) {
        String cutVideoCmd = "ffmpeg -ss %d -accurate_seek -t %d -i %s -acodec copy -vcodec copy -avoid_negative_ts 1 %s";
        cutVideoCmd = String.format(Locale.getDefault(), cutVideoCmd, startTime, duration, inputPath, outputPath);
        return cutVideoCmd.split(" ");
    }

    /**
     * screenshot from video, you could assign the specific time
     *
     * @param inputPath  input file
     * @param time       which time you want to shot
     * @param outputPath output file
     * @return screenshot success or not
     */
    public static String[] screenShot(String inputPath, int time, String outputPath) {
        String screenShotCmd = "ffmpeg -ss %d -i %s -f image2 -vframes 1 -an %s";
        screenShotCmd = String.format(Locale.getDefault(), screenShotCmd, time, inputPath, outputPath);
        return screenShotCmd.split(" ");
    }

    private static String obtainOverlay(int offsetX, int offsetY, int location) {
        switch (location) {
            case 2:
                return "overlay='(main_w-overlay_w)-" + offsetX + ":" + offsetY + "'";
            case 3:
                return "overlay='" + offsetX + ":(main_h-overlay_h)-" + offsetY + "'";
            case 4:
                return "overlay='(main_w-overlay_w)-" + offsetX + ":(main_h-overlay_h)-" + offsetY + "'";
            case 1:
            default:
                return "overlay=" + offsetX + ":" + offsetY;
        }
    }

    /**
     * add watermark with image to video, you could assign the location and bitRate
     *
     * @param inputPath  input file
     * @param imgPath    the path of the image
     * @param location   the location in the video(1:top left 2:top right 3:bottom left 4:bottom right)
     * @param bitRate    bitRate
     * @param offsetXY   the offset of x and y in the video
     * @param outputPath output file
     * @return add watermark success or not
     */
    public static String[] addWaterMarkImg(String inputPath, String imgPath, int location, int bitRate,
                                           int offsetXY, String outputPath) {
        String mBitRate = bitRate + "k";
        int offset = ScreenUtil.dp2px(FFmpegApplication.getInstance(), offsetXY);
        String overlay = obtainOverlay(offset, offset, location);
        String waterMarkCmd = "ffmpeg -i %s -i %s -b:v %s -filter_complex %s -preset:v superfast %s";
        waterMarkCmd = String.format(waterMarkCmd, inputPath, imgPath, mBitRate, overlay, outputPath);
        return waterMarkCmd.split(" ");
    }

    /**
     * add watermark with gif to video, you could assign the location and bitRate
     *
     * @param inputPath  input file
     * @param imgPath    the path of the gif
     * @param location   the location in the video(1:top left 2:top right 3:bottom left 4:bottom right)
     * @param bitRate    bitRate
     * @param offsetXY   the offset of x and y in the video
     * @param outputPath output file
     * @return add watermark success or not
     */
    public static String[] addWaterMarkGif(String inputPath, String imgPath, int location, int bitRate,
                                           int offsetXY, String outputPath) {
        String mBitRate = bitRate + "k";
        int offset = ScreenUtil.dp2px(FFmpegApplication.getInstance(), offsetXY);
        String overlay = obtainOverlay(offset, offset, location) + ":shortest=1";
        String waterMarkCmd = "ffmpeg -i %s -ignore_loop 0 -i %s -b:v %s -filter_complex %s -preset:v superfast %s";
        waterMarkCmd = String.format(waterMarkCmd, inputPath, imgPath, mBitRate, overlay, outputPath);
        return waterMarkCmd.split(" ");
    }

    /**
     * generate a palette for gif
     *
     * @param inputPath  input file
     * @param frameRate  frameRate of the gif
     * @param startTime  startTime in the video
     * @param duration   duration, how long you want to
     * @param width      width
     * @param outputPath output file
     * @return generate palette success or not
     */
    public static String[] generatePalette(String inputPath, int startTime, int duration,
                                           int frameRate, int width, String outputPath) {
        String paletteCmd = "ffmpeg -ss %d -accurate_seek -t %d -i %s -vf fps=%d,scale=%d:-1:flags=lanczos,palettegen %s";
        paletteCmd = String.format(Locale.getDefault(), paletteCmd, startTime,
                duration, inputPath, frameRate, width, outputPath);
        return paletteCmd.split(" ");
    }

    /**
     * convert video into gif with palette
     *
     * @param inputPath  input file
     * @param palette    the palette which will apply to gif
     * @param startTime  startTime in the video
     * @param duration   duration, how long you want to
     * @param frameRate  frameRate of the gif
     * @param width      width
     * @param outputPath output gif
     * @return convert gif success or not
     */
    public static String[] generateGifByPalette(String inputPath, String palette, int startTime, int duration,
                                           int frameRate, int width, String outputPath) {
        String paletteGifCmd = "ffmpeg -ss %d -accurate_seek -t %d -i %s -i %s -lavfi fps=%d,scale=%d:-1:flags=lanczos[x];[x][1:v]" +
                "paletteuse=dither=bayer:bayer_scale=3 %s";
        paletteGifCmd = String.format(Locale.getDefault(), paletteGifCmd, startTime,
                duration, inputPath, palette, frameRate, width, outputPath);
        return paletteGifCmd.split(" ");
    }

    /**
     * convert s series of pictures into video
     *
     * @param inputPath  input file
     * @param frameRate  frameRate
     * @param outputPath output file
     * @return convert success or not
     */
    public static String[] pictureToVideo(String inputPath, int frameRate, String outputPath) {
        //-f: stand for format
        String combineVideo = "ffmpeg -f image2 -r %d -i %simg#d.jpg -vcodec mpeg4 %s";
        combineVideo = String.format(Locale.getDefault(), combineVideo, frameRate, inputPath, outputPath);
        combineVideo = combineVideo.replace("#", "%");
        return combineVideo.split(" ");
    }

    /**
     * convert resolution
     *
     * @param inputPath   input file
     * @param resolution  resolution
     * @param outputPath  output file
     * @return convert success or not
     */
    public static String[] convertResolution(String inputPath, String resolution, String outputPath) {
        String convertCmd = "ffmpeg -i %s -s %s -pix_fmt yuv420p %s";
        convertCmd = String.format(Locale.getDefault(), convertCmd, inputPath, resolution, outputPath);
        return convertCmd.split(" ");
    }

    /**
     * encode audio, you could assign the sampleRate and channel
     *
     * @param inputPath  pcm raw audio
     * @param outputPath output file
     * @param sampleRate sampleRate
     * @param channel    sound channel: mono channel is 1, stereo channel is 2
     * @return encode audio success or not
     */
    public static String[] encodeAudio(String inputPath, String outputPath, int sampleRate, int channel) {
        String combineVideo = "ffmpeg -f s16le -ar %d -ac %d -i %s %s";
        combineVideo = String.format(Locale.getDefault(), combineVideo, sampleRate, channel, inputPath, outputPath);
        return combineVideo.split(" ");
    }

    /**
     * join multi videos together
     *
     * @param input1      input one
     * @param input2      input two
     * @param videoLayout the layout of video, which could be horizontal or vertical
     * @param outputPath  output file
     * @return join success or not
     */
    public static String[] multiVideo(String input1, String input2, String outputPath, int videoLayout) {
        String multiVideo = "ffmpeg -i %s -i %s -filter_complex hstack %s";//hstack: horizontal
        if (videoLayout == VideoLayout.LAYOUT_VERTICAL) {//vstack: vertical
            multiVideo = multiVideo.replace("hstack", "vstack");
        }
        multiVideo = String.format(multiVideo, input1, input2, outputPath);
        return multiVideo.split(" ");
    }

    /**
     * reverse video
     *
     * @param inputPath  input file
     * @param outputPath output file
     * @return reverse success or not
     */
    public static String[] reverseVideo(String inputPath, String outputPath) {
        //-vf reverse: only video reverse, -an: disable audio
        //tip: reverse will cost a lot of time, only short video are recommended
        String reverseVideo = "ffmpeg -i %s -vf reverse -an %s";
        reverseVideo = String.format(reverseVideo, inputPath, outputPath);
        return reverseVideo.split(" ");
    }

    /**
     * noise reduction with video
     *
     * @param inputPath  input file
     * @param outputPath output file
     * @return noise reduction success or not
     */
    public static String[] denoiseVideo(String inputPath, String outputPath) {
        String denoiseVideo = "ffmpeg -i %s -nr 500 %s";
        denoiseVideo = String.format(denoiseVideo, inputPath, outputPath);
        return denoiseVideo.split(" ");
    }

    /**
     * covert video into a series of pictures
     *
     * @param inputPath  input file
     * @param startTime  startTime in the video
     * @param duration   duration, how long you want
     * @param frameRate  frameRate
     * @param outputPath output file
     * @return convert success or not
     */
    public static String[] videoToImage(String inputPath, int startTime, int duration, int frameRate, String outputPath) {
        return videoToImageWithScale(inputPath, startTime, duration, frameRate, 0, outputPath);
    }

    public static String[] videoToImageWithScale(String inputPath, int startTime, int duration,
                                                 int frameRate, int width, String outputPath) {
        String toImage;
        if (width > 0) {
            toImage = "ffmpeg -ss %d -accurate_seek -t %d -i %s -an -vf fps=%d,scale=%d:-1 %s";
            toImage = String.format(Locale.CHINESE, toImage, startTime, duration, inputPath, frameRate, width, outputPath);
        } else {
            toImage = "ffmpeg -ss %d -accurate_seek -t %d -i %s -an -r %d %s";
            toImage = String.format(Locale.CHINESE, toImage, startTime, duration, inputPath, frameRate, outputPath);
        }
        toImage = toImage + "%3d.png";
        return toImage.split(" ");
    }

    /**
     * convert videos into picture-in-picture mode
     *
     * @param inputFile1 input one
     * @param inputFile2 input two
     * @param outputPath output file
     * @param x          x coordinate point
     * @param y          y coordinate point
     * @return convert success or not
     */
    public static String[] picInPicVideo(String inputFile1, String inputFile2, int x, int y, String outputPath) {
        String pipVideo = "ffmpeg -i %s -i %s -filter_complex overlay=%d:%d %s";
        pipVideo = String.format(Locale.getDefault(), pipVideo, inputFile1, inputFile2, x, y, outputPath);
        return pipVideo.split(" ");
    }

    /**
     * move moov box in the front of mdat box, when moox box is behind mdat box(only mp4)
     *
     * @param inputPath  inputFile
     * @param outputPath outputFile
     * @return move success or not
     */
    public static String[] moveMoovAhead(String inputPath, String outputPath) {
        String moovCmd = "ffmpeg -i %s -movflags faststart -acodec copy -vcodec copy %s";
        moovCmd = String.format(Locale.getDefault(), moovCmd, inputPath, outputPath);
        return moovCmd.split(" ");
    }

    /**
     * using FFprobe to parse the media format
     *
     * @param inputPath  inputFile
     * @return probe success or not
     */
    public static String[] probeFormat(String inputPath) {
        //show format:ffprobe -i %s -show_format -print_format json
        //show stream:ffprobe -i %s -show_streams
        String ffprobeCmd = "ffprobe -i %s -show_streams -show_format -print_format json";
        ffprobeCmd = String.format(Locale.getDefault(), ffprobeCmd, inputPath);
        return ffprobeCmd.split(" ");
    }

    /**
     * Changing the speed of playing, speed range at 0.5-2 in audio-video mode.
     * However, in pure video mode, the speed range at 0.25-4
     * @param inputPath  the inputFile of normal speed
     * @param outputPath the outputFile which you want to change speed
     * @param speed      speed of playing
     * @param pureVideo  whether pure video or not, default false
     * @return change speed success or not
     */
    public static String[] changeSpeed(String inputPath, String outputPath, float speed, boolean pureVideo) {
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
            speedCmd = String.format(Locale.getDefault(), speedCmd, inputPath, ptsFactor, outputPath);
        } else {
            speedCmd = "ffmpeg -i %s -filter_complex [0:v]setpts=%.2f*PTS[v];[0:a]atempo=%.2f[a] -map [v] -map [a] %s";
            speedCmd = String.format(Locale.getDefault(), speedCmd, inputPath, ptsFactor, speed, outputPath);
        }
        return speedCmd.split(" ");
    }

    /**
     * Changing the speed of playing, speed range at 0.5-2 in audio.
     * @param inputPath the inputFile of normal speed
     * @param outputPath the outputFile which you want to change speed
     * @param speed speed of playing
     * @return change speed success or not
     */
    public static String[] changeAudioSpeed(String inputPath, String outputPath, float speed) {
        if (speed > 2 || speed < 0.5) {
            throw new IllegalArgumentException("audio speed range is from 0.5 to 2");
        }
        String speedCmd = "ffmpeg -i %s -filter_complex atempo=%.2f %s";
        speedCmd = String.format(Locale.getDefault(), speedCmd, inputPath, speed, outputPath);
        return speedCmd.split(" ");
    }

    /**
     * Rebuild the keyframe index of FLV, make it seekable
     * @param inputPath inputFile
     * @param outputPath targetFile
     * @return command of building flv index
     */
    public static String[] buildFlvIndex(String inputPath, String outputPath) {
        String buildIndex = "ffmpeg -i %s -flvflags add_keyframe_index %s";
        buildIndex = String.format(buildIndex, inputPath, outputPath);
        return buildIndex.split(" ");
    }

    /**
     * Insert the picture into the header of video, which as a thumbnail
     * @param inputPath inputFile
     * @param picturePath the path of thumbnail
     * @param outputPath targetFile
     * @return command of inserting picture
     */
    public static String[] insertPicIntoVideo(String inputPath, String picturePath, String outputPath) {
        String buildIndex = "ffmpeg -i %s -i %s -map 0 -map 1 -c copy -c:v:1 png -disposition:v:1 attached_pic %s";
        buildIndex = String.format(buildIndex, inputPath, picturePath, outputPath);
        return buildIndex.split(" ");
    }

    /**
     * Using one input file to push multi streams.
     * After publish the streams, you could use VLC to play it
     * Note: if stream is rtmp protocol, need to start your rtmp server
     * Note: if stream is http protocol, need to start your http server
     * @param inputPath inputFile
     * @param duration how long of inputFile you want to publish
     * @param streamUrl1 the url of stream1
     * @param streamUrl2 the url of stream2
     * @return command of build flv index
     */
    public static String[] pushMultiStreams(String inputPath, int duration, String streamUrl1, String streamUrl2) {
        //ffmpeg -i what.mp4 -vcodec libx264 -acodec aac -t 60 -f flv
        //"tee:rtmp://192.168.1.102/live/stream1|rtmp://192.168.1.102/live/stream2"
        String format = "flv";
        if (streamUrl1.startsWith("rtmp://")) {//rtmp protocol
            format = "flv";
        } else if (streamUrl1.startsWith("http://")) {//http protocol
            format = "mpegts";
        }
        String pushStreams = "ffmpeg -i %s -vcodec libx264 -acodec aac -t %d -f %s \"tee:%s|%s\"";
        pushStreams = String.format(Locale.getDefault(), pushStreams, inputPath, duration, format, streamUrl1, streamUrl2);
        return pushStreams.split(" ");
    }

}
