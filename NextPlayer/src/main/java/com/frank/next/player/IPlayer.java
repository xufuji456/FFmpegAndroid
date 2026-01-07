package com.frank.next.player;

import android.view.Surface;
import android.view.SurfaceHolder;

import java.io.IOException;
import java.util.Map;

/**
 * Note: interface of player
 * Date: 2026/1/6
 * Author: frank
 */
public interface IPlayer {

    /**********************************message begin**********************************/

    int MSG_COMPONENT_OPEN     = 1000;
    int MSG_OPEN_INPUT         = 1001;
    int MSG_FIND_STREAM_INFO   = 1002;
    int MSG_VIDEO_FIRST_PACKET = 1004;
    int MSG_VIDEO_DECODE_START = 1006;
    int MSG_ON_PREPARED        = 1007;
    int MSG_SET_VIDEO_SIZE     = 1008;
    int MSG_SET_VIDEO_SAR      = 1009;
    int MSG_ROTATION_CHANGED   = 1010;
    int MSG_VIDEO_RENDER_START = 1011;
    int MSG_AUDIO_RENDER_START = 1012;
    int MSG_NO_OP              = 1013; // Flush
    int MSG_ON_ERROR           = 1014;
    int MSG_ON_COMPLETED       = 1015;
    int MSG_MEDIA_INFO         = 1016;

    int MSG_BUFFER_START       = 2000;
    int MSG_BUFFER_UPDATE      = 2001; // progress
    int MSG_BUFFER_BYTE_UPDATE = 2002; // cached data in bytes
    int MSG_BUFFER_TIME_UPDATE = 2003; // cached duration in ms
    int MSG_BUFFER_END         = 2004;

    int MSG_SEEK_COMPLETED     = 3003;
    int MSG_PLAY_URL_CHANGED   = 3006;

    /*******************************PlayControl begin*********************************/

    void prepareAsync() throws IllegalStateException;

    void start() throws IllegalStateException;

    void pause() throws IllegalStateException;

    void seekTo(long msec) throws IllegalStateException;

    void stop() throws IllegalStateException;

    void reset();

    void release();

    /***********************************Set begin************************************/

    void setCachePath(String path);

    void setLiveMode(boolean enable);

    void setEnableMediaCodec(boolean enable);

    void setDataSource(String path) throws IOException, IllegalStateException;

    void setDataSource(String path, Map<String, String> headers) throws IOException, IllegalStateException;

    void setSurface(Surface surface);

    void setDisplay(SurfaceHolder sh);

    void setSpeed(float speed);

    void setVolume(float leftVolume, float rightVolume);

    void setScreenOnWhilePlaying(boolean screenOn);


    /**********************************Get begin***********************************/

    String getAudioCodecInfo();

    String getVideoCodecInfo();

    int getVideoWidth();

    int getVideoHeight();

    long getCurrentPosition();

    long getDuration();

    String getPlayUrl() throws IllegalStateException;

    int getVideoSarNum();

    int getVideoSarDen();

    int getVideoDecoder();

    float getVideoFrameRate();

    float getVideoDecodeFrameRate();

    float getVideoRenderFrameRate();

    long getVideoCacheTime();

    long getAudioCacheTime();

    long getVideoCacheSize();

    long getAudioCacheSize();

    long getBitRate();

    long getFileSize();

    boolean isPlaying();

    int getPlayerState();

    long getSeekCostTime();

    String getDataSource();


    /**********************************interface begin**********************************/

    void setOnPreparedListener(OnPreparedListener listener);

    void setOnInfoListener(OnInfoListener listener);

    void setOnErrorListener(OnErrorListener listener);

    void setOnBufferingUpdateListener(OnBufferUpdateListener listener);

    void setOnVideoSizeChangedListener(OnVideoSizeChangedListener listener);

    void setOnSeekCompleteListener(OnSeekCompleteListener listener);

    void setOnCompletionListener(OnCompleteListener listener);

    interface OnPreparedListener {
        void onPrepared(IPlayer mp);
    }

    interface OnBufferUpdateListener {
        void onBufferUpdate(int percent);
    }

    interface OnVideoSizeChangedListener {
        void onVideoSizeChanged(int width, int height);
    }

    interface OnInfoListener {
        boolean onInfo(int what, int extra);
    }

    interface OnErrorListener {
        boolean onError(int what, int extra);
    }

    interface OnSeekCompleteListener {
        void onSeekComplete(IPlayer mp);
    }

    interface OnCompleteListener {
        void onComplete(IPlayer mp);
    }

}
