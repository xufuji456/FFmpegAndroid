package com.frank.ffmpeg.activity;

import android.os.Bundle;
import android.os.Environment;
import android.support.v7.app.AppCompatActivity;
import android.util.Log;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.View;
import android.widget.Button;
import android.widget.Toast;

import com.frank.ffmpeg.R;
import com.frank.ffmpeg.VideoPlayer;
import com.frank.ffmpeg.util.FileUtil;

import java.io.File;

/**
 * 使用ffmpeg播放视频
 * Created by frank on 2018/2/1.
 */
public class VideoPlayerActivity extends AppCompatActivity implements SurfaceHolder.Callback {
    private static final String TAG = VideoPlayerActivity.class.getSimpleName();
    SurfaceHolder surfaceHolder;
    private final static String PATH = Environment.getExternalStorageDirectory().getPath() + File.separator;
    private String filePath = PATH + "hello.mp4";
    private VideoPlayer videoPlayer;
    //播放倍率
    private float playRate = 1;
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_video_player);

        initView();
        initPlayer();
    }

    private void initView(){
        SurfaceView surfaceView = (SurfaceView) findViewById(R.id.surface_view);
        surfaceHolder = surfaceView.getHolder();
        surfaceHolder.addCallback(this);

        Button btn_slow = (Button) findViewById(R.id.btn_slow);
        btn_slow.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                if(playRate <= 32){
                    playRate *= 2;
                }
                Log.i(TAG, "playRate=" + playRate);
                videoPlayer.setPlayRate(playRate);
            }
        });

        Button btn_fast = (Button) findViewById(R.id.btn_fast);
        btn_fast.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                if(playRate >= 1/32){
                    playRate *= 0.5;
                }
                Log.i(TAG, "playRate=" + playRate);
                videoPlayer.setPlayRate(playRate);
            }
        });

    }

    private void initPlayer(){
        videoPlayer = new VideoPlayer();
    }

    @Override
    public void surfaceCreated(SurfaceHolder holder) {
        new Thread(new Runnable() {
            @Override
            public void run() {
                if (!FileUtil.checkFileExist(filePath)){
                    return;
                }

                videoPlayer.play(filePath, surfaceHolder.getSurface());
            }
        }).start();
    }

    @Override
    public void surfaceChanged(SurfaceHolder holder, int format, int width, int height) {

    }

    @Override
    public void surfaceDestroyed(SurfaceHolder holder) {

    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        if(videoPlayer != null){
            videoPlayer = null;
        }
    }

}
