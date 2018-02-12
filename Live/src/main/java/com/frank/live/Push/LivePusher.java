package com.frank.live.Push;

import android.util.Log;
import android.view.SurfaceHolder;
import com.frank.live.LiveUtil;
import com.frank.live.listener.LiveStateChangeListener;
import com.frank.live.param.AudioParam;
import com.frank.live.param.VideoParam;

/**
 * 音视频推流
 * Created by frank on 2018/1/28.
 */

public class LivePusher {

    private VideoPusher videoPusher;
    private AudioPusher audioPusher;
    private LiveUtil liveUtil;

    public LivePusher(SurfaceHolder surfaceHolder, VideoParam videoParam, AudioParam audioParam){
        liveUtil = new LiveUtil();
        videoPusher = new VideoPusher(surfaceHolder, videoParam, liveUtil);
        audioPusher = new AudioPusher(audioParam, liveUtil);
    }

    /**
     * 开始推流
     */
    public void startPush(String liveUrl, LiveStateChangeListener liveStateChangeListener){
        videoPusher.startPush();
        audioPusher.startPush();
        liveUtil.setOnLiveStateChangeListener(liveStateChangeListener);
        int result = liveUtil.startPush(liveUrl);
        Log.i("LivePusher", "startPush=" + (result == 0 ? "success" : "fail"));
    }

    /**
     * 停止推流
     */
    public void stopPush(){
        videoPusher.stopPush();
        audioPusher.stopPush();
        liveUtil.stopPush();
    }

    /**
     * 切换摄像头
     */
    public void switchCamera(){
        videoPusher.switchCamera();
    }

    /**
     * 释放资源
     */
    public void release(){
        videoPusher.release();
        audioPusher.release();
        liveUtil.release();
    }

    /**
     * 设置静音
     * @param isMute 是否静音
     */
    public void setMute(boolean isMute){
        audioPusher.setMute(isMute);
    }

}
