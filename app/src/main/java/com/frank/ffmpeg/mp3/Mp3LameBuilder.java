package com.frank.ffmpeg.mp3;

public class Mp3LameBuilder {


    public enum Mode {
        STEREO, JSTEREO, MONO, DEFAULT
    }

    public enum VbrMode {
        VBR_OFF, VBR_RH, VBR_MTRH, VBR_ABR, VBR_DEFAUT
    }


    int inSampleRate;
    int outSampleRate;
    int outBitrate;
    int outChannel;
    public int quality;
    int vbrQuality;
    int abrMeanBitrate;
    int lowPassFreq;
    int highPassFreq;
    float scaleInput;
    Mode mode;
    VbrMode vbrMode;

    String id3tagTitle;
    String id3tagArtist;
    String id3tagAlbum;
    String id3tagComment;
    String id3tagYear;

    Mp3LameBuilder() {

        this.id3tagTitle = null;
        this.id3tagAlbum = null;
        this.id3tagArtist = null;
        this.id3tagComment = null;
        this.id3tagYear = null;

        this.inSampleRate = 44100;
        this.outSampleRate = 0;
        this.outChannel = 2;
        this.outBitrate = 128;
        this.scaleInput = 1;

        this.quality = 5;
        this.mode = Mode.DEFAULT;
        this.vbrMode = VbrMode.VBR_OFF;
        this.vbrQuality = 5;
        this.abrMeanBitrate = 128;

        this.lowPassFreq = 0;
        this.highPassFreq = 0;
    }

    Mp3LameBuilder setQuality(int quality) {
        this.quality = quality;
        return this;
    }

    Mp3LameBuilder setInSampleRate(int inSampleRate) {
        this.inSampleRate = inSampleRate;
        return this;
    }

    Mp3LameBuilder setOutSampleRate(int outSampleRate) {
        this.outSampleRate = outSampleRate;
        return this;
    }

    Mp3LameBuilder setOutBitrate(int bitrate) {
        this.outBitrate = bitrate;
        return this;
    }

    Mp3LameBuilder setOutChannels(int channels) {
        this.outChannel = channels;
        return this;
    }

    Mp3LameBuilder setId3tagTitle(String title) {
        this.id3tagTitle = title;
        return this;
    }

    Mp3LameBuilder setId3tagArtist(String artist) {
        this.id3tagArtist = artist;
        return this;
    }

    Mp3LameBuilder setId3tagAlbum(String album) {
        this.id3tagAlbum = album;
        return this;
    }

    Mp3LameBuilder setId3tagComment(String comment) {
        this.id3tagComment = comment;
        return this;
    }

    Mp3LameBuilder setId3tagYear(String year) {
        this.id3tagYear = year;
        return this;
    }

    Mp3LameBuilder setScaleInput(float scaleAmount) {
        this.scaleInput = scaleAmount;
        return this;
    }

    Mp3LameBuilder setMode(Mode mode) {
        this.mode = mode;
        return this;
    }

    Mp3LameBuilder setVbrMode(VbrMode mode) {
        this.vbrMode = mode;
        return this;
    }

    Mp3LameBuilder setVbrQuality(int quality) {
        this.vbrQuality = quality;
        return this;
    }

    Mp3LameBuilder setAbrMeanBitrate(int bitrate) {
        this.abrMeanBitrate = bitrate;
        return this;
    }

    Mp3LameBuilder setLowpassFreqency(int freq) {
        this.lowPassFreq = freq;
        return this;
    }

    Mp3LameBuilder setHighpassFreqency(int freq) {
        this.highPassFreq = freq;
        return this;
    }

    Mp3Lame build() {
        return new Mp3Lame(this);
    }

}
