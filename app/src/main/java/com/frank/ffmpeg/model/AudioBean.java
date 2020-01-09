package com.frank.ffmpeg.model;

/**
 * the model of audio data
 * Created by frank on 2020/1/7.
 */
public class AudioBean {

    //"codec_tag_string": "mp4a"
    private String audioCodec;

    //"sample_rate": "44100"
    private int sampleRate;

    //"channels": 2
    private int channels;

    //"channel_layout": "stereo"
    private String channelLayout;

    public String getAudioCodec() {
        if ("[0][0][0][0]".equals(audioCodec)) {
            return null;
        }
        return audioCodec;
    }

    public void setAudioCodec(String audioCodec) {
        this.audioCodec = audioCodec;
    }

    public int getSampleRate() {
        return sampleRate;
    }

    public void setSampleRate(int sampleRate) {
        this.sampleRate = sampleRate;
    }

    public int getChannels() {
        return channels;
    }

    public void setChannels(int channels) {
        this.channels = channels;
    }

    public String getChannelLayout() {
        return channelLayout;
    }

    public void setChannelLayout(String channelLayout) {
        this.channelLayout = channelLayout;
    }

}
