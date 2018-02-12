package com.frank.live;

import com.frank.live.listener.LiveStateChangeListener;

/**
 * 直播核心类
 * Created by frank on 2017/8/15.
 */

public class LiveUtil {

    static {
        System.loadLibrary("live");
    }

    private native int native_start(String url);
    private native void setVideoParam(int width, int height, int bitRate, int frameRate);
    private native void setAudioParam(int sampleRate, int numChannels);
    private native void pushVideo(byte data[]);
    private native void pushAudio(byte data[], int length);
    private native void native_stop();
    private native void native_release();

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
    private final static int ERROR_RTMP_SEND_PACKAT = 0x07;

    private LiveStateChangeListener liveStateChangeListener;

    public LiveUtil(){}

    public int startPush(String url){
        return native_start(url);
    }

    public void setVideoParams(int width, int height, int bitRate, int frameRate){
        setVideoParam(width, height, bitRate, frameRate);
    }

    public void setAudioParams(int sampleRate, int numChannels){
        setAudioParam(sampleRate, numChannels);
    }

    public void pushVideoData(byte[] data){
        pushVideo(data);
    }

    public void pushAudioData(byte[] data, int length){
        pushAudio(data, length);
    }

    public void stopPush(){
        native_stop();
    }

    public void release(){
        native_release();
    }

    public void setOnLiveStateChangeListener(LiveStateChangeListener liveStateChangeListener){
        this.liveStateChangeListener = liveStateChangeListener;
    }

    /**
     * 当native报错时，回调这个方法
     * @param errCode errCode
     */
    public void errorFromNative(int errCode){
        //直播出错了，应该停止推流
        stopPush();
        if(liveStateChangeListener != null){
            String msg = "";
            switch (errCode){
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
                case ERROR_RTMP_SEND_PACKAT:
                    msg = "RTMP发送数据包失败...";
                    break;
                default:
                    break;
            }
            liveStateChangeListener.onError(msg);
        }
    }
}
