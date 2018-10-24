package com.frank.ffmpeg.activity;

import android.os.Environment;
import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.util.Log;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import com.frank.ffmpeg.MediaPlayer;
import com.frank.ffmpeg.R;
import com.frank.ffmpeg.util.FileUtil;

import java.io.File;

/**
 * 音视频解码播放
 * Created by frank on 2018/2/12.
 */

public class MediaPlayerActivity extends AppCompatActivity implements SurfaceHolder.Callback {
    private static final String TAG = MediaPlayerActivity.class.getSimpleName();
    SurfaceHolder surfaceHolder;
    private final static String PATH = Environment.getExternalStorageDirectory().getPath() + File.separator;
    private String filePath = PATH + "hello.mp4";
    private MediaPlayer mediaPlayer;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_media_player);
        initView();
        initPlayer();
    }

    private void initView(){
        SurfaceView surfaceView = (SurfaceView) findViewById(R.id.surface_media);
        surfaceHolder = surfaceView.getHolder();
        surfaceHolder.addCallback(this);

//        Button btn_slow = (Button) findViewById(R.id.btn_play_slow);
//        Button btn_fast = (Button) findViewById(R.id.btn_play_fast);
    }

    private void initPlayer(){
        mediaPlayer = new MediaPlayer();
    }

    @Override
    public void surfaceCreated(SurfaceHolder holder) {
        if (!FileUtil.checkFileExist(filePath)){
            return;
        }

        new Thread(new Runnable() {
            @Override
            public void run() {
                int result = mediaPlayer.setup(filePath, surfaceHolder.getSurface());
                if(result < 0){
                    Log.e(TAG, "mediaPlayer-->setup");
                    return;
                }
                mediaPlayer.play();
            }
        }).start();
    }

    @Override
    public void surfaceChanged(SurfaceHolder holder, int format, int width, int height) {
        Log.i(TAG, "surfaceChanged");
    }

    @Override
    public void surfaceDestroyed(SurfaceHolder holder) {
        Log.i(TAG, "surfaceDestroyed");
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        if(mediaPlayer != null){
            mediaPlayer.release();
            mediaPlayer = null;
        }
    }

}
