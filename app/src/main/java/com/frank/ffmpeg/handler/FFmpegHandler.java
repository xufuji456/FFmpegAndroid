package com.frank.ffmpeg.handler;

import android.os.Handler;
import android.util.Log;

import com.frank.ffmpeg.FFmpegCmd;
import com.frank.ffmpeg.listener.OnHandleListener;
import com.frank.ffmpeg.model.MediaBean;
import com.frank.ffmpeg.tool.JsonParseTool;

/**
 * Handler Message processing Device
 * Created by frank on 2019/11/11.
 */
public class FFmpegHandler {

    private final static String TAG = FFmpegHandler.class.getSimpleName();

    public final static int MSG_BEGIN = 9012;

    public final static int MSG_FINISH = 1112;

    public final static int MSG_CONTINUE = 2012;

    public final static int MSG_TOAST = 4562;

    private Handler mHandler;

    private boolean isContinue = false;

    public FFmpegHandler(Handler mHandler) {
        this.mHandler = mHandler;
    }

    public void isContinue(boolean isContinue) {
        this.isContinue = isContinue;
    }

    /**
     *  carried out ffmpeg Command Line
     * @param commandLine commandLine
     */
    public void executeFFmpegCmd(final String[] commandLine) {
        if(commandLine == null) {
            return;
        }
        FFmpegCmd.execute(commandLine, new OnHandleListener() {
            @Override
            public void onBegin() {
                Log.i(TAG, "handle onBegin...");
                mHandler.obtainMessage(MSG_BEGIN).sendToTarget();
            }

            @Override
            public void onEnd(int resultCode, String resultMsg) {
                Log.i(TAG, "handle onEnd...");
                if(isContinue) {
                    mHandler.obtainMessage(MSG_CONTINUE).sendToTarget();
                }else {
                    mHandler.obtainMessage(MSG_FINISH).sendToTarget();
                }
            }
        });
    }

    /**
     * execute probe cmd
     * @param commandLine commandLine
     */
    public void executeFFprobeCmd(final String[] commandLine) {
        if(commandLine == null) {
            return;
        }
        FFmpegCmd.executeProbe(commandLine, new OnHandleListener() {
            @Override
            public void onBegin() {
                Log.i(TAG, "handle ffprobe onBegin...");
                mHandler.obtainMessage(MSG_BEGIN).sendToTarget();
            }

            @Override
            public void onEnd(int resultCode, String resultMsg) {
                Log.i(TAG, "handle ffprobe onEnd result=" + resultMsg);
                MediaBean mediaBean = null;
                if(resultMsg != null && !resultMsg.isEmpty()) {
                    mediaBean = JsonParseTool.parseMediaFormat(resultMsg);
                }
                mHandler.obtainMessage(MSG_FINISH, mediaBean).sendToTarget();
            }
        });
    }

}
