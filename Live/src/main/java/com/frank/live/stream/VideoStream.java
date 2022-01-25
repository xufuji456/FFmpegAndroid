package com.frank.live.stream;

import android.app.Activity;
import android.hardware.Camera;
import android.view.SurfaceHolder;

import com.frank.live.listener.OnFrameDataCallback;


public class VideoStream implements Camera.PreviewCallback, CameraHelper.OnChangedSizeListener {


    private final OnFrameDataCallback mCallback;
    private final CameraHelper cameraHelper;
    private final int mBitrate;
    private final int mFrameRate;
    private boolean isLiving;

    public VideoStream(OnFrameDataCallback callback,
                       Activity activity,
                       int width,
                       int height,
                       int bitrate,
                       int frameRate,
                       int cameraId) {
        mCallback = callback;
        mBitrate  = bitrate;
        mFrameRate = frameRate;
        cameraHelper = new CameraHelper(activity, cameraId, width, height);
        cameraHelper.setPreviewCallback(this);
        cameraHelper.setOnChangedSizeListener(this);
    }

    public void setPreviewDisplay(SurfaceHolder surfaceHolder) {
        cameraHelper.setPreviewDisplay(surfaceHolder);
    }


    /**
     * preview data
     *
     * @param data   data
     * @param camera camera
     */
    @Override
    public void onPreviewFrame(byte[] data, Camera camera) {
        if (isLiving && mCallback != null) {
            mCallback.onVideoFrame(data, null, null, null);
        }
    }

    public void switchCamera() {
        cameraHelper.switchCamera();
    }

    @Override
    public void onChanged(int w, int h) {
        if (mCallback != null) {
            mCallback.onVideoCodecInfo(w, h, mFrameRate, mBitrate);
        }
    }

    public void startLive() {
        isLiving = true;
    }

    public void stopLive() {
        isLiving = false;
    }

    public void release() {
        cameraHelper.release();
    }
}
