package com.frank.live.stream;

import android.media.AudioFormat;
import android.media.AudioRecord;
import android.media.MediaRecorder;

import com.frank.live.LivePusherNew;
import com.frank.live.param.AudioParam;

import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

public class AudioStream {

    private int inputSamples;
    private ExecutorService executor;
    private AudioRecord audioRecord;
    private LivePusherNew mLivePusher;
    private boolean isLiving;
    private boolean isMute;

    public AudioStream(LivePusherNew livePusher, AudioParam audioParam) {
        mLivePusher = livePusher;
        executor = Executors.newSingleThreadExecutor();
        int channelConfig;
        if (audioParam.getNumChannels() == 2) {
            channelConfig = AudioFormat.CHANNEL_IN_STEREO;
        } else {
            channelConfig = AudioFormat.CHANNEL_IN_MONO;
        }

        mLivePusher.setAudioCodecInfo(audioParam.getSampleRate(), audioParam.getNumChannels());
        inputSamples = mLivePusher.getInputSample() * 2;

        int minBufferSize = AudioRecord.getMinBufferSize(audioParam.getSampleRate(),
                channelConfig, audioParam.getAudioFormat()) * 2;
        int bufferSizeInBytes = minBufferSize > inputSamples ? minBufferSize : inputSamples;
        audioRecord = new AudioRecord(MediaRecorder.AudioSource.MIC, audioParam.getSampleRate(),
                channelConfig, audioParam.getAudioFormat(), bufferSizeInBytes);
    }


    public void startLive() {
        isLiving = true;
        executor.submit(new AudioTask());
    }

    public void stopLive() {
        isLiving = false;
    }


    public void release() {
        audioRecord.release();
    }


    class AudioTask implements Runnable {

        @Override
        public void run() {
            audioRecord.startRecording();
            byte[] bytes = new byte[inputSamples];
            while (isLiving) {
                if (!isMute) {
                    int len = audioRecord.read(bytes, 0, bytes.length);
                    if (len > 0) {
                        mLivePusher.pushAudio(bytes);
                    }
                }
            }
            audioRecord.stop();
        }
    }

    /**
     * Setting mute or not
     *
     * @param isMute isMute
     */
    public void setMute(boolean isMute) {
        this.isMute = isMute;
    }

}
