package com.frank.camerafilter.camera;

import android.graphics.SurfaceTexture;
import android.hardware.Camera;
import android.hardware.Camera.Parameters;

import java.io.IOException;
import java.util.List;

/**
 * @author xufulong
 * @date 2022/6/17 5:14 下午
 * @desc
 */
public class CameraManager {

    private Camera mCamera;
    private int mCameraId = 0;
    private SurfaceTexture mSurfaceTexture;

    public Camera getCamera() {
        return mCamera;
    }

    public boolean openCamera() {
        return openCamera(mCameraId);
    }

    public boolean openCamera(int cameraId) {
        if (mCamera == null) {
            try {
                mCameraId = cameraId;
                mCamera = Camera.open(cameraId);
                setDefaultParams();
                return true;
            } catch (RuntimeException e) {
                return false;
            }
        }
        return false;
    }

    public void releaseCamera() {
        if (mCamera == null)
            return;
        stopPreview();
        mCamera.release();
        mCamera = null;
    }

    public void switchCamera() {
        if (mCameraId == Camera.CameraInfo.CAMERA_FACING_BACK) {
            mCameraId = Camera.CameraInfo.CAMERA_FACING_FRONT;
        } else {
            mCameraId = Camera.CameraInfo.CAMERA_FACING_BACK;
        }
        releaseCamera();
        openCamera(mCameraId);
        startPreview(mSurfaceTexture);
    }

    private static Camera.Size getLargePictureSize(Camera camera){
        if(camera != null){
            List<Camera.Size> sizes = camera.getParameters().getSupportedPictureSizes();
            Camera.Size temp = sizes.get(0);
            for(int i = 1;i < sizes.size();i ++){
                float scale = (float)(sizes.get(i).height) / sizes.get(i).width;
                if(temp.width < sizes.get(i).width && scale < 0.6f && scale > 0.5f)
                    temp = sizes.get(i);
            }
            return temp;
        }
        return null;
    }

    private static Camera.Size getLargePreviewSize(Camera camera){
        if(camera != null){
            List<Camera.Size> sizes = camera.getParameters().getSupportedPreviewSizes();
            Camera.Size temp = sizes.get(0);
            for(int i = 1;i < sizes.size();i ++){
                if(temp.width < sizes.get(i).width)
                    temp = sizes.get(i);
            }
            return temp;
        }
        return null;
    }

    public void setDefaultParams() {
        Parameters parameters = mCamera.getParameters();
        if (parameters.getSupportedFocusModes().contains(
                Camera.Parameters.FOCUS_MODE_CONTINUOUS_PICTURE)) {
            parameters.setFocusMode(Parameters.FOCUS_MODE_CONTINUOUS_PICTURE);
        }
        Camera.Size previewSize = getLargePreviewSize(mCamera);
        parameters.setPreviewSize(previewSize.width, previewSize.height);
        Camera.Size pictureSize = getLargePictureSize(mCamera);
        parameters.setPictureSize(pictureSize.width, pictureSize.height);
        parameters.setRotation(90);
        mCamera.setParameters(parameters);
    }

    public void startPreview(SurfaceTexture surfaceTexture) {
        if (mCamera == null)
            return;
        try {
            mCamera.setPreviewTexture(surfaceTexture);
            mSurfaceTexture = surfaceTexture;
            mCamera.startPreview();
        } catch (IOException e) {
            e.printStackTrace();
        }
    }

    public void stopPreview() {
        if (mCamera == null)
            return;
        mCamera.setPreviewCallback(null);
        mCamera.stopPreview();
    }

    public int getOrientation() {
        Camera.CameraInfo cameraInfo = new Camera.CameraInfo();
        Camera.getCameraInfo(mCameraId, cameraInfo);
        return cameraInfo.orientation;
    }

    public Camera.Size getPreviewSize() {
        return mCamera.getParameters().getPreviewSize();
    }

    public boolean isFront() {
        return mCameraId == Camera.CameraInfo.CAMERA_FACING_FRONT;
    }

}
