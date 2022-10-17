package com.frank.camerafilter.widget;

import android.graphics.SurfaceTexture;
import android.hardware.Camera;
import android.opengl.EGL14;
import android.opengl.GLES30;
import android.opengl.GLSurfaceView;
import android.os.Environment;

import com.frank.camerafilter.camera.CameraManager;
import com.frank.camerafilter.filter.BeautyCameraFilter;
import com.frank.camerafilter.filter.BaseFilter;
import com.frank.camerafilter.factory.BeautyFilterFactory;
import com.frank.camerafilter.factory.BeautyFilterType;
import com.frank.camerafilter.recorder.video.TextureVideoRecorder;
import com.frank.camerafilter.util.OpenGLUtil;
import com.frank.camerafilter.util.Rotation;
import com.frank.camerafilter.util.TextureRotateUtil;

import java.io.File;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.FloatBuffer;

import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.opengles.GL10;

public class CameraRender implements GLSurfaceView.Renderer, SurfaceTexture.OnFrameAvailableListener {

    protected BaseFilter mFilter;

    private SurfaceTexture surfaceTexture;
    private BeautyCameraFilter cameraFilter;

    private final CameraManager cameraManager;

    protected int mTextureId = OpenGLUtil.NO_TEXTURE;

    protected FloatBuffer mVertexBuffer;

    protected FloatBuffer mTextureBuffer;

    protected int mImageWidth, mImageHeight;

    protected int mSurfaceWidth, mSurfaceHeight;
    private final float[] mMatrix = new float[16];

    private final BeautyCameraView mCameraView;

    private final File outputFile;
    private int recordStatus;
    protected boolean recordEnable;
    private final TextureVideoRecorder videoRecorder;

    private final static int RECORDING_OFF    = 0;
    private final static int RECORDING_ON     = 1;
    private final static int RECORDING_RESUME = 2;

    private static final int videoBitrate = 6 * 1024 * 1024;
    private static final String videoName = "camera_record.mp4";
    private static final String videoPath = Environment.getExternalStorageDirectory().getPath();

    public CameraRender(BeautyCameraView cameraView) {
        mCameraView = cameraView;

        cameraManager = new CameraManager();
        mVertexBuffer = ByteBuffer.allocateDirect(TextureRotateUtil.VERTEX.length * 4)
                .order(ByteOrder.nativeOrder())
                .asFloatBuffer();
        mVertexBuffer.put(TextureRotateUtil.VERTEX).position(0);
        mTextureBuffer = ByteBuffer.allocateDirect(TextureRotateUtil.TEXTURE_ROTATE_0.length * 4)
                .order(ByteOrder.nativeOrder())
                .asFloatBuffer();
        mTextureBuffer.put(TextureRotateUtil.TEXTURE_ROTATE_0).position(0);

        recordEnable    = false;
        recordStatus    = RECORDING_OFF;
        videoRecorder   = new TextureVideoRecorder(mCameraView.getContext());
        outputFile      = new File(videoPath, videoName);
    }

    private void openCamera() {
        if (cameraManager.getCamera() == null)
            cameraManager.openCamera();
        Camera.Size size = cameraManager.getPreviewSize();
        // rotation=90 or rotation=270, we need to exchange width and height
        if (cameraManager.getOrientation() == 90 || cameraManager.getOrientation() == 270) {
            mImageWidth = size.height;
            mImageHeight = size.width;
        } else {
            mImageWidth = size.width;
            mImageHeight = size.height;
        }
        cameraFilter.onInputSizeChanged(mImageWidth, mImageHeight);
        adjustSize(cameraManager.getOrientation(), cameraManager.isFront(), true);
    }

    @Override
    public void onSurfaceCreated(GL10 gl10, EGLConfig eglConfig) {
        GLES30.glDisable(GL10.GL_DITHER);
        GLES30.glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
        GLES30.glEnable(GL10.GL_CULL_FACE);
        GLES30.glEnable(GL10.GL_DEPTH_TEST);

        cameraFilter = new BeautyCameraFilter(mCameraView.getContext());
        cameraFilter.init();
        mTextureId = OpenGLUtil.getExternalOESTextureId();
        if (mTextureId != OpenGLUtil.NO_TEXTURE) {
            surfaceTexture = new SurfaceTexture(mTextureId);
            surfaceTexture.setOnFrameAvailableListener(this);
        }

        openCamera();
    }

    @Override
    public void onSurfaceChanged(GL10 gl10, int width, int height) {
        GLES30.glViewport(0, 0, width, height);
        mSurfaceWidth = width;
        mSurfaceHeight = height;
        cameraManager.startPreview(surfaceTexture);
        onFilterChanged();
    }

    @Override
    public void onDrawFrame(GL10 gl10) {
        GLES30.glClear(GLES30.GL_COLOR_BUFFER_BIT | GLES30.GL_DEPTH_BUFFER_BIT);
        GLES30.glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

        surfaceTexture.updateTexImage();

        surfaceTexture.getTransformMatrix(mMatrix);
        cameraFilter.setTextureTransformMatrix(mMatrix);
        int id = mTextureId;
        if (mFilter == null) {
            cameraFilter.onDrawFrame(mTextureId, mVertexBuffer, mTextureBuffer);
        } else {
            id = cameraFilter.onDrawToTexture(mTextureId);
            mFilter.onDrawFrame(id, mVertexBuffer, mTextureBuffer);
        }

        onRecordVideo(id);
    }

    @Override
    public void onFrameAvailable(SurfaceTexture surfaceTexture) {
        mCameraView.requestRender();
    }

    public void adjustSize(int rotation, boolean horizontalFlip, boolean verticalFlip) {
        float[] vertexData = TextureRotateUtil.VERTEX;
        float[] textureData = TextureRotateUtil.getRotateTexture(Rotation.fromInt(rotation),
                horizontalFlip, verticalFlip);

        mVertexBuffer.clear();
        mVertexBuffer.put(vertexData).position(0);
        mTextureBuffer.clear();
        mTextureBuffer.put(textureData).position(0);

    }

    public void switchCamera() {
        if (cameraManager != null) {
            cameraManager.switchCamera();
        }
    }

    public void releaseCamera() {
        if (cameraManager != null) {
            cameraManager.releaseCamera();
        }
    }

    private void onRecordVideo(int textureId) {
        if (recordEnable) {
            switch (recordStatus) {
                case RECORDING_OFF:
                    videoRecorder.setPreviewSize(mImageWidth, mImageHeight);
                    videoRecorder.setTextureBuffer(mTextureBuffer);
                    videoRecorder.setCubeBuffer(mVertexBuffer);
                    videoRecorder.startRecording(new TextureVideoRecorder.RecorderConfig(
                            mImageWidth,
                            mImageHeight,
                            videoBitrate,
                            outputFile,
                            EGL14.eglGetCurrentContext()));
                    recordStatus = RECORDING_ON;
                    break;
                case RECORDING_RESUME:
                    videoRecorder.updateSharedContext(EGL14.eglGetCurrentContext());
                    recordStatus = RECORDING_ON;
                    break;
                case RECORDING_ON:
                    break;
                default:
                    throw new RuntimeException("unknown status " + recordStatus);
            }
        } else {
            switch (recordStatus) {
                case RECORDING_ON:
                case RECORDING_RESUME:
                    videoRecorder.stopRecording();
                    recordStatus = RECORDING_OFF;
                    break;
                case RECORDING_OFF:
                    break;
                default:
                    throw new RuntimeException("unknown status " + recordStatus);
            }
        }
        videoRecorder.setTextureId(textureId);
        videoRecorder.frameAvailable(surfaceTexture);
    }

    public void setRecording(boolean isRecording) {
        recordEnable = isRecording;
    }

    public boolean isRecording() {
        return recordEnable;
    }

    public void setFilter(final BeautyFilterType type) {
        mCameraView.queueEvent(new Runnable() {
            @Override
            public void run() {
                if (mFilter != null)
                    mFilter.destroy();
                mFilter = null;
                mFilter = BeautyFilterFactory.getFilter(type, mCameraView.getContext());
                if (mFilter != null)
                    mFilter.init();
                onFilterChanged();
            }
        });
        mCameraView.requestRender();
    }

    public void onFilterChanged() {
        if (mFilter != null) {
            mFilter.onInputSizeChanged(mImageWidth, mImageHeight);
            mFilter.onOutputSizeChanged(mSurfaceWidth, mSurfaceHeight);
        }
        cameraFilter.onOutputSizeChanged(mSurfaceWidth, mSurfaceHeight);
        if (mFilter != null)
            cameraFilter.initFrameBuffer(mImageWidth, mImageHeight);
        else
            cameraFilter.destroyFrameBuffer();
    }

}
