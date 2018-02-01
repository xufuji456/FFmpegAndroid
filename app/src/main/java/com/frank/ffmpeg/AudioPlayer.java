package com.frank.ffmpeg;

import android.media.AudioFormat;
import android.media.AudioManager;
import android.media.AudioTrack;

/**
 * 音频播放器
 * Created by frank on 2018/2/1.
 */

public class AudioPlayer {
    static {
        System.loadLibrary("media-handle");
    }
    //调用AudioTrack播放
    public native void play(String audioPath);
    //调用OpenSL ES播放
    public native void playAudio(String audioPath);
    //调用OpenSL ES播放
    public native void stop(String audioPath);

    /**
     * 创建一个AudioTrack对象
     * @param sampleRate 采样率
     * @param channels 声道布局
     * @return AudioTrack
     */
    public AudioTrack createAudioTrack(int sampleRate, int channels){
        int audioFormat = AudioFormat.ENCODING_PCM_16BIT;
        int channelConfig;
        if(channels == 1){
            channelConfig = AudioFormat.CHANNEL_OUT_MONO;
        }else if(channels == 2){
            channelConfig = AudioFormat.CHANNEL_OUT_STEREO;
        }else{
            channelConfig = AudioFormat.CHANNEL_OUT_STEREO;
        }

        int bufferSizeInBytes = AudioTrack.getMinBufferSize(sampleRate, channelConfig, audioFormat);

        return new AudioTrack(AudioManager.STREAM_MUSIC, sampleRate, channelConfig, audioFormat,
                bufferSizeInBytes, AudioTrack.MODE_STREAM);
    }
}
