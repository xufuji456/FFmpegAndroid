package com.frank.ffmpeg.listener;

/**
 * 流程 carried out 监听Device
 * Created by frank on 2019/11/11.
 */
public interface OnHandleListener {
    void onBegin();
    void onEnd(int resultCode, String resultMsg);
}
