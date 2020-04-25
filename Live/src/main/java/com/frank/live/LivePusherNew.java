package com.frank.live;

import android.app.Activity;
import android.view.SurfaceHolder;
import android.view.TextureView;

import com.frank.live.listener.LiveStateChangeListener;
import com.frank.live.param.AudioParam;
import com.frank.live.param.VideoParam;
import com.frank.live.stream.AudioStream;
import com.frank.live.stream.VideoStream;
import com.frank.live.stream.VideoStreamNew;

public class LivePusherNew {

    //error of opening video encoder
    private final static int ERROR_VIDEO_ENCODER_OPEN = 0x01;
    //error of video encoding
    private final static int ERROR_VIDEO_ENCODE = 0x02;
    //error of opening audio encoder
    private final static int ERROR_AUDIO_ENCODER_OPEN = 0x03;
    //error of audio encoding
    private final static int ERROR_AUDIO_ENCODE = 0x04;
    //error of RTMP connecting server
    private final static int ERROR_RTMP_CONNECT = 0x05;
    //error of RTMP connecting stream
    private final static int ERROR_RTMP_CONNECT_STREAM = 0x06;
    //error of RTMP sending packet
    private final static int ERROR_RTMP_SEND_PACKET = 0x07;

    static {
        System.loadLibrary("live");
    }

    private AudioStream audioStream;
    private VideoStream videoStream;
//    private VideoStreamNew videoStream;

    private LiveStateChangeListener liveStateChangeListener;

    private Activity activity;

    public LivePusherNew(Activity activity, VideoParam videoParam, AudioParam audioParam) {
        this.activity = activity;
        native_init();
        videoStream = new VideoStream(this, activity, videoParam.getWidth(), videoParam.getHeight(),
                videoParam.getBitRate(), videoParam.getFrameRate(), videoParam.getCameraId());
        audioStream = new AudioStream(this, audioParam);
    }

    public LivePusherNew(Activity activity, VideoParam videoParam, AudioParam audioParam, TextureView textureView) {
        native_init();
//        videoStream = new VideoStreamNew(this, textureView, videoParam, activity);
        audioStream = new AudioStream(this, audioParam);
    }

    public void setPreviewDisplay(SurfaceHolder surfaceHolder) {
        videoStream.setPreviewDisplay(surfaceHolder);
    }

    public void switchCamera() {
        videoStream.switchCamera();
    }

    /**
     * setting mute
     *
     * @param isMute is mute or not
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
     * Callback this method, when native occurring error
     *
     * @param errCode errCode
     */
    public void errorFromNative(int errCode) {
        //stop pushing stream
        stopPush();
        if (liveStateChangeListener != null && activity != null) {
            String msg = "";
            switch (errCode) {
                case ERROR_VIDEO_ENCODER_OPEN:
                    msg = activity.getString(R.string.error_video_encoder);
                    break;
                case ERROR_VIDEO_ENCODE:
                    msg = activity.getString(R.string.error_video_encode);
                    break;
                case ERROR_AUDIO_ENCODER_OPEN:
                    msg = activity.getString(R.string.error_audio_encoder);
                    break;
                case ERROR_AUDIO_ENCODE:
                    msg = activity.getString(R.string.error_audio_encode);
                    break;
                case ERROR_RTMP_CONNECT:
                    msg = activity.getString(R.string.error_rtmp_connect);
                    break;
                case ERROR_RTMP_CONNECT_STREAM:
                    msg = activity.getString(R.string.error_rtmp_connect_strem);
                    break;
                case ERROR_RTMP_SEND_PACKET:
                    msg = activity.getString(R.string.error_rtmp_send_packet);
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

    public void pushVideo(byte[] y, byte[] u, byte[] v) {
        native_pushVideoNew(y, u, v);
    }

    private native void native_init();

    private native void native_start(String path);

    private native void native_setVideoCodecInfo(int width, int height, int fps, int bitrate);

    private native void native_setAudioCodecInfo(int sampleRateInHz, int channels);

    private native int getInputSamples();

    private native void native_pushAudio(byte[] data);

    private native void native_pushVideo(byte[] data);

    private native void native_pushVideoNew(byte[] y, byte[] u, byte[] v);

    private native void native_stop();

    private native void native_release();

}
