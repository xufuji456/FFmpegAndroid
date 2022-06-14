package com.frank.ffmpeg.util;

import com.frank.ffmpeg.FFmpegApplication;
import com.frank.ffmpeg.model.VideoLayout;

import java.util.List;
import java.util.Locale;

/**
 * ffmpeg tool: assemble the complete command
 * Created by frank on 2018/1/23.
 */

public class FFmpegUtil {

    private static String[] insert(String[] cmd, int position, String inputPath) {
        return insert(cmd, position, inputPath, null);
    }

    /**
     * insert inputPath and outputPath into target array
     */
    private static String[] insert(String[] cmd, int position, String inputPath, String outputPath) {
        if (cmd == null || inputPath == null || position < 2) {
            return cmd;
        }
        int len = (outputPath != null ? (cmd.length + 2) : (cmd.length + 1));
        String[] result = new String[len];
        System.arraycopy(cmd, 0, result, 0, position);
        result[position] = inputPath;
        System.arraycopy(cmd, position, result, position + 1, cmd.length - position);
        if (outputPath != null) {
            result[result.length - 1] = outputPath;
        }
        return result;
    }

    public static String[] insert(String[] cmd, int position1, String inputPath1,
                                  int position2, String inputPath2, String outputPath) {
        if (cmd == null || inputPath1 == null || position1 < 2 || inputPath2 == null || position2 < 4) {
            return cmd;
        }
        int len = (outputPath != null ? (cmd.length + 3) : (cmd.length + 2));
        String[] result = new String[len];
        System.arraycopy(cmd, 0, result, 0, position1);
        result[position1] = inputPath1;
        System.arraycopy(cmd, position1, result, position1 + 1, position2 - position1 - 1);
        result[position2] = inputPath2;
        System.arraycopy(cmd, position2 - 1, result, position2 + 1, cmd.length - (position2 - 1));
        if (outputPath != null) {
            result[result.length - 1] = outputPath;
        }
        return result;
    }

    /**
     * transform audio, according to your assigning the output format
     *
     * @param inputPath  input file
     * @param outputPath output file
     * @return transform success or not
     */
    public static String[] transformAudio(String inputPath, String outputPath) {
        String[] result = new String[4];
        result[0] = "ffmpeg";
        result[1] = "-i";
        result[2] = inputPath;
        result[3] = outputPath;
        return result;
    }

    public static String[] transformAudio(String inputPath, String acodec, String outputPath) {
        String transformAudioCmd = "ffmpeg -i -acodec %s -ac 2 -ar 44100";
        transformAudioCmd = String.format(transformAudioCmd, acodec);
        return insert(transformAudioCmd.split(" "), 2, inputPath, outputPath);
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
        String cutAudioCmd = "ffmpeg -i -ss %f -t %f -vn";
        cutAudioCmd = String.format(Locale.getDefault(), cutAudioCmd, startTime, duration);
        return insert(cutAudioCmd.split(" "), 2, inputPath, outputPath);
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
        String concatAudioCmd = "ffmpeg -i -acodec copy"; // "ffmpeg -i -acodec libmp3lame"
        return insert(concatAudioCmd.split(" "), 2, concatStr, outputPath);
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
        String echoCmd = "ffmpeg -i -af aecho=0.8:0.8:%d:0.5";
        echoCmd = String.format(Locale.getDefault(), echoCmd, delay);
        return insert(echoCmd.split(" "), 2, inputPath, outputPath);
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
        String tremoloCmd = "ffmpeg -i -af tremolo=%d:%f";
        tremoloCmd = String.format(Locale.getDefault(), tremoloCmd, frequency, depth);
        return insert(tremoloCmd.split(" "), 2, inputPath, outputPath);
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
        String fftDenoiseCmd = "ffmpeg -i -af afftdn";
        return insert(fftDenoiseCmd.split(" "), 2, inputPath, outputPath);
    }

    /**
     * Detect silence of a chunk of audio
     *
     * @param inputPath  input file
     */
    public static String[] audioSilenceDetect(String inputPath) {
        // silence_start: 268.978
        // silence_end: 271.048 | silence_duration: 2.06975
        String silenceCmd = "ffmpeg -i -af silencedetect=noise=0.0001 -f null -";
        return insert(silenceCmd.split(" "), 2, inputPath);
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
        String volumeCmd = "ffmpeg -i -af volume=%f";
        volumeCmd = String.format(Locale.getDefault(), volumeCmd, volume);
        return insert(volumeCmd.split(" "), 2, inputPath, outputPath);
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
        builder.deleteCharAt(builder.length() - 1);
        // "ffmpeg -i %s -af superequalizer=%s -y %s"
        String equalizerCmd = "ffmpeg -i -af superequalizer=%s -y";
        equalizerCmd = String.format(Locale.getDefault(), equalizerCmd, builder.toString());
        return insert(equalizerCmd.split(" "), 2, inputPath, outputPath);
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
        // -vn: disable video
        // multi audio track: ffmpeg -i input.mp4 -map 0:1 -vn output.mp3
        String extractAudioCmd = "ffmpeg -i -vn";
        return insert(extractAudioCmd.split(" "), 2, inputPath, outputPath);
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
        String extractVideoCmd = "ffmpeg -i -vcodec copy -an";
        return insert(extractVideoCmd.split(" "), 2, inputPath, outputPath);
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
//        ffmpeg -i %s -vcodec libx264 -acodec libmp3lame %s
        String transformVideoCmd = "ffmpeg -i -vcodec libx264 -acodec libmp3lame";
        return insert(transformVideoCmd.split(" "), 2, inputPath, outputPath);
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
            transformVideoCmd = "ffmpeg -i -vcodec libx264 -acodec aac " + scale;
        } else {
            transformVideoCmd = "ffmpeg -i -vcodec libx264 -acodec aac";
        }
        return insert(transformVideoCmd.split(" "), 2, inputPath, outputPath);
    }

    /**
     * joint every single video together
     * @param fileListPath the path file list
     * @param outputPath   output path
     * @return joint video success or not
     */
    public static String[] jointVideo(String fileListPath, String outputPath) {
        // ffmpeg -f concat -safe 0 -i %s -c copy %s
        String jointVideoCmd = "ffmpeg -f concat -safe 0 -i file.txt -c copy %s";
        return insert(jointVideoCmd.split(" "), 6, fileListPath, outputPath);
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
        // ffmpeg -ss %f -accurate_seek -t %f -i %s -map 0 -codec copy -avoid_negative_ts 1 %s
        String cutVideoCmd = "ffmpeg -ss %f -accurate_seek -t %f -i -map 0 -codec copy -avoid_negative_ts 1";
        cutVideoCmd = String.format(Locale.getDefault(), cutVideoCmd, startTime, duration);
        return insert(cutVideoCmd.split(" "), 7, inputPath, outputPath);
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
        // ffmpeg -ss %f -i %s -f image2 -vframes 1 -an %s
        String screenShotCmd = "ffmpeg -ss %f -i -f image2 -vframes 1 -an";
        screenShotCmd = String.format(Locale.getDefault(), screenShotCmd, offset);
        return insert(screenShotCmd.split(" "), 4, inputPath, outputPath);
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
        String waterMarkCmd = "ffmpeg -i -i -b:v %s -filter_complex %s -preset:v superfast";
        waterMarkCmd = String.format(waterMarkCmd, mBitRate, overlay);
        return insert(waterMarkCmd.split(" "), 2, inputPath, 4, imgPath, outputPath);
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
        String waterMarkCmd = "ffmpeg -i -ignore_loop 0 -i -b:v %s -filter_complex %s -preset:v superfast";
        waterMarkCmd = String.format(waterMarkCmd, mBitRate, overlay);
        return insert(waterMarkCmd.split(" "), 2, inputPath, 6, imgPath, outputPath);
    }

    /**
     * Remove watermark from video: Suppress logo by a simple interpolation of the surrounding pixels.
     * On the other hand, it can be used to mosaic video
     *
     * @return delogo cmd
     */
    public static String[] removeLogo(String inputPath, int x, int y, int width, int height, String outputPath) {
        // ffmpeg -i in.mp4 -filter_complex delogo=x=%d:y=%d:w=%d:h=%d out.mp4
        String delogoCmd = "ffmpeg -i -filter_complex delogo=x=%d:y=%d:w=%d:h=%d";
        delogoCmd = String.format(Locale.getDefault(), delogoCmd, x, y, width, height);
        return insert(delogoCmd.split(" "), 2, inputPath, outputPath);
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
        String paletteCmd = "ffmpeg -ss %d -accurate_seek -t %d -i -vf fps=%d,scale=%d:-1:flags=lanczos,palettegen";
        paletteCmd = String.format(Locale.getDefault(), paletteCmd, startTime,
                duration, frameRate, width);
        return insert(paletteCmd.split(" "), 7, inputPath, outputPath);
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
        String paletteGifCmd = "ffmpeg -ss %d -accurate_seek -t %d -i -i -lavfi fps=%d,scale=%d:-1:flags=lanczos[x];[x][1:v]" +
                "paletteuse=dither=bayer:bayer_scale=3";
        paletteGifCmd = String.format(Locale.getDefault(), paletteGifCmd, startTime,
                                      duration, frameRate, width);
        return insert(paletteGifCmd.split(" "), 7, inputPath, 9, palette, outputPath);
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
        String convertCmd = "ffmpeg -i -s %s -pix_fmt yuv420p";
        convertCmd = String.format(Locale.getDefault(), convertCmd, resolution);
        return insert(convertCmd.split(" "), 2, inputPath, outputPath);
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
        String encodeAudioCmd = "ffmpeg -f s16le -ar %d -ac %d -i";
        encodeAudioCmd = String.format(Locale.getDefault(), encodeAudioCmd, sampleRate, channel);
        return insert(encodeAudioCmd.split(" "), 8, inputPath, outputPath);
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
        String multiVideo = "ffmpeg -i -i -filter_complex hstack";//hstack: horizontal
        if (videoLayout == VideoLayout.LAYOUT_VERTICAL) {//vstack: vertical
            multiVideo = multiVideo.replace("hstack", "vstack");
        }
        return insert(multiVideo.split(" "), 2, input1, 4, input2, outputPath);
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
        String reverseVideo = "ffmpeg -i -vf reverse -an";
        return insert(reverseVideo.split(" "), 2, inputPath, outputPath);
    }

    /**
     * noise reduction with video
     *
     * @param inputPath  input file
     * @param outputPath output file
     * @return noise reduction success or not
     */
    public static String[] denoiseVideo(String inputPath, String outputPath) {
        String denoiseVideo = "ffmpeg -i -nr 500";
        return insert(denoiseVideo.split(" "), 2, inputPath, outputPath);
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
     * @param inputPath1 input one
     * @param inputPath2 input two
     * @param outputPath output file
     * @param x          x coordinate point
     * @param y          y coordinate point
     * @return convert success or not
     */
    public static String[] picInPicVideo(String inputPath1, String inputPath2, int x, int y, String outputPath) {
        String pipVideo = "ffmpeg -i -i -filter_complex overlay=%d:%d";
        pipVideo = String.format(Locale.getDefault(), pipVideo, x, y);
        return insert(pipVideo.split(" "), 2, inputPath1, 4, inputPath2, outputPath);
    }

    /**
     * move moov box in the front of mdat box, when moox box is behind mdat box(only mp4)
     *
     * @param inputPath  inputFile
     * @param outputPath outputFile
     * @return move success or not
     */
    public static String[] moveMoovAhead(String inputPath, String outputPath) {
        String moovCmd = "ffmpeg -i -movflags faststart -acodec copy -vcodec copy";
        return insert(moovCmd.split(" "), 2, inputPath, outputPath);
    }

    /**
     * Change Video from RGB to gray(black & white)
     * @param inputPath inputPath
     * @param outputPath outputPath
     * @return grayCmd
     */
    public static String[] toGrayVideo(String inputPath, String outputPath) {
        String grayCmd = "ffmpeg -i -vf lutyuv='u=128:v=128'";
        return insert(grayCmd.split(" "), 2, inputPath, outputPath);
    }

    /**
     * Photo zoom to video
     * @param inputPath inputPath
     * @param outputPath outputPath
     * @return zoomCmd
     */
    public static String[] photoZoomToVideo(String inputPath, String outputPath) {
        String zoomCmd = "ffmpeg -loop 1 -i -vf zoompan=z='if(lte(zoom,1.0),1.5,max(1.001,zoom-0.0015))':d=125 -t 8";
        return insert(zoomCmd.split(" "), 4, inputPath, outputPath);
    }

    /**
     * using FFprobe to parse the media format
     *
     * @param inputPath  inputFile
     * @return probe success or not
     */
    public static String[] probeFormat(String inputPath) {
        // ffprobe -i hello.mp4 -show_streams -show_format -print_format json"
        String ffprobeCmd = "ffprobe -i -show_streams -show_format -print_format json";
        return insert(ffprobeCmd.split(" "), 2, inputPath);
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
            speedCmd = "ffmpeg -i -filter_complex [0:v]setpts=%.2f*PTS[v] -map [v]";
            speedCmd = String.format(Locale.getDefault(), speedCmd, ptsFactor);
        } else {
            speedCmd = "ffmpeg -i -filter_complex [0:v]setpts=%.2f*PTS[v];[0:a]atempo=%.2f[a] -map [v] -map [a]";
            speedCmd = String.format(Locale.getDefault(), speedCmd, ptsFactor, speed);
        }
        return insert(speedCmd.split(" "), 2, inputPath, outputPath);
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
        String speedCmd = "ffmpeg -i -filter_complex atempo=%.2f";
        speedCmd = String.format(Locale.getDefault(), speedCmd, speed);
        return insert(speedCmd.split(" "), 2, inputPath, outputPath);
    }

    /**
     * Rebuild the keyframe index of FLV, make it seekable
     * @param inputPath inputFile
     * @param outputPath targetFile
     * @return command of building flv index
     */
    public static String[] buildFlvIndex(String inputPath, String outputPath) {
        String buildIndex = "ffmpeg -i -flvflags add_keyframe_index";
        return insert(buildIndex.split(" "), 2, inputPath, outputPath);
    }

    /**
     * Insert the picture into the header of video, which as a thumbnail
     * @param inputPath inputFile
     * @param picturePath the path of thumbnail
     * @param outputPath targetFile
     * @return command of inserting picture
     */
    public static String[] insertPicIntoVideo(String inputPath, String picturePath, String outputPath) {
        String insertPicCmd = "ffmpeg -i -i -map 0 -map 1 -c copy -c:v:1 png -disposition:v:1 attached_pic";
        return insert(insertPicCmd.split(" "), 2, inputPath, 4, picturePath, outputPath);
    }

    public static String[] addSubtitleIntoVideo(String inputPath, String subtitlePath, String outputPath) {
        String subtitleCmd = "ffmpeg -i -i -map 0:v -map 0:a -map 1:s -c copy";
        return insert(subtitleCmd.split(" "), 2, inputPath, 4, subtitlePath, outputPath);
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
        String rotateCmd = "ffmpeg -i -c copy -metadata:s:v:0 rotate=%d";
        rotateCmd = String.format(Locale.getDefault(), rotateCmd, rotateDegree);
        return insert(rotateCmd.split(" "), 2, inputPath, outputPath);
    }

    public static String[] changeGOP(String inputPath, int gop, String outputPath) {
        String gopCmd = "ffmpeg -i -g %d";
        gopCmd = String.format(Locale.getDefault(), gopCmd, gop);
        return insert(gopCmd.split(" "), 2, inputPath, outputPath);
    }

    /**
     * Trim one or more segments from a single video
     * @param inputPath  the path of input file
     * @param start1     start time of the first segment
     * @param end1       end time of the first segment
     * @param start2     start time of the second segment
     * @param end2       end time of the second segment
     * @param outputPath the path of output file
     * @return the command of trim video
     */
    public static String[] trimVideo(String inputPath, int start1, int end1, int start2, int end2, String outputPath) {
        String trimCmd = "ffmpeg -i -filter_complex " +
                "[0:v]trim=start=%d:end=%d,setpts=PTS-STARTPTS[v0];" +
                "[0:a]atrim=start=%d:end=%d,asetpts=PTS-STARTPTS[a0];" +
                "[0:v]trim=start=%d:end=%d,setpts=PTS-STARTPTS[v1];" +
                "[0:a]atrim=start=%d:end=%d,asetpts=PTS-STARTPTS[a1];" +
                "[v0][a0][v1][a1]concat=n=2:v=1:a=1[out] -map [out]";
        trimCmd = String.format(Locale.getDefault(), trimCmd, start1, end1, start1, end1, start2, end2, start2, end2);
        return insert(trimCmd.split(" "), 2, inputPath, outputPath);
    }

    public static String[] showAudioWaveform(String inputPath, String resolution, String outputPath) {
        String waveformCmd = "ffmpeg -i -filter_complex showwavespic=s=%s";
        waveformCmd = String.format(Locale.getDefault(), waveformCmd, resolution);
        return insert(waveformCmd.split(" "), 2, inputPath, outputPath);
    }

    // ffmpeg -i beyond.mp4 -vf stereo3d=sbsl:arbg -y left3d.mp4
}
