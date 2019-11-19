package com.frank.ffmpeg.activity;

import android.annotation.SuppressLint;
import android.os.Environment;
import android.os.Handler;
import android.os.Message;
import android.os.Bundle;
import android.view.View;
import android.widget.LinearLayout;
import android.widget.ProgressBar;
import java.io.File;

import com.frank.ffmpeg.AudioPlayer;
import com.frank.ffmpeg.R;
import com.frank.ffmpeg.handler.FFmpegHandler;
import com.frank.ffmpeg.mp3.Mp3Converter;
import com.frank.ffmpeg.util.FFmpegUtil;
import com.frank.ffmpeg.util.FileUtil;

import static com.frank.ffmpeg.handler.FFmpegHandler.MSG_BEGIN;
import static com.frank.ffmpeg.handler.FFmpegHandler.MSG_FINISH;

/**
 * 使用ffmpeg处理音频
 * Created by frank on 2018/1/23.
 */

public class AudioHandleActivity extends BaseActivity {

    private final static String PATH = Environment.getExternalStorageDirectory().getPath();
    private String appendFile = PATH + File.separator + "test.mp3";

    private ProgressBar progressAudio;
    private LinearLayout layoutAudioHandle;
    private int viewId;
    private FFmpegHandler ffmpegHandler;

    @SuppressLint("HandlerLeak")
    private Handler mHandler = new Handler(){
        @Override
        public void handleMessage(Message msg) {
            super.handleMessage(msg);
            switch (msg.what) {
                case MSG_BEGIN:
                    progressAudio.setVisibility(View.VISIBLE);
                    layoutAudioHandle.setVisibility(View.GONE);
                    break;
                case MSG_FINISH:
                    progressAudio.setVisibility(View.GONE);
                    layoutAudioHandle.setVisibility(View.VISIBLE);
                    break;
                default:
                    break;
            }
        }
    };

    @Override
    int getLayoutId() {
        return R.layout.activity_audio_handle;
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        hideActionBar();
        initView();
        ffmpegHandler = new FFmpegHandler(mHandler);
    }

    private void initView() {
        progressAudio = getView(R.id.progress_audio);
        layoutAudioHandle = getView(R.id.layout_audio_handle);
        initViewsWithClick(
                R.id.btn_transform,
                R.id.btn_cut,
                R.id.btn_concat,
                R.id.btn_mix,
                R.id.btn_play_audio,
                R.id.btn_play_opensl,
                R.id.btn_audio_encode,
                R.id.btn_pcm_concat
        );
    }

    @Override
    public void onViewClick(View view) {
        viewId = view.getId();
        selectFile();
    }

    @Override
    void onSelectedFile(String filePath) {
        doHandleAudio(filePath);
    }

    /**
     * 调用ffmpeg处理音频
     * @param srcFile srcFile
     */
    private void doHandleAudio(final String srcFile) {
        String[] commandLine = null;
        if (!FileUtil.checkFileExist(srcFile)) {
            return;
        }
        if (!FileUtil.isAudio(srcFile)) {
            showToast(getString(R.string.wrong_audio_format));
            return;
        }
        switch (viewId) {
            case R.id.btn_transform://转码
//                String transformFile = PATH + File.separator + "transform.aac";
//                commandLine = FFmpegUtil.transformAudio(srcFile, transformFile);
                //使用mp3lame进行转码
                String inputFile = PATH + File.separator + "hello.aac";
                String transformFile = PATH + File.separator + "transform.mp3";
                Mp3Converter mp3Converter = new Mp3Converter();
                mp3Converter.convertToMp3(inputFile, transformFile);
                break;
            case R.id.btn_cut://剪切(注意原文件与剪切文件格式一致，文件绝对路径最好不包含中文、特殊字符)
                String cutFile = PATH + File.separator + "cut.mp3";
                commandLine = FFmpegUtil.cutAudio(srcFile, 10, 15, cutFile);
                break;
            case R.id.btn_concat://合并，支持MP3、AAC、AMR等，不支持PCM裸流，不支持WAV（PCM裸流加音频头）
                if (!FileUtil.checkFileExist(appendFile)){
                    return;
                }
                String concatFile = PATH + File.separator + "concat.mp3";
                commandLine = FFmpegUtil.concatAudio(srcFile, appendFile, concatFile);
                break;
            case R.id.btn_mix://混音
                if (!FileUtil.checkFileExist(appendFile)){
                    return;
                }
                String mixFile = PATH + File.separator + "mix.aac";
                commandLine = FFmpegUtil.mixAudio(srcFile, appendFile, mixFile);
                break;
            case R.id.btn_play_audio://解码播放（AudioTrack）
                new Thread(new Runnable() {
                    @Override
                    public void run() {
                        new AudioPlayer().play(srcFile);
                    }
                }).start();
                return;
            case R.id.btn_play_opensl://解码播放（OpenSL ES）
                new Thread(new Runnable() {
                    @Override
                    public void run() {
                        new AudioPlayer().playAudio(srcFile);
                    }
                }).start();
                return;
            case R.id.btn_audio_encode://音频编码
                //可编码成WAV、AAC。如果需要编码成MP3，ffmpeg需要重新编译，把MP3库enable
                String pcmFile = PATH + File.separator + "concat.pcm";
                String wavFile = PATH + File.separator + "new.wav";
                //pcm数据的采样率，一般采样率为8000、16000、44100
                int sampleRate = 8000;
                //pcm数据的声道，单声道为1，立体声道为2
                int channel = 1;
                commandLine = FFmpegUtil.encodeAudio(pcmFile, wavFile, sampleRate, channel);
                break;
            case R.id.btn_pcm_concat://PCM裸流音频文件合并
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
