package com.frank.live.Push;

import android.app.Activity;
import android.content.Context;
import android.graphics.Point;
import android.graphics.SurfaceTexture;
import android.util.Log;
import android.util.Size;
import android.view.TextureView;

import com.frank.live.LiveUtil;
import com.frank.live.camera2.Camera2Helper;
import com.frank.live.camera2.Camera2Listener;
import com.frank.live.param.VideoParam;

/**
 * 视频推流:使用Camera2
 * Created by frank on 2019/12/18.
 */

public class VideoPusherNew extends Pusher implements TextureView.SurfaceTextureListener, Camera2Listener {
    private final static String TAG = VideoPusherNew.class.getSimpleName();

    private VideoParam mVideoParam;
    private Camera2Helper camera2Helper;
    private boolean isPushing;
    private LiveUtil mLiveUtil;
    private TextureView mTextureView;
    private Context mContext;

    VideoPusherNew(TextureView textureView, VideoParam videoParam, LiveUtil liveUtil, Context context) {
        this.mTextureView = textureView;
        this.mVideoParam = videoParam;
        this.mLiveUtil = liveUtil;
        this.mContext = context;
        mTextureView.setSurfaceTextureListener(this);
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
        releasePreview();
    }

    /**
     * 开始预览
     */
    private void startPreview() {
        int rotateDegree = 0;
        if (mContext instanceof Activity) {
            rotateDegree = ((Activity) mContext).getWindowManager().getDefaultDisplay().getRotation();
        }
        Log.e(TAG, "preview width=" + mTextureView.getWidth() + "--height=" + mTextureView.getHeight());
        camera2Helper = new Camera2Helper.Builder()
                .cameraListener(this)
                .maxPreviewSize(new Point(1080, 720))
                .minPreviewSize(new Point(mVideoParam.getWidth(), mVideoParam.getHeight()))
                .specificCameraId(Camera2Helper.CAMERA_ID_BACK)
                .context(mContext.getApplicationContext())
                .previewOn(mTextureView)
//                .previewViewSize(new Point(mTextureView.getWidth(), mTextureView.getHeight()))
                .previewViewSize(new Point(mVideoParam.getWidth(), mVideoParam.getHeight()))
                .rotation(rotateDegree)
                .build();
        camera2Helper.start();
    }

    /**
     * 停止预览
     */
    private void stopPreview() {
        if (camera2Helper != null) {
            camera2Helper.stop();
        }
    }

    /**
     * 释放资源
     */
    private void releasePreview() {
        if (camera2Helper != null) {
            camera2Helper.stop();
            camera2Helper.release();
            camera2Helper = null;
        }
    }

    /**
     * 切换摄像头
     */
    void switchCamera() {
        if (camera2Helper != null) {
            camera2Helper.switchCamera();
        }
    }

    @Override
    public void onSurfaceTextureAvailable(SurfaceTexture surface, int width, int height) {
        Log.e(TAG, "onSurfaceTextureAvailable...");
        startPreview();
    }

    @Override
    public void onSurfaceTextureSizeChanged(SurfaceTexture surface, int width, int height) {

    }

    @Override
    public boolean onSurfaceTextureDestroyed(SurfaceTexture surface) {
        Log.e(TAG, "onSurfaceTextureDestroyed...");
        stopPreview();
        return false;
    }

    @Override
    public void onSurfaceTextureUpdated(SurfaceTexture surface) {

    }

    @Override
    public void onPreviewFrame(byte[] y, byte[] u, byte[] v) {
        if (isPushing && mLiveUtil != null) {
            mLiveUtil.pushVideoData(y, u, v);
        }
    }

    @Override
    public void onCameraOpened(Size previewSize, int displayOrientation) {
        Log.e(TAG, "onCameraOpened previewSize=" + previewSize.toString());
    }

    @Override
    public void onCameraClosed() {
        Log.e(TAG, "onCameraClosed");
    }

    @Override
    public void onCameraError(Exception e) {
        Log.e(TAG, "onCameraError=" + e.toString());
    }

}
