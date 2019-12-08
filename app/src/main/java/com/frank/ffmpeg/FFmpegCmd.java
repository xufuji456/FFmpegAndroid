package com.frank.ffmpeg;

import android.text.TextUtils;

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

    /**
     * 使用FastStart把Moov移动到Mdat前面
     * @param inputFile inputFile
     * @param outputFile outputFile
     * @return 是否操作成功
     */
    public int moveMoovAhead(String inputFile, String outputFile) {
        if (TextUtils.isEmpty(inputFile) || TextUtils.isEmpty(outputFile)) {
            return -1;
        }
        return fastStart(inputFile, outputFile);
    }

    private native static int handle(String[] commands);

    private native static int fastStart(String inputFile, String outputFile);

}