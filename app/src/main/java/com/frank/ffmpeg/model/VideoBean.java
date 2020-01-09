package com.frank.ffmpeg.model;

/**
 * the model of video data
 * Created by frank on 2020/1/7.
 */
public class VideoBean {

    //"codec_tag_string": "avc1"
    private String videoCodec;

    //"width": 640
    private int width;

    //"height": 360
    private int height;

    //"display_aspect_ratio": "16:9"
    private String displayAspectRatio;

    //"pix_fmt": "yuv420p"
    private String pixelFormat;

    //"profile": "578"
    private String profile;

    //"level": 30
    private int level;

    //"r_frame_rate": "24000/1001"
    private int frameRate;

    public String getVideoCodec() {
        if ("[0][0][0][0]".equals(videoCodec)) {
            return null;
        }
        return videoCodec;
    }

    public void setVideoCodec(String videoCodec) {
        this.videoCodec = videoCodec;
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

    public String getDisplayAspectRatio() {
        return displayAspectRatio;
    }

    public void setDisplayAspectRatio(String displayAspectRatio) {
        this.displayAspectRatio = displayAspectRatio;
    }

    public String getPixelFormat() {
        return pixelFormat;
    }

    public void setPixelFormat(String pixelFormat) {
        this.pixelFormat = pixelFormat;
    }

    public String getProfile() {
        return profile;
    }

    public void setProfile(String profile) {
        this.profile = profile;
    }

    public int getLevel() {
        return level;
    }

    public void setLevel(int level) {
        this.level = level;
    }

    public int getFrameRate() {
        return frameRate;
    }

    public void setFrameRate(int frameRate) {
        this.frameRate = frameRate;
    }

}
