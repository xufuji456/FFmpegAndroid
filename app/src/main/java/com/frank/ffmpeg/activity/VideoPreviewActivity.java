package com.frank.ffmpeg.activity;

import android.Manifest;
import android.media.MediaPlayer;
import android.os.Build;
import android.os.Environment;
import android.os.Bundle;
import android.util.Log;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.widget.SeekBar;

import com.frank.ffmpeg.R;
import com.frank.ffmpeg.hardware.HardwareDecode;

import java.io.IOException;
import androidx.appcompat.app.AppCompatActivity;

/**
 * 视频拖动实时预览
 * Created by frank on 2019/11/16.
 */

public class VideoPreviewActivity extends AppCompatActivity implements HardwareDecode.OnDataCallback {

    private final static String TAG = VideoPreviewActivity.class.getSimpleName();

    private final static String ROOT_PATH = Environment.getExternalStorageDirectory().getAbsolutePath();
    private static String PATH1 = ROOT_PATH + "/What.mp4";
//    private final static String PATH2 = ROOT_PATH + "/bird-1080P.mkv";
//        PATH1 = "https://www.apple.com/105/media/cn/mac/family/2018/46c4b917_abfd_45a3_9b51_4e3054191797" +
//                "/films/bruce/mac-bruce-tpl-cn-2018_1280x720h.mp4";

    private SeekBar previewBar;
    private  HardwareDecode hardwareDecode;
    private long duration;

    private MediaPlayer mediaPlayer;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_preview);

        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
            requestPermissions(new String[]{Manifest.permission.WRITE_EXTERNAL_STORAGE}, 1234);
        }

        initView();
        setListener();
    }

    private void initView() {
        previewBar = findViewById(R.id.preview_bar);

        SurfaceView surfaceVideo = findViewById(R.id.surface_view);
        setCallback(surfaceVideo);

        SurfaceView surfaceView = findViewById(R.id.surface_preview);
        setCallbackAndPlay(PATH1, surfaceView);

//        SurfaceView surfaceViewOther = findViewById(R.id.surface_view_other);
//        setCallbackAndPlay(PATH2, surfaceViewOther);
    }

    private void setCallback(SurfaceView surfaceView) {
        mediaPlayer = new MediaPlayer();
        mediaPlayer.setOnPreparedListener(new MediaPlayer.OnPreparedListener() {
            @Override
            public void onPrepared(MediaPlayer mp) {
                Log.e(TAG, "onPrepared...");
                mediaPlayer.start();
            }
        });

        surfaceView.getHolder().addCallback(new SurfaceHolder.Callback() {
            @Override
            public void surfaceCreated(SurfaceHolder holder) {
                try {
                    mediaPlayer.setDataSource(PATH1);
                    mediaPlayer.setSurface(holder.getSurface());
                    mediaPlayer.prepareAsync();
                } catch (IOException e) {
                    e.printStackTrace();
                }
            }

            @Override
            public void surfaceChanged(SurfaceHolder holder, int format, int width, int height) {

            }

            @Override
            public void surfaceDestroyed(SurfaceHolder holder) {

            }
        });

    }

    private void setCallbackAndPlay(final String filePath, SurfaceView surfaceView) {
        surfaceView.getHolder().addCallback(new SurfaceHolder.Callback() {
            @Override
            public void surfaceCreated(SurfaceHolder holder) {

                hardwareDecode = new HardwareDecode(holder.getSurface(), filePath, VideoPreviewActivity.this);
                hardwareDecode.decode();
            }

            @Override
            public void surfaceChanged(SurfaceHolder holder, int format, int width, int height) {

            }

            @Override
            public void surfaceDestroyed(SurfaceHolder holder) {

            }
        });
    }

    private void setListener() {
        previewBar.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener() {
            @Override
            public void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser) {
                if (!fromUser) {
                    return;
                }
                previewBar.setProgress(progress);
                if (hardwareDecode != null && progress < duration) {
                    hardwareDecode.seekTo(progress);
                }
            }

            @Override
            public void onStartTrackingTouch(SeekBar seekBar) {

            }

            @Override
            public void onStopTrackingTouch(SeekBar seekBar) {
                //TODO
                if (mediaPlayer != null) {
                    Log.e(TAG, "onStop progress=" + seekBar.getProgress());
                    mediaPlayer.seekTo(seekBar.getProgress()/1000);
                }
            }
        });
    }

    @Override
    public void onData(long duration) {
        Log.e(TAG,"duration=" + duration);
        this.duration = duration;
        previewBar.setMax((int) duration);
    }

}
