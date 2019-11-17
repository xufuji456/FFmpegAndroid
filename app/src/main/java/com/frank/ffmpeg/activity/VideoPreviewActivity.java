package com.frank.ffmpeg.activity;

import android.annotation.SuppressLint;
import android.media.MediaPlayer;
import android.os.Environment;
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

/**
 * 视频拖动实时预览
 * Created by frank on 2019/11/16.
 */

public class VideoPreviewActivity extends BaseActivity implements VideoPreviewBar.PreviewBarCallback {

    private final static String TAG = VideoPreviewActivity.class.getSimpleName();

    private final static String ROOT_PATH = Environment.getExternalStorageDirectory().getAbsolutePath();
    private String videoPath = ROOT_PATH + "/What.mp4";
//    private final static String videoPath = ROOT_PATH + "/bird-1080P.mkv";
//        videoPath = "https://www.apple.com/105/media/cn/mac/family/2018/46c4b917_abfd_45a3_9b51_4e3054191797" +
//                "/films/bruce/mac-bruce-tpl-cn-2018_1280x720h.mp4";

    private MediaPlayer mediaPlayer;
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
    }

    private void initView() {
        SurfaceView surfaceVideo = getView(R.id.surface_view);
        setPlayCallback(videoPath, surfaceVideo);
        videoPreviewBar = getView(R.id.preview_video);
        videoPreviewBar.init(videoPath, this);
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

    }

    @Override
    public void onStopTracking(long progress) {
        if (mediaPlayer != null) {
            Log.e(TAG, "onStopTracking progress=" + progress);
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
