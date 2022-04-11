package com.frank.live.listener;

/**
 * Video/Audio frame callback
 * Created by frank on 2022/01/25.
 */

public interface OnFrameDataCallback {

    int getInputSamples();

    void onAudioFrame(byte[] pcm);

    void onAudioCodecInfo(int sampleRate, int channelCount);

    void onVideoFrame(byte[] yuv, int cameraType);

    void onVideoCodecInfo(int width, int height, int frameRate, int bitrate);
}
