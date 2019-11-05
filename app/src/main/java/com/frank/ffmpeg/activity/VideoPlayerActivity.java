package com.frank.ffmpeg.activity;

import android.os.Bundle;
import android.util.Log;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.View;

import com.frank.ffmpeg.R;
import com.frank.ffmpeg.VideoPlayer;
import com.frank.ffmpeg.util.FileUtil;

/**
 * 使用ffmpeg播放视频
 * Created by frank on 2018/2/1.
 */
public class VideoPlayerActivity extends BaseActivity implements SurfaceHolder.Callback {
    private static final String TAG = VideoPlayerActivity.class.getSimpleName();
    SurfaceHolder surfaceHolder;
    private VideoPlayer videoPlayer;
    //播放倍率
    private float playRate = 1;

    private boolean surfaceCreated;

    @Override
    int getLayoutId() {
        return R.layout.activity_video_player;
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        initView();
        initPlayer();
        showToast(getString(R.string.please_click_select));
    }

    private void initView() {
        SurfaceView surfaceView = getView(R.id.surface_view);
        surfaceHolder = surfaceView.getHolder();
        surfaceHolder.addCallback(this);

        initViewsWithClick(R.id.btn_slow);
        initViewsWithClick(R.id.btn_fast);
    }

    private void initPlayer(){
        videoPlayer = new VideoPlayer();
    }

    @Override
    public void surfaceCreated(SurfaceHolder holder) {
        surfaceCreated = true;
    }

    @Override
    public void surfaceChanged(SurfaceHolder holder, int format, int width, int height) {

    }

    @Override
    public void surfaceDestroyed(SurfaceHolder holder) {

    }

    private void startPlay(final String filePath) {
        new Thread(new Runnable() {
            @Override
            public void run() {
                if (!FileUtil.checkFileExist(filePath)) {
                    return;
                }

                videoPlayer.play(filePath, surfaceHolder.getSurface());
            }
        }).start();
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        if(videoPlayer != null){
            videoPlayer = null;
        }
    }

    @Override
    void onViewClick(View view) {
        switch (view.getId()) {
            case R.id.btn_fast:
                if(playRate >= 1/32){
                    playRate *= 0.5;
                }
                Log.i(TAG, "fast playRate=" + playRate);
                videoPlayer.setPlayRate(playRate);
                break;
            case R.id.btn_slow:
                if(playRate <= 32){
                    playRate *= 2;
                }
                Log.i(TAG, "slow playRate=" + playRate);
                videoPlayer.setPlayRate(playRate);
                break;
            default:
                break;
        }
    }

    @Override
    void onSelectedFile(String filePath) {
        if (surfaceCreated) {
            startPlay(filePath);
        }
    }

}
