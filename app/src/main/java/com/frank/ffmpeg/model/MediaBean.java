package com.frank.ffmpeg.model;

/**
 * the model of media data
 * Created by frank on 2020/1/7.
 */
public class MediaBean {

    private VideoBean videoBean;

    private AudioBean audioBean;

    // "duration": "313.330000"
    private long duration;

    // "size": "22160429"
    private long size;

    // "bit_rate": "565804"
    private int bitRate;

    // "format_name": "mov,mp4,m4a,3gp,3g2,mj2"
    private String formatName;

    // "nb_streams": 2
    private int streamNum;

    public VideoBean getVideoBean() {
        return videoBean;
    }

    public void setVideoBean(VideoBean videoBean) {
        this.videoBean = videoBean;
    }

    public AudioBean getAudioBean() {
        return audioBean;
    }

    public void setAudioBean(AudioBean audioBean) {
        this.audioBean = audioBean;
    }

    public long getDuration() {
        return duration;
    }

    public void setDuration(long duration) {
        this.duration = duration;
    }

    public long getSize() {
        return size;
    }

    public void setSize(long size) {
        this.size = size;
    }

    public int getBitRate() {
        return bitRate;
    }

    public void setBitRate(int bitRate) {
        this.bitRate = bitRate;
    }

    public String getFormatName() {
        return formatName;
    }

    public void setFormatName(String formatName) {
        this.formatName = formatName;
    }

    public int getStreamNum() {
        return streamNum;
    }

    public void setStreamNum(int streamNum) {
        this.streamNum = streamNum;
    }
}
