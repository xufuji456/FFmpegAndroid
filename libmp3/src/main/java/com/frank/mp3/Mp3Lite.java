package com.frank.mp3;

/**
 * Mp3Lite: use AudioTrack and OpenSL ES to play audio
 * Created by frank on 2018/2/1.
 */

public class Mp3Lite {
    static {
        System.loadLibrary("mp3-lite");
    }

    public native static void lameInitDefault();

    public native static void lameInit(int inSamplerate, int outChannel,
                                       int outSamplerate, int outBitrate, float scaleInput, int mode, int vbrMode,
                                       int quality, int vbrQuality, int abrMeanBitrate, int lowpassFreq, int highpassFreq, String id3tagTitle,
                                       String id3tagArtist, String id3tagAlbum, String id3tagYear,
                                       String id3tagComment);

    public native static int lameEncode(short[] buffer_l, short[] buffer_r,
                                        int samples, byte[] mp3buf);

    public native static int encodeBufferInterleaved(short[] pcm, int samples,
                                                     byte[] mp3buf);

    public native static int lameFlush(byte[] mp3buf);

    public native static void lameClose();

}
