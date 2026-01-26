package com.frank.ffmpeg;

import android.app.Application;

public class FFmpegApplication extends Application {

    private static FFmpegApplication context;

    @Override
    public void onCreate() {
        super.onCreate();
        context = this;
    }

    public static FFmpegApplication getInstance() {
        return context;
    }

    public boolean enableSurfaceView() {
        return true;
    }

    public boolean useMediaCodec() {
        return false;
    }

}
