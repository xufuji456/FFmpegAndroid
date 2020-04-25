package com.frank.live.param;


/**
 * Audio param Entity
 * Created by frank on 2018/1/28.
 */

public class AudioParam {
    private int channelConfig;
    private int sampleRate;
    private int audioFormat;
    private int numChannels;

    public AudioParam(int sampleRate, int channelConfig, int audioFormat, int numChannels) {
        this.sampleRate = sampleRate;
        this.channelConfig = channelConfig;
        this.audioFormat = audioFormat;
        this.numChannels = numChannels;
    }

    public int getChannelConfig() {
        return channelConfig;
    }

    public void setChannelConfig(int channelConfig) {
        this.channelConfig = channelConfig;
    }

    public int getSampleRate() {
        return sampleRate;
    }

    public void setSampleRate(int sampleRate) {
        this.sampleRate = sampleRate;
    }

    public int getAudioFormat() {
        return audioFormat;
    }

    public void setAudioFormat(int audioFormat) {
        this.audioFormat = audioFormat;
    }

    public int getNumChannels() {
        return numChannels;
    }

    public void setNumChannels(int numChannels) {
        this.numChannels = numChannels;
    }
}
