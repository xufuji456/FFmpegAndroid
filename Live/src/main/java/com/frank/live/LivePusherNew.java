package com.frank.live;

import android.app.Activity;
import android.view.SurfaceHolder;

import com.frank.live.listener.LiveStateChangeListener;
import com.frank.live.param.AudioParam;
import com.frank.live.param.VideoParam;
import com.frank.live.stream.AudioStream;
import com.frank.live.stream.VideoStream;

public class LivePusherNew {

    //视频编码器打开失败
    private final static int ERROR_VIDEO_ENCODER_OPEN = 0x01;
    //视频帧编码失败
    private final static int ERROR_VIDEO_ENCODE = 0x02;
    //音频编码器打开失败
    private final static int ERROR_AUDIO_ENCODER_OPEN = 0x03;
    //音频帧编码失败
    private final static int ERROR_AUDIO_ENCODE = 0x04;
    //RTMP连接失败
    private final static int ERROR_RTMP_CONNECT = 0x05;
    //RTMP连接流失败
    private final static int ERROR_RTMP_CONNECT_STREAM = 0x06;
    //RTMP发送数据包失败
    private final static int ERROR_RTMP_SEND_PACKET = 0x07;

    static {
        System.loadLibrary("live");
    }

    private AudioStream audioStream;
    private VideoStream videoStream;

    private LiveStateChangeListener liveStateChangeListener;

    public LivePusherNew(Activity activity, VideoParam videoParam, AudioParam audioParam) {
        native_init();
        videoStream = new VideoStream(this, activity, videoParam.getWidth(), videoParam.getHeight(),
                videoParam.getBitRate(), videoParam.getFrameRate(), videoParam.getCameraId());
        audioStream = new AudioStream(this, audioParam);
    }

    public void setPreviewDisplay(SurfaceHolder surfaceHolder) {
        videoStream.setPreviewDisplay(surfaceHolder);
    }

    public void switchCamera() {
        videoStream.switchCamera();
    }

    /**
     * 设置静音
     *
     * @param isMute 是否静音
     */
    public void setMute(boolean isMute) {
        audioStream.setMute(isMute);
    }

    public void startPush(String path, LiveStateChangeListener stateChangeListener) {
        this.liveStateChangeListener = stateChangeListener;
        native_start(path);
        videoStream.startLive();
        audioStream.startLive();
    }

    public void stopPush() {
        videoStream.stopLive();
        audioStream.stopLive();
        native_stop();
    }

    public void release() {
        videoStream.release();
        audioStream.release();
        native_release();
    }

    /**
     * 当native报错时，回调这个方法
     *
     * @param errCode errCode
     */
    public void errorFromNative(int errCode) {
        //停止推流
        stopPush();
        if (liveStateChangeListener != null) {
            String msg = "";
            switch (errCode) {
                case ERROR_VIDEO_ENCODER_OPEN:
                    msg = "视频编码器打开失败...";
                    break;
                case ERROR_VIDEO_ENCODE:
                    msg = "视频帧编码失败...";
                    break;
                case ERROR_AUDIO_ENCODER_OPEN:
                    msg = "音频编码器打开失败...";
                    break;
                case ERROR_AUDIO_ENCODE:
                    msg = "音频帧编码失败...";
                    break;
                case ERROR_RTMP_CONNECT:
                    msg = "RTMP连接失败...";
                    break;
                case ERROR_RTMP_CONNECT_STREAM:
                    msg = "RTMP连接流失败...";
                    break;
                case ERROR_RTMP_SEND_PACKET:
                    msg = "RTMP发送数据包失败...";
                    break;
                default:
                    break;
            }
            liveStateChangeListener.onError(msg);
        }
    }

    public void setVideoCodecInfo(int width, int height, int fps, int bitrate) {
        native_setVideoCodecInfo(width, height, fps, bitrate);
    }

    public void setAudioCodecInfo(int sampleRateInHz, int channels) {
        native_setAudioCodecInfo(sampleRateInHz, channels);
    }

    public void start(String path) {
        native_start(path);
    }

    public int getInputSample() {
        return getInputSamples();
    }

    public void pushAudio(byte[] data) {
        native_pushAudio(data);
    }

    public void pushVideo(byte[] data) {
        native_pushVideo(data);
    }

    private native void native_init();

    private native void native_start(String path);

    private native void native_setVideoCodecInfo(int width, int height, int fps, int bitrate);

    private native void native_setAudioCodecInfo(int sampleRateInHz, int channels);

    private native int getInputSamples();

    private native void native_pushAudio(byte[] data);

    private native void native_pushVideo(byte[] data);

    private native void native_stop();

    private native void native_release();

}
