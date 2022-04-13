package com.frank.live.camera;

import android.Manifest;
import android.annotation.TargetApi;
import android.content.Context;
import android.content.pm.PackageManager;
import android.graphics.ImageFormat;
import android.graphics.Matrix;
import android.graphics.Point;
import android.graphics.RectF;
import android.graphics.SurfaceTexture;
import android.hardware.camera2.CameraAccessException;
import android.hardware.camera2.CameraCaptureSession;
import android.hardware.camera2.CameraCharacteristics;
import android.hardware.camera2.CameraDevice;
import android.hardware.camera2.CameraManager;
import android.hardware.camera2.CaptureRequest;
import android.hardware.camera2.params.StreamConfigurationMap;
import android.media.Image;
import android.media.ImageReader;
import android.os.Build;
import android.os.Handler;
import android.os.HandlerThread;
import android.util.Log;
import android.util.Size;
import android.view.Surface;
import android.view.TextureView;

import java.nio.ByteBuffer;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import java.util.concurrent.Semaphore;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.locks.ReentrantLock;

import androidx.annotation.NonNull;

import com.frank.live.util.YUVUtil;

/**
 * Camera2: open, preview and close
 * Created by frank on 2019/12/18.
 */
@TargetApi(21)
public class Camera2Helper {

    private static final String TAG = Camera2Helper.class.getSimpleName();

    public static final String CAMERA_ID_FRONT = "1";
    public static final String CAMERA_ID_BACK = "0";

    private Context context;
    private String mCameraId;
    private String specificCameraId;
    private TextureView mTextureView;
    private final int rotation;
    private final Point previewViewSize;
    private Camera2Listener camera2Listener;
    /**
     * A {@link CameraCaptureSession } for camera preview.
     */
    private CameraCaptureSession mCaptureSession;

    /**
     * A reference to the opened {@link CameraDevice}.
     */
    private CameraDevice mCameraDevice;

    private Size mPreviewSize;

    private int rotateDegree = 0;

    private Camera2Helper(Builder builder) {
        mTextureView = builder.previewDisplayView;
        specificCameraId = builder.specificCameraId;
        camera2Listener = builder.camera2Listener;
        rotation = builder.rotation;
        rotateDegree = builder.rotateDegree;
        previewViewSize = builder.previewViewSize;
        context = builder.context;
    }

    public void switchCamera() {
        if (CAMERA_ID_BACK.equals(mCameraId)) {
            specificCameraId = CAMERA_ID_FRONT;
        } else if (CAMERA_ID_FRONT.equals(mCameraId)) {
            specificCameraId = CAMERA_ID_BACK;
        }
        stop();
        start();
    }

    private int getCameraOrientation(int rotation, String cameraId) {
        int degree = rotation * 90;
        switch (rotation) {
            case Surface.ROTATION_0:
                degree = 0;
                break;
            case Surface.ROTATION_90:
                degree = 90;
                break;
            case Surface.ROTATION_180:
                degree = 180;
                break;
            case Surface.ROTATION_270:
                degree = 270;
                break;
            default:
                break;
        }
        int result;
        if (CAMERA_ID_FRONT.equals(cameraId)) {
            result = (mSensorOrientation + degree) % 360;
            result = (360 - result) % 360;
        } else {
            result = (mSensorOrientation - degree + 360) % 360;
        }
        Log.i(TAG, "getCameraOrientation, result=" + result);
        return result;
    }

    private final TextureView.SurfaceTextureListener mSurfaceTextureListener
            = new TextureView.SurfaceTextureListener() {

        @Override
        public void onSurfaceTextureAvailable(SurfaceTexture texture, int width, int height) {
            Log.i(TAG, "onSurfaceTextureAvailable...");
            openCamera();
        }

        @Override
        public void onSurfaceTextureSizeChanged(SurfaceTexture texture, int width, int height) {
            Log.i(TAG, "onSurfaceTextureSizeChanged, width=" + width + "--height=" + height);
            configureTransform(width, height);
        }

        @Override
        public boolean onSurfaceTextureDestroyed(SurfaceTexture texture) {
            Log.i(TAG, "onSurfaceTextureDestroyed...");
            return true;
        }

        @Override
        public void onSurfaceTextureUpdated(SurfaceTexture texture) {
        }

    };

    private final CameraDevice.StateCallback mDeviceStateCallback = new CameraDevice.StateCallback() {

        @Override
        public void onOpened(@NonNull CameraDevice cameraDevice) {
            Log.i(TAG, "onOpened: ");
            mCameraOpenCloseLock.release();
            mCameraDevice = cameraDevice;
            createCameraPreviewSession();
            if (camera2Listener != null) {
                camera2Listener.onCameraOpened(mPreviewSize, getCameraOrientation(rotation, mCameraId));
            }
        }

        @Override
        public void onDisconnected(@NonNull CameraDevice cameraDevice) {
            Log.i(TAG, "onDisconnected: ");
            mCameraOpenCloseLock.release();
            cameraDevice.close();
            mCameraDevice = null;
            if (camera2Listener != null) {
                camera2Listener.onCameraClosed();
            }
        }

        @Override
        public void onError(@NonNull CameraDevice cameraDevice, int error) {
            Log.i(TAG, "onError: ");
            mCameraOpenCloseLock.release();
            cameraDevice.close();
            mCameraDevice = null;

            if (camera2Listener != null) {
                camera2Listener.onCameraError(new Exception("error occurred, code is " + error));
            }
        }

    };

    private final CameraCaptureSession.StateCallback mCaptureStateCallback = new CameraCaptureSession.StateCallback() {

        @Override
        public void onConfigured(@NonNull CameraCaptureSession cameraCaptureSession) {
            Log.i(TAG, "onConfigured: ");
            // The camera is already closed
            if (null == mCameraDevice) {
                return;
            }

            // When the session is ready, we start displaying the preview.
            mCaptureSession = cameraCaptureSession;
            try {
                mCaptureSession.setRepeatingRequest(mPreviewRequestBuilder.build(),
                        new CameraCaptureSession.CaptureCallback() {
                        }, mBackgroundHandler);
            } catch (CameraAccessException e) {
                e.printStackTrace();
            }
        }

        @Override
        public void onConfigureFailed(
                @NonNull CameraCaptureSession cameraCaptureSession) {
            Log.i(TAG, "onConfigureFailed: ");
            if (camera2Listener != null) {
                camera2Listener.onCameraError(new Exception("configureFailed"));
            }
        }
    };
    /**
     * An additional thread for running tasks that shouldn't block the UI.
     */
    private HandlerThread mBackgroundThread;

    /**
     * A {@link Handler} for running tasks in the background.
     */
    private Handler mBackgroundHandler;

    private ImageReader mImageReader;


    /**
     * {@link CaptureRequest.Builder} for the camera preview
     */
    private CaptureRequest.Builder mPreviewRequestBuilder;


    /**
     * A {@link Semaphore} to prevent the app from exiting before closing the camera.
     */
    private final Semaphore mCameraOpenCloseLock = new Semaphore(1);

    /**
     * Orientation of the camera sensor
     */
    private int mSensorOrientation;

    private Size getBestSupportedSize(List<Size> sizes) {
        Size defaultSize = sizes.get(0);
        Log.e(TAG, "default width=" + defaultSize.getWidth() + "--height=" + defaultSize.getHeight());
        int defaultDelta = Math.abs(defaultSize.getWidth() * defaultSize.getHeight() - previewViewSize.x * previewViewSize.y);
        for (Size size : sizes) {
            Log.e(TAG, "current width=" + defaultSize.getWidth() + "--height=" + defaultSize.getHeight());
            int currentDelta = Math.abs(size.getWidth() * size.getHeight() - previewViewSize.x * previewViewSize.y);
            if (currentDelta < defaultDelta) {
                defaultDelta = currentDelta;
                defaultSize = size;
            }
        }
        Log.e(TAG, "final width=" + defaultSize.getWidth() + "--height=" + defaultSize.getHeight());
        return defaultSize;
    }

    public synchronized void start() {
        if (mCameraDevice != null) {
            return;
        }
        startBackgroundThread();

        // When the screen is turned off and turned back on, the SurfaceTexture is already
        // available, and "onSurfaceTextureAvailable" will not be called. In that case, we can open
        // a camera and start preview from here (otherwise, we wait until the surface is ready in
        // the SurfaceTextureListener).
        if (mTextureView.isAvailable()) {
            openCamera();
        } else {
            mTextureView.setSurfaceTextureListener(mSurfaceTextureListener);
        }
    }

    public void updatePreviewDegree(int degree) {
        rotateDegree = degree;
    }

    public synchronized void stop() {
        if (mCameraDevice == null) {
            return;
        }
        closeCamera();
        stopBackgroundThread();
    }

    public void release() {
        stop();
        mTextureView = null;
        camera2Listener = null;
        context = null;
    }

    private void setUpCameraOutput(CameraManager cameraManager) {
        try {
            if (configCameraParams(cameraManager, specificCameraId)) {
                return;
            }
            for (String cameraId : cameraManager.getCameraIdList()) {
                if (configCameraParams(cameraManager, cameraId)) {
                    return;
                }
            }
        } catch (CameraAccessException e) {
            e.printStackTrace();
        } catch (NullPointerException e) {
            // Currently an NPE is thrown when the Camera2API is used but not supported on the
            // device this code runs.

            if (camera2Listener != null) {
                camera2Listener.onCameraError(e);
            }
        }
    }

    private boolean configCameraParams(CameraManager manager, String cameraId) throws CameraAccessException {
        CameraCharacteristics characteristics
                = manager.getCameraCharacteristics(cameraId);

        StreamConfigurationMap map = characteristics.get(
                CameraCharacteristics.SCALER_STREAM_CONFIGURATION_MAP);
        if (map == null) {
            return false;
        }
        mPreviewSize = getBestSupportedSize(new ArrayList<>(Arrays.asList(map.getOutputSizes(SurfaceTexture.class))));
        mImageReader = ImageReader.newInstance(mPreviewSize.getWidth(), mPreviewSize.getHeight(),
                ImageFormat.YUV_420_888, 2);
        mImageReader.setOnImageAvailableListener(
                new OnImageAvailableListenerImpl(), mBackgroundHandler);

        mSensorOrientation = characteristics.get(CameraCharacteristics.SENSOR_ORIENTATION);
        mCameraId = cameraId;
        return true;
    }

    private void openCamera() {
        CameraManager cameraManager = (CameraManager) context.getSystemService(Context.CAMERA_SERVICE);
        setUpCameraOutput(cameraManager);
        configureTransform(mTextureView.getWidth(), mTextureView.getHeight());
        try {
            if (!mCameraOpenCloseLock.tryAcquire(2500, TimeUnit.MILLISECONDS)) {
                throw new RuntimeException("Time out waiting to lock camera opening.");
            }
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M
                    && context.checkSelfPermission(Manifest.permission.CAMERA) != PackageManager.PERMISSION_GRANTED) {
                return;
            }

            cameraManager.openCamera(mCameraId, mDeviceStateCallback, mBackgroundHandler);
        } catch (CameraAccessException | InterruptedException e) {
            if (camera2Listener != null) {
                camera2Listener.onCameraError(e);
            }
        }
    }

    /**
     * Closes the current {@link CameraDevice}.
     */
    private void closeCamera() {
        try {
            mCameraOpenCloseLock.acquire();
            if (null != mCaptureSession) {
                mCaptureSession.close();
                mCaptureSession = null;
            }
            if (null != mCameraDevice) {
                mCameraDevice.close();
                mCameraDevice = null;
            }
            if (null != mImageReader) {
                mImageReader.close();
                mImageReader = null;
            }
            if (camera2Listener != null) {
                camera2Listener.onCameraClosed();
            }
        } catch (InterruptedException e) {
            if (camera2Listener != null) {
                camera2Listener.onCameraError(e);
            }
        } finally {
            mCameraOpenCloseLock.release();
        }
    }

    /**
     * Starts a background thread and its {@link Handler}.
     */
    private void startBackgroundThread() {
        mBackgroundThread = new HandlerThread("CameraBackground");
        mBackgroundThread.start();
        mBackgroundHandler = new Handler(mBackgroundThread.getLooper());
    }

    /**
     * Stops the background thread and its {@link Handler}.
     */
    private void stopBackgroundThread() {
        mBackgroundThread.quitSafely();
        try {
            mBackgroundThread.join();
            mBackgroundThread = null;
            mBackgroundHandler = null;
        } catch (InterruptedException e) {
            e.printStackTrace();
        }
    }

    /**
     * Creates a new {@link CameraCaptureSession} for camera preview.
     */
    private void createCameraPreviewSession() {
        try {
            SurfaceTexture texture = mTextureView.getSurfaceTexture();
            assert texture != null;

            // We configure the size of default buffer to be the size of camera preview we want.
            texture.setDefaultBufferSize(mPreviewSize.getWidth(), mPreviewSize.getHeight());

            // This is the output Surface we need to start preview.
            Surface surface = new Surface(texture);

            // We set up a CaptureRequest.Builder with the output Surface.
            mPreviewRequestBuilder
                    = mCameraDevice.createCaptureRequest(CameraDevice.TEMPLATE_PREVIEW);

            mPreviewRequestBuilder.set(CaptureRequest.CONTROL_AF_MODE,
                    CaptureRequest.CONTROL_AF_MODE_CONTINUOUS_PICTURE);

            mPreviewRequestBuilder.addTarget(surface);
            mPreviewRequestBuilder.addTarget(mImageReader.getSurface());

            // Here, we create a CameraCaptureSession for camera preview.
            mCameraDevice.createCaptureSession(Arrays.asList(surface, mImageReader.getSurface()),
                    mCaptureStateCallback, mBackgroundHandler
            );
        } catch (CameraAccessException e) {
            e.printStackTrace();
        }
    }

    /**
     * Configures the necessary {@link Matrix} transformation to `mTextureView`.
     * This method should be called after the camera preview size is determined in
     * setUpCameraOutputs and also the size of `mTextureView` is fixed.
     *
     * @param viewWidth  The width of `mTextureView`
     * @param viewHeight The height of `mTextureView`
     */
    private void configureTransform(int viewWidth, int viewHeight) {
        if (null == mTextureView || null == mPreviewSize) {
            return;
        }
        Matrix matrix = new Matrix();
        RectF viewRect = new RectF(0, 0, viewWidth, viewHeight);
        RectF bufferRect = new RectF(0, 0, mPreviewSize.getHeight(), mPreviewSize.getWidth());
        float centerX = viewRect.centerX();
        float centerY = viewRect.centerY();
        if (Surface.ROTATION_90 == rotation || Surface.ROTATION_270 == rotation) {
            bufferRect.offset(centerX - bufferRect.centerX(), centerY - bufferRect.centerY());
            matrix.setRectToRect(viewRect, bufferRect, Matrix.ScaleToFit.FILL);
            float scale = Math.max(
                    (float) viewHeight / mPreviewSize.getHeight(),
                    (float) viewWidth / mPreviewSize.getWidth());
            matrix.postScale(scale, scale, centerX, centerY);
            matrix.postRotate((90 * (rotation - 2)) % 360, centerX, centerY);
        } else if (Surface.ROTATION_180 == rotation) {
            matrix.postRotate(180, centerX, centerY);
        }
        mTextureView.setTransform(matrix);
    }

    public static final class Builder {

        private TextureView previewDisplayView;

        private String specificCameraId;

        private Camera2Listener camera2Listener;

        private Point previewViewSize;

        private int rotation;

        private int rotateDegree;

        private Context context;

        public Builder() {
        }

        public Builder previewOn(TextureView val) {
            previewDisplayView = val;
            return this;
        }

        public Builder previewViewSize(Point val) {
            previewViewSize = val;
            return this;
        }

        public Builder rotation(int val) {
            rotation = val;
            return this;
        }

        public Builder rotateDegree(int val) {
            rotateDegree = val;
            return this;
        }

        public Builder specificCameraId(String val) {
            specificCameraId = val;
            return this;
        }

        public Builder cameraListener(Camera2Listener val) {
            camera2Listener = val;
            return this;
        }

        public Builder context(Context val) {
            context = val;
            return this;
        }

        public Camera2Helper build() {
            if (previewDisplayView == null) {
                throw new NullPointerException("must preview on a textureView or a surfaceView");
            }
            return new Camera2Helper(this);
        }
    }

    private class OnImageAvailableListenerImpl implements ImageReader.OnImageAvailableListener {
        private byte[] temp = null;
        private byte[] yuvData = null;
        private byte[] dstData = null;
        private final ReentrantLock lock = new ReentrantLock();

        @Override
        public void onImageAvailable(ImageReader reader) {
            Image image = reader.acquireNextImage();
            if (camera2Listener != null && image.getFormat() == ImageFormat.YUV_420_888) {
                Image.Plane[] planes = image.getPlanes();
                lock.lock();

                int offset = 0;
                int width  = image.getWidth();
                int height = image.getHeight();
                int len    = width * height;
                if (yuvData == null) {
                    yuvData = new byte[len * 3 / 2];
                }
                planes[0].getBuffer().get(yuvData, offset, len);
                offset += len;
                for (int i = 1; i < planes.length; i++) {
                    int srcIndex = 0, dstIndex = 0;
                    int rowStride = planes[i].getRowStride();
                    int pixelsStride = planes[i].getPixelStride();
                    ByteBuffer buffer = planes[i].getBuffer();
                    if (temp == null || temp.length != buffer.capacity()) {
                        temp = new byte[buffer.capacity()];
                    }
                    buffer.get(temp);

                    for (int j = 0; j < height / 2; j++) {
                        for (int k = 0; k < width / 2; k++) {
                            yuvData[offset + dstIndex++] = temp[srcIndex];
                            srcIndex += pixelsStride;
                        }
                        if (pixelsStride == 2) {
                            srcIndex += rowStride - width;
                        } else if (pixelsStride == 1) {
                            srcIndex += rowStride - width / 2;
                        }
                    }
                    offset += len / 4;
                }

                if (rotateDegree == 90) {
                    if (dstData == null) {
                        dstData = new byte[len * 3 / 2];
                    }
                    YUVUtil.YUV420pRotate90(dstData, yuvData, width, height);
                    if (camera2Listener != null) {
                        camera2Listener.onPreviewFrame(dstData);
                    }
                } else {
                    if (camera2Listener != null) {
                        camera2Listener.onPreviewFrame(yuvData);
                    }
                }

                lock.unlock();
            }
            image.close();
        }
    }

}
