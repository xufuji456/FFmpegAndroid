package com.frank.ffmpeg.activity;

import android.annotation.SuppressLint;
import android.media.MediaPlayer;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.text.TextUtils;
import android.util.Log;
import android.view.Surface;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.View;

import com.frank.ffmpeg.R;
import com.frank.ffmpeg.view.VideoPreviewBar;

import java.io.IOException;

import static com.frank.ffmpeg.handler.FFmpegHandler.MSG_TOAST;

/**
 * Preview the thumbnail of video when seeking
 * Created by frank on 2019/11/16.
 */

public class VideoPreviewActivity extends BaseActivity implements VideoPreviewBar.PreviewBarCallback {

    private final static String TAG = VideoPreviewActivity.class.getSimpleName();

    private MediaPlayer mediaPlayer;
    private SurfaceView surfaceVideo;
    private VideoPreviewBar videoPreviewBar;
    private final static int TIME_UPDATE = 1000;
    private final static int MSG_UPDATE = 1234;

    @SuppressLint("HandlerLeak")
    private Handler mHandler = new Handler() {
        @Override
        public void handleMessage(Message msg) {
            super.handleMessage(msg);
            if (msg.what == MSG_UPDATE) {
                if (videoPreviewBar != null && mediaPlayer != null) {
                    videoPreviewBar.updateProgress(mediaPlayer.getCurrentPosition());
                }
                mHandler.sendEmptyMessageDelayed(MSG_UPDATE, TIME_UPDATE);
            } else if (msg.what == MSG_TOAST) {
                showToast(getString(R.string.please_click_select));
            }
        }
    };

    @Override
    int getLayoutId() {
        return R.layout.activity_preview;
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        initView();
        mHandler.sendEmptyMessageDelayed(MSG_TOAST, 500);
    }

    private void initView() {
        surfaceVideo = getView(R.id.surface_view);
        videoPreviewBar = getView(R.id.preview_video);
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
                Log.i(TAG, "onPrepared...");
                mediaPlayer.start();
                mHandler.sendEmptyMessage(MSG_UPDATE);
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

    @Override
    void onViewClick(View view) {

    }

    @Override
    void onSelectedFile(String filePath) {
        setPlayCallback(filePath, surfaceVideo);
        videoPreviewBar.init(filePath, this);
    }

    @Override
    public void onStopTracking(long progress) {
        if (mediaPlayer != null) {
            Log.i(TAG, "onStopTracking progress=" + progress);
            mediaPlayer.seekTo((int) progress);
        }
    }

    private void releasePlayer() {
        if (mediaPlayer != null) {
            mediaPlayer.stop();
            mediaPlayer.release();
            mediaPlayer = null;
        }
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        releasePlayer();
        if (videoPreviewBar != null) {
            videoPreviewBar.release();
        }
    }

}
