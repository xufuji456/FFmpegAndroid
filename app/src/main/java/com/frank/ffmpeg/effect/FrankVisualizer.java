package com.frank.ffmpeg.effect;

import java.nio.ByteBuffer;

/**
 * @author frank
 * @date 2021/10/1 11:25
 * @desc custom audio visualizer
 */
public class FrankVisualizer {

    private long mNativeVisualizer;

    private static OnFftDataListener mOnFftDataListener;

    public FrankVisualizer() {}

    public void setOnFftDataListener(OnFftDataListener onFftDataListener) {
        mOnFftDataListener = onFftDataListener;
    }

    public int initVisualizer() {
        return nativeInitVisualizer();
    }

    public void captureData(ByteBuffer data, int size) {
        if (data == null || size <= 0) {
            nativeCaptureData(data, size);
        }
    }

    public void releaseVisualizer() {
        mOnFftDataListener = null;
        nativeReleaseVisualizer();
    }

    public interface OnFftDataListener {
        void onFftData(byte[] data);
    }

    public static void onFftCallback(byte[] data) {
        if (mOnFftDataListener != null) {
            mOnFftDataListener.onFftData(data);
        }
    }

    private native int nativeInitVisualizer();

    private native int nativeCaptureData(ByteBuffer buffer, int size);

    private native void nativeReleaseVisualizer();

}
