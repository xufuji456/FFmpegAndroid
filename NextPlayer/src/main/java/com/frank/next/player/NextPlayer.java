package com.frank.next.player;

import android.content.Context;
import android.os.Handler;
import android.os.Looper;
import android.os.Message;
import android.text.TextUtils;
import android.util.Log;
import android.view.Surface;
import android.view.SurfaceHolder;

import androidx.annotation.NonNull;

import com.frank.next.loader.LibraryLoader;

import java.io.IOException;
import java.lang.ref.WeakReference;
import java.nio.charset.StandardCharsets;
import java.util.Map;


public class NextPlayer extends BasePlayer {
    private final static String TAG = NextPlayer.class.getName();

    enum LogLevel {
        LOG_DEFAULT,
        LOG_DEBUG,
        LOG_INFO,
        LOG_WARN,
        LOG_ERROR
    }

    private static final int gLogCallBackLevel = LogLevel.LOG_DEBUG.ordinal();
    private static NativeLogCallback mNativeLogListener;

    private int mVideoWidth;
    private int mVideoHeight;
    private int mVideoSarNum;
    private int mVideoSarDen;

    private String mDataSource;
    private EventHandler mEventHandler;
    private SurfaceHolder mSurfaceHolder;

    private static volatile boolean mNativeInitialized = false;

    public static void loadLibraryOnce() {
        LibraryLoader.loadLibsOnce();
    }

    private static void initOnce() {
        synchronized (NextPlayer.class) {
            if (!mNativeInitialized) {
                native_init();
                mNativeInitialized = true;
            }
        }
    }

    public NextPlayer() {
        this(null);
    }

    public NextPlayer(Context context) {
        initPlayer(context);
    }

    private void initPlayer(Context context) {
        loadLibraryOnce();
        initOnce();

        Looper looper;
        if ((looper = Looper.myLooper()) != null) {
            mEventHandler = new EventHandler(this, looper);
        } else if ((looper = Looper.getMainLooper()) != null) {
            mEventHandler = new EventHandler(this, looper);
        } else {
            mEventHandler = null;
        }
        native_setup(new WeakReference<>(this));
    }

    @Override
    public void setDisplay(SurfaceHolder holder) throws IllegalStateException {
        mSurfaceHolder = holder;
        Surface surface;
        if (holder != null) {
            surface = holder.getSurface();
        } else {
            surface = null;
        }
        _setVideoSurface(surface);
    }

    @Override
    public void setDataSource(String path, Map<String, String> headers)
            throws IOException, IllegalStateException {
        if (headers != null && !headers.isEmpty()) {
            StringBuilder sb = new StringBuilder();
            for (Map.Entry<String, String> entry : headers.entrySet()) {
                sb.append(entry.getKey());
                sb.append(":");
                String value = entry.getValue();
                if (!TextUtils.isEmpty(value))
                    sb.append(entry.getValue());
                sb.append("\r\n");
                _setHeaders(sb.toString());
            }
        }
        setDataSource(path);
    }

    @Override
    public void setDataSource(String path) throws IOException, IllegalStateException {
        mDataSource = path;
        _setDataSource(path);
    }

    @Override
    public String getDataSource() {
        return mDataSource;
    }

    @Override
    public void prepareAsync() throws IllegalStateException {
        _prepareAsync();
    }

    @Override
    public void start() throws IllegalStateException {
        _start();
    }

    @Override
    public void stop() throws IllegalStateException {
        _stop();
    }

    @Override
    public void pause() throws IllegalStateException {
        _pause();
    }

    @Override
    public void setScreenOnWhilePlaying(boolean screenOn) {
        if (mSurfaceHolder != null) {
            mSurfaceHolder.setKeepScreenOn(screenOn);
        }
    }

    @Override
    public void setEnableMediaCodec(boolean enable) {
        _setEnableMediaCodec(enable);
    }

    @Override
    public void setCachePath(String path) {
        _setVideoCacheDir(path);
    }

    @Override
    public String getAudioCodecInfo() {
        return _getAudioCodecInfo();
    }

    @Override
    public String getVideoCodecInfo() {
        return _getVideoCodecInfo();
    }

    @Override
    public float getVideoFrameRate() {
        return _getVideoFileFps();
    }

    @Override
    public int getVideoWidth() {
        return mVideoWidth;
    }

    @Override
    public int getVideoHeight() {
        return mVideoHeight;
    }

    @Override
    public boolean isPlaying() {
        return _playing();
    }

    @Override
    public void seekTo(long msec) throws IllegalStateException {
        _seekTo(msec);
    }

    @Override
    public long getCurrentPosition() {
        return _getCurrentPosition();
    }

    @Override
    public long getDuration() {
        return _getDuration();
    }

    @Override
    public void release() {
        resetListeners();
        _release();
    }

    @Override
    public void reset() {
        _reset();
        mEventHandler.removeCallbacksAndMessages(null);
        mVideoWidth = 0;
        mVideoHeight = 0;
    }

    @Override
    public void setVolume(float leftVolume, float rightVolume) {
        _setVolume(leftVolume, rightVolume);
    }

    @Override
    public String getPlayUrl() throws IllegalStateException {
        return _getPlayUrl();
    }

    @Override
    public int getVideoSarNum() {
        return mVideoSarNum;
    }

    @Override
    public int getVideoSarDen() {
        return mVideoSarDen;
    }

    @Override
    public void setSurface(Surface surface) {
        mSurfaceHolder = null;
        _setVideoSurface(surface);
    }

    @Override
    public void setSpeed(float speed) {
        _setSpeed(speed);
    }

    @Override
    public int getVideoDecoder() {
        return _getVideoDecoder();
    }

    @Override
    public float getVideoRenderFrameRate() {
        return _getVideoRenderFrameRate();
    }

    @Override
    public float getVideoDecodeFrameRate() {
        return _getVideoDecodeFrameRate();
    }

    @Override
    public long getVideoCacheTime() {
        return _getVideoCachedTime();
    }

    @Override
    public long getAudioCacheTime() {
        return _getAudioCachedTime();
    }

    @Override
    public long getVideoCacheSize() {
        return _getVideoCachedSize();
    }

    @Override
    public long getAudioCacheSize() {
        return _getAudioCachedSize();
    }

    @Override
    public long getFileSize() {
        return _getFileSize();
    }

    @Override
    public long getBitRate() {
        return _getBitRate();
    }

    @Override
    public long getSeekCostTime() {
        return _getSeekCostTime();
    }

    @Override
    public int getPlayerState() {
        return _getPlayerState();
    }

    private static class EventHandler extends Handler {
        private final WeakReference<NextPlayer> mWeakPlayer;

        public EventHandler(NextPlayer mp, Looper looper) {
            super(looper);
            mWeakPlayer = new WeakReference<>(mp);
        }

        @Override
        public void handleMessage(@NonNull Message msg) {
            NextPlayer player = mWeakPlayer.get();
            if (player == null) {
                return;
            }

            switch (msg.what) {
                case MSG_ON_PREPARED:
                    player.onPrepared();
                    return;

                case MSG_ON_COMPLETED:
                    player.onComplete();
                    return;

                case MSG_BUFFER_UPDATE:
                    return;

                case MSG_SEEK_COMPLETED: {
                    player.onSeekComplete();
                    player.onInfo(MSG_SEEK_COMPLETED, msg.arg2);
                    return;
                }

                case MSG_SET_VIDEO_SIZE:
                    player.mVideoWidth  = msg.arg1;
                    player.mVideoHeight = msg.arg2;
                    player.onVideoSizeChanged(player.mVideoWidth, player.mVideoHeight,
                            player.mVideoSarNum, player.mVideoSarDen);
                    return;

                case MSG_ON_ERROR:
                    player.onError(msg.arg1, msg.arg2);
                    if (msg.arg2 >= 0) {
                        player.onComplete();
                    }
                    return;

                case MSG_MEDIA_INFO:
                    switch (msg.arg1) {
                        case MSG_VIDEO_RENDER_START:
                            break;
                        case MSG_PLAY_URL_CHANGED:
                            player.onInfo(msg.arg1, (Integer) msg.obj);
                            return;
                    }
                    player.onInfo(msg.arg1, msg.arg2);
                    return;

                case MSG_SET_VIDEO_SAR:
                    player.mVideoSarNum = msg.arg1;
                    player.mVideoSarDen = msg.arg2;
                    player.onVideoSizeChanged(player.mVideoWidth, player.mVideoHeight,
                            player.mVideoSarNum, player.mVideoSarDen);
                    break;

                default:
                    break;
            }
        }
    }

    private static void postEventFromNative(Object weakThiz, int what, int arg1, int arg2, Object obj) {
        if (weakThiz == null)
            return;

        NextPlayer mp = (NextPlayer) ((WeakReference<?>) weakThiz).get();
        if (mp == null) {
            return;
        }

        if (mp.mEventHandler != null) {
            Message m = mp.mEventHandler.obtainMessage(what, arg1, arg2);
            mp.mEventHandler.sendMessage(m);
        }
    }

    private static void onNativeLog(int level, String tag, byte[] logContent) {
        if (level < gLogCallBackLevel) {
            return;
        }
        String logStr = new String(logContent, StandardCharsets.UTF_8);
        if (mNativeLogListener != null) {
            mNativeLogListener.onLogOutput(level, tag, logStr);
        } else {
            Log.println(level, tag, logStr);
        }
    }

    private static void log(int level, String log) {
        if (level < gLogCallBackLevel) {
            return;
        }
        if (mNativeLogListener != null) {
            mNativeLogListener.onLogOutput(level, TAG, log);
        } else {
            Log.println(level, TAG, log);
        }
    }

    public static void setNativeLogCallback(NativeLogCallback nativeLogCallback) {
        mNativeLogListener = nativeLogCallback;
    }

    public interface NativeLogCallback {
        void onLogOutput(int logLevel, String tag, String log);
    }


    private static native void native_init();

    private native void native_setup(Object player);

    private native void _setVideoSurface(Surface surface) throws IllegalStateException;

    private native void _setHeaders(String headers);

    private native void _setDataSource(String path)
            throws IOException, IllegalArgumentException, SecurityException, IllegalStateException;

    private native void _prepareAsync() throws IllegalStateException;

    private native void _start() throws IllegalStateException;

    private native void _stop() throws IllegalStateException;

    private native void _pause() throws IllegalStateException;

    private native void _setEnableMediaCodec(boolean enable);

    private native void _setVideoCacheDir(String dir);

    private native String _getAudioCodecInfo();

    private native String _getVideoCodecInfo();

    private native float _getVideoFileFps();

    private native boolean _playing();

    private native void _seekTo(long msec) throws IllegalStateException;

    private native long _getCurrentPosition();

    private native long _getDuration();

    private native void _reset();

    private native void _setVolume(float leftVolume, float rightVolume);

    private native String _getPlayUrl();

    private native void _setSpeed(float speed);

    private native int _getVideoDecoder();

    private native float _getVideoRenderFrameRate();

    private native float _getVideoDecodeFrameRate();

    private native long _getVideoCachedTime(); // ms

    private native long _getAudioCachedTime();

    private native long _getVideoCachedSize(); // byte

    private native long _getAudioCachedSize();

    private native long _getFileSize();

    private native long _getBitRate();

    private native long _getSeekCostTime();

    private native int _getPlayerState();

    private native void _release();

}
