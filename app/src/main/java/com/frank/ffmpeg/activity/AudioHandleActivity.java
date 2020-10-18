package com.frank.ffmpeg.activity;

import android.annotation.SuppressLint;
import android.os.Environment;
import android.os.Handler;
import android.os.Message;
import android.os.Bundle;
import android.view.View;
import android.widget.LinearLayout;
import android.widget.TextView;

import java.io.File;
import java.util.ArrayList;
import java.util.List;
import java.util.Locale;

import com.frank.ffmpeg.AudioPlayer;
import com.frank.ffmpeg.R;
import com.frank.ffmpeg.handler.FFmpegHandler;
import com.frank.ffmpeg.mp3.Mp3Converter;
import com.frank.ffmpeg.util.FFmpegUtil;
import com.frank.ffmpeg.util.FileUtil;

import static com.frank.ffmpeg.handler.FFmpegHandler.MSG_BEGIN;
import static com.frank.ffmpeg.handler.FFmpegHandler.MSG_FINISH;
import static com.frank.ffmpeg.handler.FFmpegHandler.MSG_PROGRESS;

/**
 * Using ffmpeg command to handle audio
 * Created by frank on 2018/1/23.
 */

public class AudioHandleActivity extends BaseActivity {

    private final static String PATH = Environment.getExternalStorageDirectory().getPath();
    private String appendFile = PATH + File.separator + "heart.m4a";

    private LinearLayout layoutAudioHandle;
    private LinearLayout layoutProgress;
    private TextView txtProgress;
    private int viewId;
    private FFmpegHandler ffmpegHandler;

    private String outputPath1 = PATH + File.separator + "output1.mp3";
    private String outputPath2 = PATH + File.separator + "output2.mp3";
    private boolean isJointing = false;

    private final static boolean useFFmpeg = true;

    @SuppressLint("HandlerLeak")
    private Handler mHandler = new Handler() {
        @Override
        public void handleMessage(Message msg) {
            super.handleMessage(msg);
            switch (msg.what) {
                case MSG_BEGIN:
                    layoutProgress.setVisibility(View.VISIBLE);
                    layoutAudioHandle.setVisibility(View.GONE);
                    break;
                case MSG_FINISH:
                    layoutProgress.setVisibility(View.GONE);
                    layoutAudioHandle.setVisibility(View.VISIBLE);
                    if (isJointing) {
                        isJointing = false;
                        FileUtil.deleteFile(outputPath1);
                        FileUtil.deleteFile(outputPath2);
                    }
                    break;
                case MSG_PROGRESS:
                    int progress = msg.arg1;
                    int duration = msg.arg2;
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
        layoutProgress = getView(R.id.layout_progress);
        txtProgress = getView(R.id.txt_progress);
        layoutAudioHandle = getView(R.id.layout_audio_handle);
        initViewsWithClick(
                R.id.btn_transform,
                R.id.btn_cut,
                R.id.btn_concat,
                R.id.btn_mix,
                R.id.btn_play_audio,
                R.id.btn_play_opensl,
                R.id.btn_audio_encode,
                R.id.btn_pcm_concat,
                R.id.btn_audio_speed
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
     * Using ffmpeg cmd to handle audio
     *
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
            case R.id.btn_transform:
                if (useFFmpeg) { //use FFmpeg to transform
                    String transformFile = PATH + File.separator + "transformAudio.mp3";
                    commandLine = FFmpegUtil.transformAudio(srcFile, transformFile);
                } else { //use MediaCodec and libmp3lame to transform
                    new Thread(() -> {
                        String transformInput = PATH + File.separator + "transformAudio.mp3";
                        Mp3Converter mp3Converter = new Mp3Converter();
                        mp3Converter.convertToMp3(srcFile, transformInput);
                    }).start();
                }
                break;
            case R.id.btn_cut://cut audio, it's best not include special characters
                String suffix = FileUtil.getFileSuffix(srcFile);
                if (suffix == null || suffix.isEmpty()) {
                    return;
                }
                String cutFile = PATH + File.separator + "cutAudio" + suffix;
                commandLine = FFmpegUtil.cutAudio(srcFile, 10, 15, cutFile);
                break;
            case R.id.btn_concat://concat audio
                if (!FileUtil.checkFileExist(appendFile)) {
                    return;
                }
//                List<String> fileList = new ArrayList<>();
//                fileList.add(srcFile);
//                fileList.add(appendFile);
//                String concatFile = PATH + File.separator + "concat.mp3";
//                commandLine = FFmpegUtil.concatAudio(fileList, concatFile);
//                break;
                concatAudio(srcFile);
                return;
            case R.id.btn_mix://mix audio
                if (!FileUtil.checkFileExist(appendFile)) {
                    return;
                }
                String mixSuffix = FileUtil.getFileSuffix(srcFile);
                if (mixSuffix == null || mixSuffix.isEmpty()) {
                    return;
                }
                String mixFile = PATH + File.separator + "mix" + mixSuffix;
                commandLine = FFmpegUtil.mixAudio(srcFile, appendFile, mixFile);
                break;
            case R.id.btn_play_audio://use AudioTrack to play audio
                new Thread(() -> new AudioPlayer().play(srcFile)).start();
                return;
            case R.id.btn_play_opensl://use OpenSL ES to play audio
                new Thread(() -> new AudioPlayer().playAudio(srcFile)).start();
                return;
            case R.id.btn_audio_encode://audio encode
                String pcmFile = PATH + File.separator + "concat.pcm";
                String wavFile = PATH + File.separator + "new.wav";
                //sample rate, normal is 8000/16000/44100
                int sampleRate = 8000;
                //channel num of pcm
                int channel = 1;
                commandLine = FFmpegUtil.encodeAudio(pcmFile, wavFile, sampleRate, channel);
                break;
            case R.id.btn_pcm_concat://concat PCM streams
                String srcPCM = PATH + File.separator + "audio.pcm";
                String appendPCM = PATH + File.separator + "audio.pcm";
                String concatPCM = PATH + File.separator + "concat.pcm";
                if (!FileUtil.checkFileExist(srcPCM) || !FileUtil.checkFileExist(appendPCM)) {
                    return;
                }

                mHandler.obtainMessage(MSG_BEGIN).sendToTarget();
                FileUtil.concatFile(srcPCM, appendPCM, concatPCM);
                mHandler.obtainMessage(MSG_FINISH).sendToTarget();
                return;
            case R.id.btn_audio_speed://change audio speed
                float speed = 2.0f;//from 0.5 to 2.0
                String speedPath = PATH + File.separator + "speed.mp3";
                commandLine = FFmpegUtil.changeAudioSpeed(srcFile, speedPath, speed);
                break;
            default:
                break;
        }
        if (ffmpegHandler != null && commandLine != null) {
            ffmpegHandler.executeFFmpegCmd(commandLine);
        }
    }

    private void concatAudio(String selectedPath) {
        if (ffmpegHandler == null || selectedPath.isEmpty() || appendFile.isEmpty()) {
            return;
        }
        isJointing = true;
        String targetPath = PATH + File.separator + "concatAudio.mp3";
        String[] transformCmd1 = FFmpegUtil.transformAudio(selectedPath, "libmp3lame", outputPath1);
        String[] transformCmd2 = FFmpegUtil.transformAudio(appendFile, "libmp3lame", outputPath2);
        List<String> fileList = new ArrayList<>();
        fileList.add(outputPath1);
        fileList.add(outputPath2);
        String[] jointVideoCmd = FFmpegUtil.concatAudio(fileList, targetPath);
        List<String[]> commandList = new ArrayList<>();
        commandList.add(transformCmd1);
        commandList.add(transformCmd2);
        commandList.add(jointVideoCmd);
        ffmpegHandler.executeFFmpegCmds(commandList);
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        if (mHandler != null) {
            mHandler.removeCallbacksAndMessages(null);
        }
    }

}
