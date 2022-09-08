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

    private long audioContext = 0;

    private AudioTrack mAudioTrack;

    private native long native_init();

    private native void native_play(long context, String audioPath, String filter);

    private native void native_again(long context, String filterDesc);

    private native void native_release(long context);

    public void play(String audioPath, String filter) {
        audioContext = native_init();
        native_play(audioContext, audioPath, filter);
    }

    public void again(String filterDesc) {
        if (audioContext == 0) {
            return;
        }
        native_again(audioContext, filterDesc);
    }

    public void release() {
        if (audioContext == 0) {
            return;
        }
        native_release(audioContext);
        audioContext = 0;
    }


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
