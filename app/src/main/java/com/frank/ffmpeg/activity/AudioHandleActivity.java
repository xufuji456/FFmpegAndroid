package com.frank.ffmpeg.activity;

import android.annotation.SuppressLint;
import android.os.Environment;
import android.os.Handler;
import android.os.Message;
import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.util.Log;
import android.view.View;
import android.widget.ProgressBar;
import java.io.File;

import com.frank.ffmpeg.AudioPlayer;
import com.frank.ffmpeg.FFmpegCmd;
import com.frank.ffmpeg.R;
import com.frank.ffmpeg.util.FFmpegUtil;
import com.frank.ffmpeg.util.FileUtil;

/**
 * 使用ffmpeg处理音频
 * Created by frank on 2018/1/23.
 */

public class AudioHandleActivity extends AppCompatActivity implements View.OnClickListener{

    private final static String TAG = AudioHandleActivity.class.getSimpleName();
    private final static String PATH = Environment.getExternalStorageDirectory().getPath();
    private String srcFile = PATH + File.separator + "tiger.mp3";
    private String appendFile = PATH + File.separator + "test.mp3";
    private final static int MSG_BEGIN = 11;
    private final static int MSG_FINISH = 12;
    private ProgressBar progress_audio;

    @SuppressLint("HandlerLeak")
    private Handler mHandler = new Handler(){
        @Override
        public void handleMessage(Message msg) {
            super.handleMessage(msg);
            switch (msg.what){
                case MSG_BEGIN:
                    progress_audio.setVisibility(View.VISIBLE);
                    setGone();
                    break;
                case MSG_FINISH:
                    progress_audio.setVisibility(View.GONE);
                    setVisible();
                    break;
                default:
                    break;
            }
        }
    };

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_audio_handle);

        initView();
    }

    private void initView() {
        progress_audio = (ProgressBar) findViewById(R.id.progress_audio);
        findViewById(R.id.btn_transform).setOnClickListener(this);
        findViewById(R.id.btn_cut).setOnClickListener(this);
        findViewById(R.id.btn_concat).setOnClickListener(this);
        findViewById(R.id.btn_mix).setOnClickListener(this);
        findViewById(R.id.btn_play_audio).setOnClickListener(this);
        findViewById(R.id.btn_play_opensl).setOnClickListener(this);
        findViewById(R.id.btn_audio_encode).setOnClickListener(this);
        findViewById(R.id.btn_pcm_concat).setOnClickListener(this);
    }

    private void setVisible() {
        findViewById(R.id.btn_transform).setVisibility(View.VISIBLE);
        findViewById(R.id.btn_cut).setVisibility(View.VISIBLE);
        findViewById(R.id.btn_concat).setVisibility(View.VISIBLE);
        findViewById(R.id.btn_mix).setVisibility(View.VISIBLE);
        findViewById(R.id.btn_play_audio).setVisibility(View.VISIBLE);
        findViewById(R.id.btn_play_opensl).setVisibility(View.VISIBLE);
        findViewById(R.id.btn_audio_encode).setVisibility(View.VISIBLE);
        findViewById(R.id.btn_pcm_concat).setVisibility(View.VISIBLE);
    }

    private void setGone() {
        findViewById(R.id.btn_transform).setVisibility(View.GONE);
        findViewById(R.id.btn_cut).setVisibility(View.GONE);
        findViewById(R.id.btn_concat).setVisibility(View.GONE);
        findViewById(R.id.btn_mix).setVisibility(View.GONE);
        findViewById(R.id.btn_play_audio).setVisibility(View.GONE);
        findViewById(R.id.btn_play_opensl).setVisibility(View.GONE);
        findViewById(R.id.btn_audio_encode).setVisibility(View.GONE);
        findViewById(R.id.btn_pcm_concat).setVisibility(View.GONE);
    }

    @Override
    public void onClick(View v) {
        int handleType;
        switch (v.getId()){
            case R.id.btn_transform:
                handleType = 0;
                break;
            case R.id.btn_cut:
                handleType = 1;
                break;
            case R.id.btn_concat:
                handleType = 2;
                break;
            case R.id.btn_mix:
                handleType = 3;
                break;
            case R.id.btn_play_audio:
                handleType = 4;
                break;
            case R.id.btn_play_opensl:
                handleType = 5;
                break;
            case R.id.btn_audio_encode:
                handleType = 6;
                break;
            case R.id.btn_pcm_concat:
                handleType = 7;
                break;
            default:
                handleType = 0;
                break;
        }
        doHandleAudio(handleType);
    }

    /**
     * 调用ffmpeg处理音频
     * @param handleType handleType
     */
    private void doHandleAudio(int handleType){
        String[] commandLine = null;
        if (!FileUtil.checkFileExist(srcFile)){
            return;
        }
        switch (handleType){
            case 0://转码
                String transformFile = PATH + File.separator + "transform.aac";
                commandLine = FFmpegUtil.transformAudio(srcFile, transformFile);
                break;
            case 1://剪切
                String cutFile = PATH + File.separator + "cut.mp3";
                commandLine = FFmpegUtil.cutAudio(srcFile, 10, 15, cutFile);
                break;
            case 2://合并，支持MP3、AAC、AMR等，不支持PCM裸流，不支持WAV（PCM裸流加音频头）
                if (!FileUtil.checkFileExist(appendFile)){
                    return;
                }
                String concatFile = PATH + File.separator + "concat.mp3";
                commandLine = FFmpegUtil.concatAudio(srcFile, appendFile, concatFile);
                break;
            case 3://混合
                if (!FileUtil.checkFileExist(appendFile)){
                    return;
                }
                String mixFile = PATH + File.separator + "mix.aac";
                commandLine = FFmpegUtil.mixAudio(srcFile, appendFile, mixFile);
                break;
            case 4://解码播放（AudioTrack）
                new Thread(new Runnable() {
                    @Override
                    public void run() {
                        new AudioPlayer().play(srcFile);
                    }
                }).start();
                return;
            case 5://解码播放（OpenSL ES）
                new Thread(new Runnable() {
                    @Override
                    public void run() {
                        new AudioPlayer().playAudio(srcFile);
                    }
                }).start();
                return;
            case 6://音频编码
                //可编码成WAV、AAC。如果需要编码成MP3、AMR，ffmpeg需要重新编译，把MP3、AMR库enable
                String pcmFile = PATH + File.separator + "concat.pcm";
                String wavFile = PATH + File.separator + "new.wav";
                //pcm数据的采样率，一般采样率为8000、16000、44100
                int sampleRate = 8000;
                //pcm数据的声道，单声道为1，立体声道为2
                int channel = 1;
                commandLine = FFmpegUtil.encodeAudio(pcmFile, wavFile, sampleRate, channel);
                break;
            case 7://PCM裸流音频文件合并
                String srcPCM = PATH + File.separator + "audio.pcm";//第一个pcm文件
                String appendPCM = PATH + File.separator + "audio.pcm";//第二个pcm文件
                String concatPCM = PATH + File.separator + "concat.pcm";//合并后的文件
                if (!FileUtil.checkFileExist(srcPCM) || !FileUtil.checkFileExist(appendPCM)){
                    return;
                }

                mHandler.obtainMessage(MSG_BEGIN).sendToTarget();
                FileUtil.concatFile(srcPCM, appendPCM, concatPCM);
                mHandler.obtainMessage(MSG_FINISH).sendToTarget();
                return;
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
                Log.i(TAG, "handle audio onBegin...");
                mHandler.obtainMessage(MSG_BEGIN).sendToTarget();
            }

            @Override
            public void onEnd(int result) {
                Log.i(TAG, "handle audio onEnd...");
                mHandler.obtainMessage(MSG_FINISH).sendToTarget();
            }
        });
    }

}
