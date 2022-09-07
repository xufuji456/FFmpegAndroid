package com.frank.ffmpeg;

/**
 * @author xufulong
 * @date 2022/9/7 10:16 上午
 * @desc
 */
public class CommonMediaHelper {

    static {
        System.loadLibrary("media-handle");
    }

    public native int audioResample(String inputFile, String outputFile, int sampleRate);

}
