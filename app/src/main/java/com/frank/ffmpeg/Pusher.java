package com.frank.ffmpeg;

/**
 * 使用FFmepg进行推流直播
 * Created by frank on 2018/2/2.
 */

public class Pusher {
    static {
        System.loadLibrary("media-handle");
    }

    //选择本地文件推流到指定平台直播
    public native int pushStream(String filePath, String liveUrl);

}
