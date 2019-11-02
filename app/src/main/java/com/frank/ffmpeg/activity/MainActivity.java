package com.frank.ffmpeg.activity;

import android.content.Intent;
import android.os.Bundle;
import android.view.View;
import android.view.animation.BounceInterpolator;

import com.frank.ffmpeg.R;
import com.frank.ffmpeg.floating.FloatPlayerView;
import com.frank.ffmpeg.floating.FloatWindow;
import com.frank.ffmpeg.floating.MoveType;
import com.frank.ffmpeg.floating.Screen;

/**
 * 使用ffmpeg进行音视频处理入口
 * Created by frank on 2018/1/23.
 */
public class MainActivity extends BaseActivity {

    @Override
    int getLayoutId() {
        return R.layout.activity_main;
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        hideActionBar();
        initViewsWithClick(
                R.id.btn_audio,
                R.id.btn_video,
                R.id.btn_media,
                R.id.btn_play,
                R.id.btn_push,
                R.id.btn_live,
                R.id.btn_filter,
                R.id.btn_reverse,
                R.id.btn_floating
        );
    }

    @Override
    public void onViewClick(View v) {
        Intent intent = new Intent();
        switch (v.getId()){
            case R.id.btn_audio://音频处理
                intent.setClass(MainActivity.this, AudioHandleActivity.class);
                break;
            case R.id.btn_video://视频处理
                intent.setClass(MainActivity.this, VideoHandleActivity.class);
                break;
            case R.id.btn_media://音视频处理
                intent.setClass(MainActivity.this, MediaHandleActivity.class);
                break;
            case R.id.btn_play://音视频播放
                intent.setClass(MainActivity.this, MediaPlayerActivity.class);
                break;
            case R.id.btn_push://FFmpeg推流
                intent.setClass(MainActivity.this, PushActivity.class);
                break;
            case R.id.btn_live://实时推流直播:AAC音频编码、H264视频编码、RTMP推流
                intent.setClass(MainActivity.this, LiveActivity.class);
                break;
            case R.id.btn_filter://滤镜特效
                intent.setClass(MainActivity.this, FilterActivity.class);
                break;
            case R.id.btn_reverse://视频倒播
                intent.setClass(MainActivity.this, VideoReverseActivity.class);
                break;
            case R.id.btn_floating://悬浮窗播放
                floatingToPlay();
                return;
            default:
                break;
        }
        startActivity(intent);
    }

    /**
     * 悬浮窗播放
     */
    private void floatingToPlay(){
        if (FloatWindow.get() != null) {
            return;
        }
        FloatPlayerView floatPlayerView = new FloatPlayerView(getApplicationContext());
        FloatWindow
                .with(getApplicationContext())
                .setView(floatPlayerView)
                .setWidth(Screen.width, 0.4f)
                .setHeight(Screen.width, 0.4f)
                .setX(Screen.width, 0.8f)
                .setY(Screen.height, 0.3f)
                .setMoveType(MoveType.slide)
                .setFilter(false)
                .setMoveStyle(500, new BounceInterpolator())
                .build();
        FloatWindow.get().show();
    }

    @Override
    void onSelectedFile(String filePath) {

    }

}
