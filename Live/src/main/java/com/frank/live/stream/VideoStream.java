package com.frank.live.stream;

import android.app.Activity;
import android.content.Context;
import android.hardware.Camera;
import android.view.SurfaceHolder;
import android.view.View;

import com.frank.live.camera.CameraHelper;
import com.frank.live.listener.OnFrameDataCallback;
import com.frank.live.param.VideoParam;

public class VideoStream extends VideoStreamBase implements Camera.PreviewCallback,
        CameraHelper.OnChangedSizeListener {

    private final OnFrameDataCallback mCallback;
    private final CameraHelper cameraHelper;
    private final int mBitrate;
    private final int mFrameRate;
    private boolean isLiving;

    public VideoStream(OnFrameDataCallback callback,
                       View view,
                       VideoParam videoParam,
                       Context context) {
        mCallback    = callback;
        mBitrate     = videoParam.getBitRate();
        mFrameRate   = videoParam.getFrameRate();
        cameraHelper = new CameraHelper((Activity) context,
                                        videoParam.getCameraId(),
                                        videoParam.getWidth(),
                                        videoParam.getHeight());
        cameraHelper.setPreviewCallback(this);
        cameraHelper.setOnChangedSizeListener(this);
    }

    @Override
    public void setPreviewDisplay(SurfaceHolder surfaceHolder) {
        cameraHelper.setPreviewDisplay(surfaceHolder);
    }

    @Override
    public void switchCamera() {
        cameraHelper.switchCamera();
    }

    @Override
    public void startLive() {
        isLiving = true;
    }

    @Override
    public void stopLive() {
        isLiving = false;
    }

    @Override
    public void release() {
        cameraHelper.release();
    }

    @Override
    public void onPreviewFrame(byte[] data, Camera camera) {
        if (isLiving && mCallback != null) {
            mCallback.onVideoFrame(data, null, null, null);
        }
    }

    @Override
    public void onChanged(int w, int h) {
        if (mCallback != null) {
            mCallback.onVideoCodecInfo(w, h, mFrameRate, mBitrate);
        }
    }

}
