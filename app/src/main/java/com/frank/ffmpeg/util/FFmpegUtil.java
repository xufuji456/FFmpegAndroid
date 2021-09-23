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
    public static String[] cutAudio(String inputPath, float startTime, float duration, String outputPath) {
        String cutAudioCmd = "ffmpeg -i %s -ss %f -t %f -acodec copy -vn %s";
        cutAudioCmd = String.format(Locale.getDefault(), cutAudioCmd, inputPath, startTime, duration, outputPath);
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
     * mix multiple audio inputs into a single output
     *
     */
    public static String[] mixAudio(String inputPath, String mixPath, String outputPath) {
        //mixing formula: value = sample1 + sample2 - ((sample1 * sample2) >> 0x10)
        return mixAudio(inputPath, mixPath, 0, 0, false, outputPath);
    }

    public static String[] mixAudio(String inputPath, String mixPath, int weight1, int weight2,
                                    boolean disableThumb, String outputPath) {
        //duration: first shortest longest
        //weight: adjust weight(volume) of each audio stream
        //disableThumb:(-vn)if not disable, it may cause incorrect mixing
        int len = 8;
        String amix = "amix=inputs=2:duration=longest";
        if (disableThumb) len += 1;
        String[] mixAudioCmd = new String[len];
        mixAudioCmd[0] = "ffmpeg";
        mixAudioCmd[1] = "-i";
        mixAudioCmd[2] = inputPath;
        mixAudioCmd[3] = "-i";
        mixAudioCmd[4] = mixPath;
        mixAudioCmd[5] = "-filter_complex";
        if (weight1 > 0 && weight2 > 0) {
            String weight = ":weights=" + "'" + weight1 + " " + weight2 + "'";
            amix += weight;
        }
        mixAudioCmd[6] = amix;
        if (disableThumb) mixAudioCmd[7] = "-vn";
        mixAudioCmd[len-1] = outputPath;
        return mixAudioCmd;
    }

    /**
     * merge multiple audio streams into a single multi-channel stream
     *
     */
    public static String[] mergeAudio(String inputPath, String mergePath, String outputPath) {
        String mergeCmd = "ffmpeg -i %s -i %s -filter_complex [0:a][1:a]amerge=inputs=2[aout] -map [aout] %s";
        mergeCmd = String.format(mergeCmd, inputPath, mergePath, outputPath);
        return mergeCmd.split(" ");
    }

    /**
     * Set echo and delay effect
     *
     * @param inputPath  input file
     * @param delay      delay to play
     * @param outputPath output file
     */
    public static String[] audioEcho(String inputPath, int delay, String outputPath) {
        // in_gain (0, 1], Default is 0.6
        // out_gain (0, 1], Default is 0.3
        // delays (0 - 90000]. Default is 1000
        // decays (0 - 1.0]. Default is 0.5
        String echoCmd = "ffmpeg -i %s -af aecho=0.8:0.8:%d:0.5 %s";
        echoCmd = String.format(Locale.getDefault(), echoCmd, inputPath, delay, outputPath);
        return echoCmd.split(" ");
    }

    /**
     * Set tremolo effect, sinusoidal amplitude modulation
     *
     * @param inputPath  input file
     * @param frequency  frequency
     * @param depth      depth
     * @param outputPath output file
     */
    public static String[] audioTremolo(String inputPath, int frequency, float depth, String outputPath) {
        // frequency [0.1, 20000.0], Default is 5
        // depth (0, 1], Default is 0.5
        String tremoloCmd = "ffmpeg -i %s -af tremolo=%d:%f %s";
        tremoloCmd = String.format(Locale.getDefault(), tremoloCmd, inputPath, frequency, depth, outputPath);
        return tremoloCmd.split(" ");
    }

    /**
     * Denoise audio samples with FFT
     *
     * @param inputPath  input file
     * @param outputPath output file
     */
    public static String[] audioDenoise(String inputPath, String outputPath) {
        // nr: noise reduction in dB, [0.01 to 97], Default value is 12 dB
        // nf: noise floor in dB, [-80 to -20],  Default value is -50 dB
        // nt: noise type {w:white noise v:vinyl noise s:shellac noise}
        String fftDenoiseCmd = "ffmpeg -i %s -af afftdn %s";
        fftDenoiseCmd = String.format(Locale.getDefault(), fftDenoiseCmd, inputPath, outputPath);
        return fftDenoiseCmd.split(" ");
    }

    /**
     * Detect silence of a chunk of audio
     *
     * @param inputPath  input file
     */
    public static String[] audioSilenceDetect(String inputPath) {
        // silence_start: 268.978
        // silence_end: 271.048 | silence_duration: 2.06975
        String silenceCmd = "ffmpeg -i %s -af silencedetect=noise=0.0001 -f null -";
        silenceCmd = String.format(Locale.getDefault(), silenceCmd, inputPath);
        return silenceCmd.split(" ");
    }

    /**
     * Change volume of a chunk of audio
     *
     * @param inputPath  input file
     * @param volume     volume
     * @param outputPath output file
     */
    public static String[] audioVolume(String inputPath, float volume, String outputPath) {
        // output_volume = volume * input_volume
        String volumeCmd = "ffmpeg -i %s -af volume=%f %s";
        volumeCmd = String.format(Locale.getDefault(), volumeCmd, inputPath, volume, outputPath);
        return volumeCmd.split(" ");
    }

    /**
     * Apply 18 band of equalizer into audio
     *
     * @param inputPath  input file
     * @param bandList   bandList
     * @param outputPath output file
     */
    public static String[] audioEqualizer(String inputPath, List<String> bandList, String outputPath) {
        // unit: Hz  gain:0-20
        /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
         |   1b   |   2b   |   3b   |   4b   |   5b   |   6b   |   7b   |   8b   |   9b   |
         |   65   |   92   |   131  |   185  |   262  |   370  |   523  |   740  |  1047  |
         |- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
         |   10b  |   11b  |   12b  |   13b  |   14b  |   15b  |   16b  |   17b  |   18b  |
         |   1480 |   2093 |   2960 |   4186 |   5920 |   8372 |  11840 |  16744 |  20000 |
         |- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
        StringBuilder builder = new StringBuilder();
        for (String band:bandList) {
            builder.append(band).append(":");
        }
        builder.deleteCharAt(builder.length()-1);
        String equalizerCmd = "ffmpeg -i %s -af superequalizer=%s -y %s";
        equalizerCmd = String.format(Locale.getDefault(), equalizerCmd, inputPath, builder.toString(), outputPath);
        return equalizerCmd.split(" ");
    }

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
        String mediaMuxCmd;
        if (copy) {
            mediaMuxCmd = "ffmpeg -i %s -i %s -codec copy -y %s";
        } else {
            mediaMuxCmd = "ffmpeg -i %s -i %s -y %s";
        }
        mediaMuxCmd = String.format(Locale.getDefault(), mediaMuxCmd, videoFile, audioFile, muxFile);
        return mediaMuxCmd.split(" ");
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
        String extractAudioCmd = "ffmpeg -i %s -acodec copy -vn %s";
        extractAudioCmd = String.format(extractAudioCmd, inputPath, outputPath);
        return extractAudioCmd.split(" ");
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
        String extractVideoCmd = "ffmpeg -i %s -vcodec copy -an %s";
        extractVideoCmd = String.format(extractVideoCmd, inputPath, outputPath);
        return extractVideoCmd.split(" ");
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
    public static String[] cutVideo(String inputPath, float startTime, float duration, String outputPath) {
        // -map 0 -codec copy (copy all tracks)
        // -map 0:v -vcodec copy (copy track of video)
        // -map 0:a -acodec copy (copy all tracks of audio)
        // -map 0:s -scodec copy (copy all tracks of subtitle)
        String cutVideoCmd = "ffmpeg -ss %f -accurate_seek -t %f -i %s -map 0 -codec copy -avoid_negative_ts 1 %s";
        cutVideoCmd = String.format(Locale.getDefault(), cutVideoCmd, startTime, duration, inputPath, outputPath);
        return cutVideoCmd.split(" ");
    }

    /**
     * screenshot from video, you could assign the specific time
     *
     * @param inputPath  input file
     * @param offset     which time you want to shot
     * @param outputPath output file
     * @return screenshot success or not
     */
    public static String[] screenShot(String inputPath, float offset, String outputPath) {
        String screenShotCmd = "ffmpeg -ss %f -i %s -f image2 -vframes 1 -an %s";
        screenShotCmd = String.format(Locale.getDefault(), screenShotCmd, offset, inputPath, outputPath);
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
        String overlay = obtainOverlay(offsetXY, offsetXY, location);
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
        int offset = ScreenUtil.INSTANCE.dp2px(FFmpegApplication.getInstance(), offsetXY);
        String overlay = obtainOverlay(offset, offset, location) + ":shortest=1";
        String waterMarkCmd = "ffmpeg -i %s -ignore_loop 0 -i %s -b:v %s -filter_complex %s -preset:v superfast %s";
        waterMarkCmd = String.format(waterMarkCmd, inputPath, imgPath, mBitRate, overlay, outputPath);
        return waterMarkCmd.split(" ");
    }

    /**
     * Remove watermark from video: Suppress logo by a simple interpolation of the surrounding pixels.
     * Just set a rectangle covering the logo and watch it disappear
     *
     * @return delogo cmd
     */
    public static String[] removeLogo(String inputPath, int x, int y, int width, int height, String outputPath) {
        String delogoCmd = "ffmpeg -i %s -filter_complex delogo=x=%d:y=%d:w=%d:h=%d %s";
        delogoCmd = String.format(Locale.getDefault(), delogoCmd, inputPath, x, y, width, height, outputPath);
        return delogoCmd.split(" ");
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
        String encodeAudioCmd = "ffmpeg -f s16le -ar %d -ac %d -i %s %s";
        encodeAudioCmd = String.format(Locale.getDefault(), encodeAudioCmd, sampleRate, channel, inputPath, outputPath);
        return encodeAudioCmd.split(" ");
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
        // atempo range [0.5, 100.0]
        if (speed > 100 || speed < 0.5) {
            throw new IllegalArgumentException("audio speed range is from 0.5 to 100");
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
        String insertPicCmd = "ffmpeg -i %s -i %s -map 0 -map 1 -c copy -c:v:1 png -disposition:v:1 attached_pic %s";
        insertPicCmd = String.format(insertPicCmd, inputPath, picturePath, outputPath);
        return insertPicCmd.split(" ");
    }

    public static String[] addSubtitleIntoVideo(String inputPath, String subtitlePath, String outputPath) {
        String subtitleCmd = "ffmpeg -i %s -i %s -map 0:v -map 0:a -map 1:s -c copy %s";
        subtitleCmd = String.format(subtitleCmd, inputPath, subtitlePath, outputPath);
        return subtitleCmd.split(" ");
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

    public static String[] rotateVideo(String inputPath, int rotateDegree, String outputPath) {
        String rotateCmd = "ffmpeg -i %s -c copy -metadata:s:v:0 rotate=%d %s";
        rotateCmd = String.format(Locale.getDefault(), rotateCmd, inputPath, rotateDegree, outputPath);
        return rotateCmd.split(" ");
    }

    public static String[] changeGOP(String inputPath, int gop, String outputPath) {
        String rotateCmd = "ffmpeg -i %s -g %d %s";
        rotateCmd = String.format(Locale.getDefault(), rotateCmd, inputPath, gop, outputPath);
        return rotateCmd.split(" ");
    }

}
