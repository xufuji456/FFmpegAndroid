package com.frank.ffmpeg.activity;

import android.annotation.SuppressLint;
import android.media.MediaMetadataRetriever;
import android.media.MediaPlayer;
import android.os.Bundle;
import android.os.Environment;
import android.os.Handler;
import android.os.Message;
import android.util.Log;
import android.view.View;
import android.widget.LinearLayout;
import android.widget.TextView;

import com.frank.ffmpeg.R;
import com.frank.ffmpeg.handler.FFmpegHandler;
import com.frank.ffmpeg.util.FFmpegUtil;
import com.frank.ffmpeg.util.FileUtil;

import java.io.File;
import java.util.Locale;

import static com.frank.ffmpeg.handler.FFmpegHandler.MSG_BEGIN;
import static com.frank.ffmpeg.handler.FFmpegHandler.MSG_CONTINUE;
import static com.frank.ffmpeg.handler.FFmpegHandler.MSG_FINISH;
import static com.frank.ffmpeg.handler.FFmpegHandler.MSG_PROGRESS;

/**
 * using ffmpeg to handle media
 * Created by frank on 2018/1/23.
 */
public class MediaHandleActivity extends BaseActivity {

    private final static String TAG = MediaHandleActivity.class.getSimpleName();
    private static final String PATH = Environment.getExternalStorageDirectory().getPath();
    private String videoFile;
    private String temp = PATH + File.separator + "temp.mp4";

    private LinearLayout layoutProgress;
    private TextView txtProgress;
    private int viewId;
    private LinearLayout layoutMediaHandle;
    private FFmpegHandler ffmpegHandler;

    @SuppressLint("HandlerLeak")
    private Handler mHandler = new Handler() {
        @Override
        public void handleMessage(Message msg) {
            super.handleMessage(msg);
            switch (msg.what) {
                case MSG_CONTINUE:
                    String audioFile = PATH + File.separator + "tiger.mp3";
                    String muxFile = PATH + File.separator + "media-mux.mp4";

                    try {
                        MediaPlayer mediaPlayer = new MediaPlayer();
                        mediaPlayer.setDataSource(videoFile);
                        mediaPlayer.prepare();
                        //ms
                        int videoDuration = mediaPlayer.getDuration() / 1000;
                        Log.i(TAG, "videoDuration=" + videoDuration);
                        mediaPlayer.release();
                        MediaMetadataRetriever mediaRetriever = new MediaMetadataRetriever();
                        mediaRetriever.setDataSource(audioFile);
                        String duration = mediaRetriever.extractMetadata(MediaMetadataRetriever.METADATA_KEY_DURATION);
                        int audioDuration = (int) (Long.parseLong(duration) / 1000);
                        Log.i(TAG, "audioDuration=" + audioDuration);
                        mediaRetriever.release();
                        int mDuration = Math.min(audioDuration, videoDuration);
                        //mux video and audio
                        String[] commandLine = FFmpegUtil.mediaMux(temp, audioFile, mDuration, muxFile);
                        if (ffmpegHandler != null) {
                            ffmpegHandler.isContinue(false);
                            ffmpegHandler.executeFFmpegCmd(commandLine);
                        }
                    } catch (Exception e) {
                        e.printStackTrace();
                    }
                    break;
                case MSG_BEGIN:
                    layoutProgress.setVisibility(View.VISIBLE);
                    layoutMediaHandle.setVisibility(View.GONE);
                    break;
                case MSG_FINISH:
                    layoutProgress.setVisibility(View.GONE);
                    layoutMediaHandle.setVisibility(View.VISIBLE);
                    break;
                case MSG_PROGRESS:
                    int progress = msg.arg1;
                    if (progress > 0) {
                        txtProgress.setVisibility(View.VISIBLE);
                        txtProgress.setText(String.format(Locale.getDefault(), "%d%%", progress));
                    } else {
                        txtProgress.setVisibility(View.INVISIBLE);
                    }
                    break;
                default:
                    break;
            }
        }
    };

    @Override
    int getLayoutId() {
        return R.layout.activity_media_handle;
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        hideActionBar();
        initView();
        ffmpegHandler = new FFmpegHandler(mHandler);
    }

    private void initView() {
        layoutProgress = getView(R.id.layout_progress);
        txtProgress = getView(R.id.txt_progress);
        layoutMediaHandle = getView(R.id.layout_media_handle);
        initViewsWithClick(
                R.id.btn_mux,
                R.id.btn_extract_audio,
                R.id.btn_extract_video
        );
    }

    @Override
    public void onViewClick(View view) {
        viewId = view.getId();
        selectFile();
    }

    @Override
    void onSelectedFile(String filePath) {
        doHandleMedia(filePath);
    }

    /**
     * Using ffmpeg cmd to handle media
     *
     * @param srcFile srcFile
     */
    private void doHandleMedia(String srcFile) {
        String[] commandLine = null;
        if (!FileUtil.checkFileExist(srcFile)) {
            return;
        }
        if (!FileUtil.isVideo(srcFile)) {
            showToast(getString(R.string.wrong_video_format));
            return;
        }

        switch (viewId) {
            case R.id.btn_mux://mux
                try {
                    videoFile = srcFile;
                    commandLine = FFmpegUtil.extractVideo(srcFile, temp);
                    if (ffmpegHandler != null) {
                        ffmpegHandler.isContinue(true);
                    }
                } catch (Exception e) {
                    e.printStackTrace();
                }
                break;
            case R.id.btn_extract_audio://extract audio
                String extractAudio = PATH + File.separator + "extractAudio.aac";
                commandLine = FFmpegUtil.extractAudio(srcFile, extractAudio);
                break;
            case R.id.btn_extract_video://extract video
                String extractVideo = PATH + File.separator + "extractVideo.mp4";
                commandLine = FFmpegUtil.extractVideo(srcFile, extractVideo);
                break;
            default:
                break;
        }
        if (ffmpegHandler != null) {
            ffmpegHandler.executeFFmpegCmd(commandLine);
        }
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        if (mHandler != null) {
            mHandler.removeCallbacksAndMessages(null);
        }
    }
}
