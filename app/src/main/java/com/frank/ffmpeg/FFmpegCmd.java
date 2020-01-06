package com.frank.ffmpeg;

import android.text.TextUtils;

import com.frank.ffmpeg.listener.OnHandleListener;

public class FFmpegCmd {

    static{
        System.loadLibrary("media-handle");
    }

    private final static int RESULT_SUCCESS = 1;

    private final static int RESULT_ERROR = 0;

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
                    onHandleListener.onEnd(result, null);
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

    /**
     * execute probe cmd internal
     * @param commands commands
     * @param onHandleListener onHandleListener
     */
    public static void executeProbe(final String[] commands, final OnHandleListener onHandleListener) {
        new Thread(new Runnable() {
            @Override
            public void run() {
                if(onHandleListener != null) {
                    onHandleListener.onBegin();
                }
                //execute ffprobe
                String result = handleProbe(commands);
                int resultCode = !TextUtils.isEmpty(result) ? RESULT_SUCCESS : RESULT_ERROR;
                if(onHandleListener != null) {
                    onHandleListener.onEnd(resultCode, result);
                }
            }
        }).start();
    }

    private native static int handle(String[] commands);

    private native static int fastStart(String inputFile, String outputFile);

    private native static String handleProbe(String[] commands);

}