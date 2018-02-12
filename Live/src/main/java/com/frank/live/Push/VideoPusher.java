package com.frank.live.Push;

import android.graphics.ImageFormat;
import android.hardware.Camera;
import android.util.Log;
import android.view.SurfaceHolder;

import com.frank.live.LiveUtil;
import com.frank.live.param.VideoParam;
import java.io.IOException;

/**
 * 视频推流
 * Created by frank on 2018/1/28.
 */

public class VideoPusher extends Pusher implements SurfaceHolder.Callback, Camera.PreviewCallback {

    private SurfaceHolder surfaceHolder;
    private VideoParam videoParam;
    private Camera camera;
    private boolean isPushing;
    private byte[] previewBuffer;
    private LiveUtil liveUtil;

    VideoPusher(SurfaceHolder surfaceHolder, VideoParam videoParam, LiveUtil liveUtil){
        this.surfaceHolder = surfaceHolder;
        this.videoParam = videoParam;
        this.liveUtil = liveUtil;
        surfaceHolder.addCallback(this);
        liveUtil.setVideoParams(videoParam.getWidth(), videoParam.getHeight(),
                videoParam.getBitRate(), videoParam.getFrameRate());
    }


    @Override
    public void startPush() {
        isPushing = true;
    }

    @Override
    public void stopPush() {
        isPushing = false;
    }

    @Override
    public void release() {
        stopPush();
        stopPreview();
    }

    @Override
    public void surfaceCreated(SurfaceHolder holder) {
        startPreview();
    }

    @Override
    public void surfaceChanged(SurfaceHolder holder, int format, int width, int height) {

    }

    @Override
    public void surfaceDestroyed(SurfaceHolder holder) {
        stopPreview();
    }

    /**
     * 开始预览
     */
    private void startPreview() {
        try {
            camera = Camera.open(videoParam.getCameraId());
            Camera.Parameters parameters = camera.getParameters();
            parameters.setPreviewFormat(ImageFormat.NV21);
            parameters.setPictureSize(videoParam.getWidth(), videoParam.getHeight());
            camera.setParameters(parameters);
            camera.setDisplayOrientation(0);//竖屏是90°
            camera.setPreviewDisplay(surfaceHolder);
            camera.startPreview();
            previewBuffer = new byte[videoParam.getWidth() * videoParam.getHeight() * 4];
            camera.addCallbackBuffer(previewBuffer);
            camera.setPreviewCallbackWithBuffer(this);
        } catch (IOException e) {
            e.printStackTrace();
        }
    }

    /**
     * 停止预览
     */
    private void stopPreview() {
        if(camera != null){
            camera.stopPreview();
            camera.setPreviewCallback(null);
            camera.release();
            camera = null;
        }
    }

    /**
     * 切换摄像头
     */
    void switchCamera(){

        if(videoParam.getCameraId() == Camera.CameraInfo.CAMERA_FACING_BACK){
            videoParam.setCameraId(Camera.CameraInfo.CAMERA_FACING_FRONT);
        }else {
            videoParam.setCameraId(Camera.CameraInfo.CAMERA_FACING_BACK);
        }
        //重新开始推流
        stopPreview();
        startPreview();
    }

    @Override
    public void onPreviewFrame(byte[] data, Camera camera) {
        camera.addCallbackBuffer(previewBuffer);
        if(isPushing){
//            Log.i("VideoPusher", "isPushing...");
            liveUtil.pushVideoData(data);
        }
    }

}
