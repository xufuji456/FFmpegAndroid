package com.frank.next.player;

/**
 * Note: base of player
 * Date: 2026/1/8
 * Author: frank
 */
public abstract class BasePlayer implements IPlayer {

    private OnInfoListener             mOnInfoListener;
    private OnErrorListener            mOnErrorListener;
    private OnCompleteListener         mOnCompleteListener;
    private OnPreparedListener         mOnPreparedListener;
    private OnBufferUpdateListener     mOnBufferUpdateListener;
    private OnSeekCompleteListener     mOnSeekCompleteListener;
    private OnVideoSizeChangedListener mOnVideoSizeChangedListener;

    public BasePlayer() {
    }

    @Override
    public final void setOnPreparedListener(IPlayer.OnPreparedListener listener) {
        this.mOnPreparedListener = listener;
    }

    @Override
    public final void setOnCompletionListener(OnCompleteListener listener) {
        this.mOnCompleteListener = listener;
    }

    @Override
    public final void setOnBufferingUpdateListener(OnBufferUpdateListener listener) {
        this.mOnBufferUpdateListener = listener;
    }

    @Override
    public final void setOnSeekCompleteListener(IPlayer.OnSeekCompleteListener listener) {
        this.mOnSeekCompleteListener = listener;
    }

    @Override
    public final void setOnVideoSizeChangedListener(IPlayer.OnVideoSizeChangedListener listener) {
        this.mOnVideoSizeChangedListener = listener;
    }

    @Override
    public final void setOnErrorListener(IPlayer.OnErrorListener listener) {
        this.mOnErrorListener = listener;
    }

    @Override
    public final void setOnInfoListener(IPlayer.OnInfoListener listener) {
        this.mOnInfoListener = listener;
    }

    public void resetListeners() {
        this.mOnInfoListener             = null;
        this.mOnErrorListener            = null;
        this.mOnPreparedListener         = null;
        this.mOnCompleteListener         = null;
        this.mOnSeekCompleteListener     = null;
        this.mOnBufferUpdateListener     = null;
        this.mOnVideoSizeChangedListener = null;
    }

    protected final void onPrepared() {
        if (this.mOnPreparedListener != null) {
            this.mOnPreparedListener.onPrepared(this);
        }
    }

    protected void onVideoSizeChanged(int width, int height, int sarNum, int sarDen) {
        if (this.mOnVideoSizeChangedListener != null) {
            this.mOnVideoSizeChangedListener.onVideoSizeChanged( width, height);
        }
    }

    protected void onInfo(int what, int extra) {
        if (mOnInfoListener != null) {
            mOnInfoListener.onInfo(what, extra);
        }
    }

    protected void onError(int what, int extra) {
        if (this.mOnErrorListener != null) {
            this.mOnErrorListener.onError(what, extra);
        }
    }

    protected void onSeekComplete() {
        if (this.mOnSeekCompleteListener != null) {
            this.mOnSeekCompleteListener.onSeekComplete(this);
        }
    }

    protected final void onComplete() {
        if (this.mOnCompleteListener != null) {
            this.mOnCompleteListener.onComplete(this);
        }
    }

}
