package com.frank.camerafilter.widget;

import android.content.Context;
import android.opengl.GLSurfaceView;
import android.util.AttributeSet;
import android.view.SurfaceHolder;

import com.frank.camerafilter.factory.BeautyFilterType;

public class BeautyCameraView extends GLSurfaceView {

    private final CameraRender mCameraRender;

    public BeautyCameraView(Context context) {
        this(context, null);
    }

    public BeautyCameraView(Context context, AttributeSet attrs) {
        super(context, attrs);
        getHolder().addCallback(this);

        mCameraRender = new CameraRender(this);
        setEGLContextClientVersion(3);
        setRenderer(mCameraRender);
        setRenderMode(GLSurfaceView.RENDERMODE_WHEN_DIRTY);
    }

    @Override
    public void surfaceDestroyed(SurfaceHolder holder) {
        super.surfaceDestroyed(holder);
        if (mCameraRender != null) {
            mCameraRender.releaseCamera();
        }
    }

    public void switchCamera() {
        if (mCameraRender != null) {
            mCameraRender.switchCamera();
        }
    }

    public void setFilter(BeautyFilterType type) {
        if (mCameraRender == null)
            return;
        mCameraRender.setFilter(type);
    }

    public void setRecording(boolean isRecording) {
        if (mCameraRender == null)
            return;
        mCameraRender.setRecording(isRecording);
    }

    public boolean isRecording() {
        if (mCameraRender == null)
            return false;
        return mCameraRender.isRecording();
    }

}
