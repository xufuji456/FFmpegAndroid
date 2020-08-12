package com.frank.ffmpeg.handler;

import android.os.Handler;
import android.util.Log;

import com.frank.ffmpeg.FFmpegCmd;
import com.frank.ffmpeg.listener.OnHandleListener;
import com.frank.ffmpeg.model.MediaBean;
import com.frank.ffmpeg.tool.JsonParseTool;

import java.util.List;

/**
 * Handler of FFmpeg and FFprobe
 * Created by frank on 2019/11/11.
 */
public class FFmpegHandler {

    private final static String TAG = FFmpegHandler.class.getSimpleName();

    public final static int MSG_BEGIN = 9012;

    public final static int MSG_PROGRESS = 1002;

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
     * execute the command of FFmpeg
     *
     * @param commandLine commandLine
     */
    public void executeFFmpegCmd(final String[] commandLine) {
        if (commandLine == null) {
            return;
        }
        FFmpegCmd.execute(commandLine, new OnHandleListener() {
            @Override
            public void onBegin() {
                Log.i(TAG, "handle onBegin...");
                mHandler.obtainMessage(MSG_BEGIN).sendToTarget();
            }

            @Override
            public void onProgress(int progress, int duration) {
                mHandler.obtainMessage(MSG_PROGRESS, progress, duration).sendToTarget();
            }

            @Override
            public void onEnd(int resultCode, String resultMsg) {
                Log.i(TAG, "handle onEnd...");
                if (isContinue) {
                    mHandler.obtainMessage(MSG_CONTINUE).sendToTarget();
                } else {
                    mHandler.obtainMessage(MSG_FINISH).sendToTarget();
                }
            }
        });
    }

    /**
     * cancel the running task, and exit quietly
     * @param cancel cancel the task when flag is true
     */
    public void cancelExecute(boolean cancel) {
        FFmpegCmd.cancelTask(cancel);
    }

    /**
     * execute multi commands of FFmpeg
     *
     * @param commandList the list of command
     */
    public void executeFFmpegCmds(final List<String[]> commandList) {
        if (commandList == null) {
            return;
        }
        FFmpegCmd.execute(commandList, new OnHandleListener() {
            @Override
            public void onBegin() {
                Log.i(TAG, "handle onBegin...");
                mHandler.obtainMessage(MSG_BEGIN).sendToTarget();
            }

            @Override
            public void onProgress(int progress, int duration) {
                mHandler.obtainMessage(MSG_PROGRESS, progress, duration).sendToTarget();
            }

            @Override
            public void onEnd(int resultCode, String resultMsg) {
                Log.i(TAG, "handle onEnd...");
                if (isContinue) {
                    mHandler.obtainMessage(MSG_CONTINUE).sendToTarget();
                } else {
                    mHandler.obtainMessage(MSG_FINISH).sendToTarget();
                }
            }
        });
    }

    /**
     * execute the command of FFprobe
     *
     * @param commandLine commandLine
     */
    public void executeFFprobeCmd(final String[] commandLine) {
        if (commandLine == null) {
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
                if (resultMsg != null && !resultMsg.isEmpty()) {
                    mediaBean = JsonParseTool.parseMediaFormat(resultMsg);
                }
                mHandler.obtainMessage(MSG_FINISH, mediaBean).sendToTarget();
            }
        });
    }

}
