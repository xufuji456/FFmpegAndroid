package com.frank.ffmpeg.activity;

import android.annotation.SuppressLint;
import android.os.Environment;
import android.os.Handler;
import android.os.Message;
import android.os.Bundle;
import android.view.View;
import android.widget.LinearLayout;
import android.widget.VideoView;
import com.frank.ffmpeg.R;
import com.frank.ffmpeg.handler.FFmpegHandler;
import com.frank.ffmpeg.util.FFmpegUtil;
import com.frank.ffmpeg.util.FileUtil;
import java.io.File;

import static com.frank.ffmpeg.handler.FFmpegHandler.MSG_FINISH;
import static com.frank.ffmpeg.handler.FFmpegHandler.MSG_TOAST;

/**
 * 先处理视频反序，再视频倒播
 * Created by frank on 2018/9/12.
 */

public class VideoReverseActivity extends BaseActivity {

    private final static String ROOT_PATH = Environment.getExternalStorageDirectory().getPath();
    private String VIDEO_NORMAL_PATH = "";
    private final static String VIDEO_REVERSE_PATH = ROOT_PATH + File.separator + "reverse.mp4";

    private LinearLayout loading;
    private VideoView videoNormal;
    private VideoView videoReverse;
    private FFmpegHandler ffmpegHandler;

    @SuppressLint("HandlerLeak")
    private Handler mHandler = new Handler(){
        @Override
        public void handleMessage(Message msg) {
            super.handleMessage(msg);
            if (msg.what == MSG_FINISH){
                changeVisibility();
                startPlay();
            }else if (msg.what == MSG_TOAST) {
                showToast(getString(R.string.please_click_select));
            }
        }
    };

    @Override
    int getLayoutId() {
        return R.layout.activity_video_reverse;
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        initView();
        initPlayer();
        mHandler.sendEmptyMessageDelayed(MSG_TOAST, 1000);
        ffmpegHandler = new FFmpegHandler(mHandler);
    }

    private void initView() {
        loading = getView(R.id.layout_loading);
        videoNormal = getView(R.id.video_normal);
        videoReverse = getView(R.id.video_reverse);
        loading.setVisibility(View.GONE);
    }

    private void changeVisibility(){
        loading.setVisibility(View.GONE);
        videoNormal.setVisibility(View.VISIBLE);
        videoReverse.setVisibility(View.VISIBLE);
    }

    private void initPlayer(){
        videoNormal.setVideoPath(VIDEO_NORMAL_PATH);
        videoReverse.setVideoPath(VIDEO_REVERSE_PATH);
    }

    /**
     * 开始视频正序、反序播放
     */
    private void startPlay(){
        videoNormal.start();
        videoReverse.start();
    }

    /**
     * 视频反序处理
     */
    private void videoReverse(){
        if (!FileUtil.checkFileExist(VIDEO_NORMAL_PATH)){
            return;
        }
        String[] commandLine = FFmpegUtil.reverseVideo(VIDEO_NORMAL_PATH, VIDEO_REVERSE_PATH);
        if (ffmpegHandler != null) {
            ffmpegHandler.executeFFmpegCmd(commandLine);
        }
    }

    @Override
    void onViewClick(View view) {

    }

    @Override
    void onSelectedFile(String filePath) {
        VIDEO_NORMAL_PATH = filePath;
        loading.setVisibility(View.VISIBLE);
        videoReverse();
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        if (mHandler != null) {
            mHandler.removeCallbacksAndMessages(null);
        }
    }
}
