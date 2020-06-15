package com.frank.ffmpeg.listener;

/**
 * listener of FFmpeg processing
 * Created by frank on 2019/11/11.
 */
public interface OnHandleListener {
    void onBegin();
    default void onProgress(int progress, int duration){}
    void onEnd(int resultCode, String resultMsg);
}
