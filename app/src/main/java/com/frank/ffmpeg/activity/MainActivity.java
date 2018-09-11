package com.frank.ffmpeg.activity;

import android.Manifest;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.os.Build;
import android.os.Bundle;
import android.support.v7.app.AppCompatActivity;
import android.view.View;
import com.frank.ffmpeg.R;

/**
 * 使用ffmpeg进行音视频处理入口
 * Created by frank on 2018/1/23.
 */
public class MainActivity extends AppCompatActivity implements View.OnClickListener{

    private final static String[] mPermissions = new String[]{
            Manifest.permission.WRITE_EXTERNAL_STORAGE,
            Manifest.permission.READ_EXTERNAL_STORAGE
    };
    private final static int CODE_STORAGE = 999;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        initView();
        checkPermission();
    }

    private void initView() {
        findViewById(R.id.btn_audio).setOnClickListener(this);
        findViewById(R.id.btn_video).setOnClickListener(this);
        findViewById(R.id.btn_media).setOnClickListener(this);
        findViewById(R.id.btn_play).setOnClickListener(this);
        findViewById(R.id.btn_push).setOnClickListener(this);
        findViewById(R.id.btn_live).setOnClickListener(this);
        findViewById(R.id.btn_filter).setOnClickListener(this);
        findViewById(R.id.btn_reverse).setOnClickListener(this);
    }

    @Override
    public void onClick(View v) {
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
            default:
                break;
        }
        startActivity(intent);
    }

    //动态申请权限
    private void checkPermission(){
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
            if (checkSelfPermission(mPermissions[0]) != PackageManager.PERMISSION_GRANTED){
                requestPermissions(mPermissions, CODE_STORAGE);
            }
        }
    }

}
