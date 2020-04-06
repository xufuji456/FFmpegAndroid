package com.frank.ffmpeg;

import android.media.AudioFormat;
import android.media.AudioManager;
import android.media.AudioTrack;

/**
 * AudioPlayer: use AudioTrack and OpenSL ES to play audio
 * Created by frank on 2018/2/1.
 */

public class AudioPlayer {
    static {
        System.loadLibrary("media-handle");
    }

    //using AudioTrack to play
    public native void play(String audioPath);

    //using OpenSL ES to play
    public native void playAudio(String audioPath);

    public native void stop();

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

    /**
     * Create an AudioTrack instance for JNI calling
     *
     * @param sampleRate sampleRate
     * @param channels   channel layout
     * @return AudioTrack
     */
    public AudioTrack createAudioTrack(int sampleRate, int channels) {
        int audioFormat = AudioFormat.ENCODING_PCM_16BIT;
        int channelConfig;
        if (channels == 1) {
            channelConfig = AudioFormat.CHANNEL_OUT_MONO;
        } else if (channels == 2) {
            channelConfig = AudioFormat.CHANNEL_OUT_STEREO;
        } else {
            channelConfig = AudioFormat.CHANNEL_OUT_STEREO;
        }

        int bufferSizeInBytes = AudioTrack.getMinBufferSize(sampleRate, channelConfig, audioFormat);

        return new AudioTrack(AudioManager.STREAM_MUSIC, sampleRate, channelConfig, audioFormat,
                bufferSizeInBytes, AudioTrack.MODE_STREAM);
    }
}
