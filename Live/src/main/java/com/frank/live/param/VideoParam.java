package com.frank.live.param;

/**
 * Video param Entity
 * Created by frank on 2018/1/28.
 */

public class VideoParam {
    private int width;
    private int height;
    private int cameraId;
    private int bitRate;
    private int frameRate;

    public VideoParam(int width, int height, int cameraId, int bitRate, int frameRate) {
        this.width = width;
        this.height = height;
        this.cameraId = cameraId;
        this.bitRate = bitRate;
        this.frameRate = frameRate;
    }

    public int getWidth() {
        return width;
    }

    public void setWidth(int width) {
        this.width = width;
    }

    public int getHeight() {
        return height;
    }

    public void setHeight(int height) {
        this.height = height;
    }

    public int getCameraId() {
        return cameraId;
    }

    public void setCameraId(int cameraId) {
        this.cameraId = cameraId;
    }

    public int getBitRate() {
        return bitRate;
    }

    public void setBitRate(int bitRate) {
        this.bitRate = bitRate;
    }

    public int getFrameRate() {
        return frameRate;
    }

    public void setFrameRate(int frameRate) {
        this.frameRate = frameRate;
    }
}
