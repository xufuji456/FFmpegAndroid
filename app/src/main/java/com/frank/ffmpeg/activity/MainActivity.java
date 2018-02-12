package com.frank.ffmpeg.activity;

import android.content.Intent;
import android.os.Bundle;
import android.support.v7.app.AppCompatActivity;
import android.view.View;
import android.widget.Toast;

import com.frank.ffmpeg.R;

/**
 * 使用ffmpeg进行音视频处理入口
 * Created by frank on 2018/1/23.
 */
public class MainActivity extends AppCompatActivity implements View.OnClickListener{

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        initView();
    }

    private void initView() {
        findViewById(R.id.btn_audio).setOnClickListener(this);
        findViewById(R.id.btn_video).setOnClickListener(this);
        findViewById(R.id.btn_media).setOnClickListener(this);
        findViewById(R.id.btn_play).setOnClickListener(this);
        findViewById(R.id.btn_push).setOnClickListener(this);
        findViewById(R.id.btn_live).setOnClickListener(this);
    }

    @Override
    public void onClick(View v) {
        switch (v.getId()){
            case R.id.btn_audio://音频处理
                startActivity(new Intent(MainActivity.this, AudioHandleActivity.class));
                break;
            case R.id.btn_video://视频处理
                startActivity(new Intent(MainActivity.this, VideoHandleActivity.class));
                break;
            case R.id.btn_media://音视频处理
                startActivity(new Intent(MainActivity.this, MediaHandleActivity.class));
                break;
            case R.id.btn_play://音视频播放
                startActivity(new Intent(MainActivity.this, MediaPlayerActivity.class));
                break;
            case R.id.btn_push://FFmpeg推流
                startActivity(new Intent(MainActivity.this, PushActivity.class));
                break;
            case R.id.btn_live://实时推流直播:AAC音频编码、H264视频编码、RTMP推流
                startActivity(new Intent(MainActivity.this, LiveActivity.class));
                break;
            default:
                break;
        }
    }

}
