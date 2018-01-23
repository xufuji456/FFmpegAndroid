package com.frank.ffmpeg.activity;

import android.annotation.SuppressLint;
import android.media.MediaMetadataRetriever;
import android.media.MediaPlayer;
import android.os.Bundle;
import android.os.Environment;
import android.os.Handler;
import android.os.Message;
import android.support.v7.app.AppCompatActivity;
import android.util.Log;
import android.view.View;
import android.widget.Toast;

import com.frank.ffmpeg.FFmpegCmd;
import com.frank.ffmpeg.R;
import com.frank.ffmpeg.util.FFmpegUtil;

import java.io.File;

/**
 * 使用ffmpeg进行音视频合成与分离
 * Created by frank on 2018/1/23.
 */
public class MediaHandleActivity extends AppCompatActivity implements View.OnClickListener{

    private final static String TAG = MediaHandleActivity.class.getSimpleName();
    private static final String PATH = Environment.getExternalStorageDirectory().getPath();
    private String srcFile = PATH + File.separator + "hello.mp4";
    private String videoFile = PATH + File.separator + "flash-tree.mp4";//flash-tree.mp4
    String temp = PATH + File.separator + "temp.mp4";
    private boolean isMux;

    @SuppressLint("HandlerLeak")
    private Handler mHandler = new Handler(){
        @Override
        public void handleMessage(Message msg) {
            super.handleMessage(msg);
            if(msg.what == 100){
                String audioFile = PATH + File.separator + "tiger.mp3";//tiger.mp3
                String muxFile = PATH + File.separator + "media-mux.mp4";

                try {
                    //使用MediaPlayer获取视频时长
                    MediaPlayer mediaPlayer = new MediaPlayer();
                    mediaPlayer.setDataSource(videoFile);
                    mediaPlayer.prepare();
                    //单位为ms
                    int videoDuration = mediaPlayer.getDuration()/1000;
                    Log.i(TAG, "videoDuration=" + videoDuration);
                    mediaPlayer.release();
                    //使用MediaMetadataRetriever获取音频时长
                    MediaMetadataRetriever mediaRetriever = new MediaMetadataRetriever();
                    mediaRetriever.setDataSource(audioFile);
                    //单位为ms
                    String duration = mediaRetriever.extractMetadata(MediaMetadataRetriever.METADATA_KEY_DURATION);
                    int audioDuration = (int)(Long.parseLong(duration)/1000);
                    Log.i(TAG, "audioDuration=" + audioDuration);
                    mediaRetriever.release();
                    //如果视频时长比音频长，采用音频时长，否则用视频时长
                    int mDuration = Math.min(audioDuration, videoDuration);
                    //使用纯视频与音频进行合成
                    String[] commandLine = FFmpegUtil.mediaMux(temp, audioFile, mDuration, muxFile);
                    executeFFmpegCmd(commandLine);
                    isMux = false;
                } catch (Exception e) {
                    e.printStackTrace();
                }
            }
        }
    };

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_media_handle);

        initView();
    }

    private void initView() {
        findViewById(R.id.btn_mux).setOnClickListener(this);
        findViewById(R.id.btn_extract_audio).setOnClickListener(this);
        findViewById(R.id.btn_extract_video).setOnClickListener(this);
    }

    @Override
    public void onClick(View v) {
        int handleType;
        switch (v.getId()){
            case R.id.btn_mux:
                handleType = 0;
                break;
            case R.id.btn_extract_audio:
                handleType = 1;
                break;
            case R.id.btn_extract_video:
                handleType = 2;
                break;
            default:
                handleType = 0;
                break;
        }
        doHandleMedia(handleType);
    }

    /**
     * 调用ffmpeg处理音视频
     * @param handleType handleType
     */
    private void doHandleMedia(int handleType){
        String[] commandLine = null;
        switch (handleType){
            case 0://音视频合成
                try {
                    //视频文件有音频,先把纯视频文件抽取出来
                    commandLine = FFmpegUtil.extractVideo(videoFile, temp);
                    isMux = true;
                } catch (Exception e) {
                    e.printStackTrace();
                }
                break;
            case 1://提取音频
                String extractAudio = PATH + File.separator + "extractAudio.aac";
                commandLine = FFmpegUtil.extractAudio(srcFile, extractAudio);
                break;
            case 2://提取视频
                String extractVideo = PATH + File.separator + "extractVideo.mp4";
                commandLine = FFmpegUtil.extractVideo(srcFile, extractVideo);
                break;
            default:
                break;
        }
        executeFFmpegCmd(commandLine);
    }

    /**
     * 执行ffmpeg命令行
     * @param commandLine commandLine
     */
    private void executeFFmpegCmd(final String[] commandLine){
        if(commandLine == null){
            return;
        }
        FFmpegCmd.execute(commandLine, new FFmpegCmd.OnHandleListener() {
            @Override
            public void onBegin() {
                Log.i(TAG, "handle media onBegin...");
            }

            @Override
            public void onEnd(int result) {
                Log.i(TAG, "handle media onEnd...");
                if(isMux){
                    mHandler.obtainMessage(100).sendToTarget();
                }else {
                    runOnUiThread(new Runnable() {
                        @Override
                        public void run() {
                            Toast.makeText(MediaHandleActivity.this, "handle media finish...", Toast.LENGTH_SHORT).show();
                        }
                    });
                }
            }
        });
    }

}
