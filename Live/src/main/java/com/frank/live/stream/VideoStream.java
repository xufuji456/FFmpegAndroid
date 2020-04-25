package com.frank.live.stream;

import android.app.Activity;
import android.hardware.Camera;
import android.view.SurfaceHolder;

import com.frank.live.LivePusherNew;


public class VideoStream implements Camera.PreviewCallback, CameraHelper.OnChangedSizeListener {


    private LivePusherNew mLivePusher;
    private CameraHelper cameraHelper;
    private int mBitrate;
    private int mFps;
    private boolean isLiving;

    public VideoStream(LivePusherNew livePusher, Activity activity, int width, int height, int bitrate, int fps, int cameraId) {
        mLivePusher = livePusher;
        mBitrate = bitrate;
        mFps = fps;
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
        if (isLiving) {
            mLivePusher.pushVideo(data);
        }
    }

    public void switchCamera() {
        cameraHelper.switchCamera();
    }

    @Override
    public void onChanged(int w, int h) {
        mLivePusher.setVideoCodecInfo(w, h, mFps, mBitrate);
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
