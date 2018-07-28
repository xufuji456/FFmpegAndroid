package com.frank.ffmpeg;

import android.media.AudioFormat;
import android.media.AudioManager;
import android.media.AudioTrack;

/**
 * 视频播放器
 * Created by frank on 2018/2/1
 */
public class VideoPlayer {

    static {
        System.loadLibrary("media-handle");
    }

    public native int play(String filePath, Object surface);
    public native int setPlayRate(float playRate);

    public native int filter(String filePath, Object surface, String filterType);
    public native void again();
    public native void release();
    public native void playAudio(boolean play);

    /**
     * 创建AudioTrack对象，供JNI调用
     * @param sampleRate sampleRate
     * @param channels channels
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
