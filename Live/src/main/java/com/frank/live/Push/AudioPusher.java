package com.frank.live.Push;

import android.media.AudioRecord;
import android.media.MediaRecorder;
import android.util.Log;
import com.frank.live.LiveUtil;
import com.frank.live.param.AudioParam;

/**
 * 音频推流
 * Created by frank on 2018/1/28.
 */

public class AudioPusher extends Pusher {

    private AudioRecord audioRecord;
    private boolean isPushing;
    private int minBufferSize;
    private LiveUtil liveUtil;
    private boolean isMute;

    AudioPusher(AudioParam audioParam, LiveUtil liveUtil){
        this.liveUtil = liveUtil;
        initAudioRecord(audioParam);
        liveUtil.setAudioParams(audioParam.getSampleRate(), audioParam.getNumChannels());
    }

    @Override
    public void startPush() {
        isPushing = true;
        new AudioRecordThread(audioRecord).start();
    }

    @Override
    public void stopPush() {
        isPushing = false;
    }

    @Override
    public void release() {
        stopPush();
        if(audioRecord != null){
            audioRecord.release();
            audioRecord = null;
        }
    }

    private void initAudioRecord(AudioParam audioParam){
        minBufferSize = AudioRecord.getMinBufferSize(audioParam.getSampleRate(),
                audioParam.getChannelConfig(), audioParam.getAudioFormat());
        audioRecord = new AudioRecord(MediaRecorder.AudioSource.MIC, audioParam.getSampleRate(),
                audioParam.getChannelConfig(), audioParam.getAudioFormat(), minBufferSize);
    }

    /**
     * 录音线程:循环读取音频数据送去native层编码
     */
    class AudioRecordThread extends Thread{
        private AudioRecord audioRecord;

        AudioRecordThread(AudioRecord audioRecord){
            this.audioRecord = audioRecord;
        }

        @Override
        public void run() {
            super.run();
            audioRecord.startRecording();
            while (isPushing){//处于推流状态
                if(!isMute){//如果不静音，才推音频流
                    byte[] audioBuffer = new byte[minBufferSize];
                    int length = audioRecord.read(audioBuffer, 0, audioBuffer.length);
                    if(length > 0){
//                        Log.i("AudioPusher", "is recording...");
                        liveUtil.pushAudioData(audioBuffer, length);
                    }
                }
            }
            audioRecord.stop();
        }
    }

    /**
     * 设置静音
     * @param isMute 是否静音
     */
    void setMute(boolean isMute){
        this.isMute = isMute;
    }

}
