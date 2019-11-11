package com.frank.ffmpeg;

import com.frank.ffmpeg.listener.OnHandleListener;

public class FFmpegCmd {

    static{
        System.loadLibrary("media-handle");
    }

    //开子线程调用native方法进行音视频处理
    public static void execute(final String[] commands, final OnHandleListener onHandleListener){
        new Thread(new Runnable() {
            @Override
            public void run() {
                if(onHandleListener != null){
                    onHandleListener.onBegin();
                }
                //调用ffmpeg进行处理
                int result = handle(commands);
                if(onHandleListener != null){
                    onHandleListener.onEnd(result);
                }
            }
        }).start();
    }
    private native static int handle(String[] commands);

}