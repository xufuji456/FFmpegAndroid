package com.frank.ffmpeg.activity;

import android.media.MediaPlayer;
import android.os.Environment;
import android.os.Bundle;
import android.text.TextUtils;
import android.util.Log;
import android.view.Surface;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.View;
import android.widget.SeekBar;

import com.frank.ffmpeg.R;
import com.frank.ffmpeg.hardware.HardwareDecode;

import java.io.IOException;

/**
 * 视频拖动实时预览
 * Created by frank on 2019/11/16.
 */

public class VideoPreviewActivity extends BaseActivity implements HardwareDecode.OnDataCallback {

    private final static String TAG = VideoPreviewActivity.class.getSimpleName();

    private final static String ROOT_PATH = Environment.getExternalStorageDirectory().getAbsolutePath();
    private String videoPath = ROOT_PATH + "/What.mp4";
//    private final static String videoPath = ROOT_PATH + "/bird-1080P.mkv";
//        videoPath = "https://www.apple.com/105/media/cn/mac/family/2018/46c4b917_abfd_45a3_9b51_4e3054191797" +
//                "/films/bruce/mac-bruce-tpl-cn-2018_1280x720h.mp4";

    private SeekBar previewBar;
    private  HardwareDecode hardwareDecode;
    private long duration;

    private MediaPlayer mediaPlayer;
    private SurfaceView surfaceVideo;
    private SurfaceView surfacePreView;

    @Override
    int getLayoutId() {
        return R.layout.activity_preview;
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        initView();
        setListener();
    }

    private void initView() {
        previewBar = getView(R.id.preview_bar);

        surfaceVideo = getView(R.id.surface_view);
        setPlayCallback(videoPath, surfaceVideo);

        surfacePreView = getView(R.id.surface_preview);
        setPreviewCallback(videoPath, surfacePreView);
    }

    private void setPlayCallback(final String filePath, SurfaceView surfaceView) {
        surfaceView.getHolder().addCallback(new SurfaceHolder.Callback() {
            @Override
            public void surfaceCreated(SurfaceHolder holder) {
                doPlay(filePath, holder.getSurface());
            }

            @Override
            public void surfaceChanged(SurfaceHolder holder, int format, int width, int height) {

            }

            @Override
            public void surfaceDestroyed(SurfaceHolder holder) {

            }
        });
    }

    private void setPrepareListener() {
        if (mediaPlayer == null) {
            return;
        }
        mediaPlayer.setOnPreparedListener(new MediaPlayer.OnPreparedListener() {
            @Override
            public void onPrepared(MediaPlayer mp) {
                Log.e(TAG, "onPrepared...");
                mediaPlayer.start();
            }
        });
    }

    private void doPlay(String filePath, Surface surface) {
        if (surface == null || TextUtils.isEmpty(filePath)) {
            return;
        }
        releasePlayer();
        try {
            mediaPlayer = new MediaPlayer();
            setPrepareListener();
            mediaPlayer.setDataSource(filePath);
            mediaPlayer.setSurface(surface);
            mediaPlayer.prepareAsync();
        } catch (IOException e) {
            e.printStackTrace();
        }
    }

    private void setPreviewCallback(final String filePath, SurfaceView surfaceView) {
        surfaceView.getHolder().addCallback(new SurfaceHolder.Callback() {
            @Override
            public void surfaceCreated(SurfaceHolder holder) {
                doPreview(filePath, holder.getSurface());
            }

            @Override
            public void surfaceChanged(SurfaceHolder holder, int format, int width, int height) {

            }

            @Override
            public void surfaceDestroyed(SurfaceHolder holder) {

            }
        });
    }

    private void doPreview(String filePath, Surface surface) {
        if (surface == null || TextUtils.isEmpty(filePath)) {
            return;
        }
        releasePreviewer();
        hardwareDecode = new HardwareDecode(surface, filePath, VideoPreviewActivity.this);
        hardwareDecode.decode();
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

    @Override
    void onViewClick(View view) {

    }

    @Override
    void onSelectedFile(String filePath) {
//        doPlay(filePath, surfaceVideo.getHolder().getSurface());
//        doPreview(filePath, surfacePreView.getHolder().getSurface());
        setPlayCallback(filePath, surfaceVideo);
        setPreviewCallback(filePath, surfacePreView);
    }

    private void releasePlayer() {
        if (mediaPlayer != null) {
            mediaPlayer.stop();
            mediaPlayer.release();
            mediaPlayer = null;
        }
    }

    private void releasePreviewer() {
        if (hardwareDecode != null) {
            hardwareDecode.release();
            hardwareDecode = null;
        }
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        releasePreviewer();
        releasePlayer();
    }

}
