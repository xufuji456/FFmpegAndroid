package com.frank.ffmpeg.activity;

import android.os.Bundle;
import android.util.Log;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.View;
import android.widget.Button;

import com.frank.ffmpeg.MediaPlayer;
import com.frank.ffmpeg.R;
import com.frank.ffmpeg.util.FileUtil;

/**
 * mediaPlayer, which decode by software
 * Created by frank on 2018/2/12.
 */

public class MediaPlayerActivity extends BaseActivity implements SurfaceHolder.Callback {

    private static final String TAG = MediaPlayerActivity.class.getSimpleName();

    private SurfaceHolder surfaceHolder;

    private MediaPlayer mediaPlayer;

    private boolean surfaceCreated;

    private Button btnSelectFile;

    @Override
    int getLayoutId() {
        return R.layout.activity_media_player;
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        hideActionBar();
        initView();
        initPlayer();
    }

    private void initView(){
        SurfaceView surfaceView = getView(R.id.surface_media);
        surfaceHolder = surfaceView.getHolder();
        surfaceHolder.addCallback(this);
        btnSelectFile = getView(R.id.btn_select_file);
        initViewsWithClick(R.id.btn_select_file);
    }

    private void initPlayer(){
        mediaPlayer = new MediaPlayer();
    }

    @Override
    public void surfaceCreated(SurfaceHolder holder) {
        surfaceCreated = true;
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

    private void startPlay(final String filePath) {
        new Thread(new Runnable() {
            @Override
            public void run() {
                int result = mediaPlayer.setup(filePath, surfaceHolder.getSurface());
                if(result < 0){
                    Log.e(TAG, "mediaPlayer setup error!");
                    return;
                }
                mediaPlayer.play();
            }
        }).start();
    }

    @Override
    void onSelectedFile(String filePath) {
        if (!FileUtil.checkFileExist(filePath)){
            return;
        }
        if (surfaceCreated) {
            btnSelectFile.setVisibility(View.GONE);
            startPlay(filePath);
        }
    }

    @Override
    void onViewClick(View view) {
        if (view.getId() == R.id.btn_select_file) {
            selectFile();
        }
    }
}
