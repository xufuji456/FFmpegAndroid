package com.frank.ffmpeg.activity;

import android.content.Intent;
import android.os.Bundle;
import android.view.View;

import com.frank.ffmpeg.R;

/**
 * The main entrance of all Activity
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

        initViewsWithClick(
                R.id.btn_audio,
                R.id.btn_video,
                R.id.btn_media,
                R.id.btn_play,
                R.id.btn_push,
                R.id.btn_live,
                R.id.btn_filter,
                R.id.btn_preview,
                R.id.btn_probe,
                R.id.btn_audio_effect
        );
    }

    @Override
    public void onViewClick(View v) {
        Intent intent = new Intent();
        switch (v.getId()) {
            case R.id.btn_audio://handle audio
                intent.setClass(MainActivity.this, AudioHandleActivity.class);
                break;
            case R.id.btn_video://handle video
                intent.setClass(MainActivity.this, VideoHandleActivity.class);
                break;
            case R.id.btn_media://handle media
                intent.setClass(MainActivity.this, MediaHandleActivity.class);
                break;
            case R.id.btn_play://media play
                intent.setClass(MainActivity.this, MediaPlayerActivity.class);
                break;
            case R.id.btn_push://pushing
                intent.setClass(MainActivity.this, PushActivity.class);
                break;
            case R.id.btn_live://realtime living with rtmp stream
                intent.setClass(MainActivity.this, LiveActivity.class);
                break;
            case R.id.btn_filter://filter effect
                intent.setClass(MainActivity.this, FilterActivity.class);
                break;
            case R.id.btn_preview://preview thumbnail
                intent.setClass(MainActivity.this, VideoPreviewActivity.class);
                break;
            case R.id.btn_probe://probe media format
                intent.setClass(MainActivity.this, ProbeFormatActivity.class);
                break;
            case R.id.btn_audio_effect://audio effect
                intent.setClass(MainActivity.this, AudioEffectActivity.class);
                break;
            default:
                break;
        }
        startActivity(intent);
    }

    @Override
    void onSelectedFile(String filePath) {

    }

}
