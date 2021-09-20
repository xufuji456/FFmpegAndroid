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

    private AudioTrack mAudioTrack;

    //using AudioTrack to play
    public native void play(String audioPath, String filterDesc);

    public native void again(String filterDesc);

    public native void release();

    //using OpenSL ES to play
    public native void playAudio(String audioPath);

    public native void stop();

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

        mAudioTrack = new AudioTrack(AudioManager.STREAM_MUSIC, sampleRate, channelConfig, audioFormat,
                bufferSizeInBytes, AudioTrack.MODE_STREAM);
        return mAudioTrack;
    }

    public void releaseAudioTrack() {
        if (mAudioTrack != null) {
            mAudioTrack.release();
            mAudioTrack = null;
        }
    }

    private OnFFTCallback onFFTCallback;

    public interface OnFFTCallback {
        void onFFT(byte[] data);
    }

    public void setOnFftCallback(OnFFTCallback onFFTCallback) {
        this.onFFTCallback = onFFTCallback;
    }

    public void fftCallbackFromJNI(byte[] data) {
        if (data != null && onFFTCallback != null) {
           onFFTCallback.onFFT(data);
        }
    }

}
