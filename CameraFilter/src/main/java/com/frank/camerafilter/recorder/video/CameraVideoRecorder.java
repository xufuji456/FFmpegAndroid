package com.frank.camerafilter.recorder.video;

import android.content.Context;
import android.graphics.SurfaceTexture;
import android.opengl.EGLContext;
import android.os.Handler;
import android.os.Looper;
import android.os.Message;

import androidx.annotation.NonNull;

import com.frank.camerafilter.filter.BeautyCameraFilter;
import com.frank.camerafilter.filter.BaseFilter;
import com.frank.camerafilter.factory.BeautyFilterFactory;
import com.frank.camerafilter.factory.BeautyFilterType;
import com.frank.camerafilter.recorder.gles.EglCore;

import java.io.File;
import java.io.IOException;
import java.lang.ref.WeakReference;
import java.nio.FloatBuffer;

/**
 * Encode a movie from frames rendered from an external texture image.
 * <p>
 * The object wraps an encoder running on a dedicated thread.  The various control messages
 * may be sent from arbitrary threads (typically the app UI thread).  The encoder thread
 * manages both sides of the encoder (feeding and draining); the only external input is
 * the GL texture.
 * <p>
 * The design is complicated slightly by the need to create an EGL context that shares state
 * with a view that gets restarted if (say) the device orientation changes.  When the view
 * in question is a GLSurfaceView, we don't have full control over the EGL context creation
 * on that side, so we have to bend a bit backwards here.
 * <p>
 * To use:
 * <ul>
 * <li>create TextureMovieEncoder object
 * <li>create an EncoderConfig
 * <li>call TextureMovieEncoder#startRecording() with the config
 * <li>call TextureMovieEncoder#setTextureId() with the texture object that receives frames
 * <li>for each frame, after latching it with SurfaceTexture#updateTexImage(),
 *     call TextureMovieEncoder#frameAvailable().
 * </ul>
 */
public class CameraVideoRecorder implements Runnable {

    private final static String TAG = CameraVideoRecorder.class.getSimpleName();

    private final static int MSG_START_RECORDING       = 0;
    private final static int MSG_STOP_RECORDING        = 1;
    private final static int MSG_FRAME_AVAILABLE       = 2;
    private final static int MSG_SET_TEXTURE_ID        = 3;
    private final static int MSG_UPDATE_SHARED_CONTEXT = 4;
    private final static int MSG_QUIT_RECORDING        = 5;

    private int mTextureId;
    private EglCore mEglCore;
    private BeautyCameraFilter mInput;
    private WindowEglSurface mWindowSurface;
    private VideoRecorderCore mVideoRecorder;

    // access by multiple threads
    private volatile RecorderHandler mHandler;

    private boolean mReady;
    private boolean mRunning;
    private Context mContext;
    private BaseFilter mFilter;
    private FloatBuffer glVertexBuffer;
    private FloatBuffer glTextureBuffer;

    // guard ready/running
    private final Object mReadyFence = new Object();

    private int mPreviewWidth = -1;
    private int mPreviewHeight = -1;
    private int mVideoWidth = -1;
    private int mVideoHeight = -1;

    private BeautyFilterType type = BeautyFilterType.NONE;

    public CameraVideoRecorder(Context context) {
        mContext = context;
    }

    public static class RecorderConfig {
        final int mWidth;
        final int mHeight;
        final int mBitrate;
        final File mOutputFile;
        final EGLContext mEglContext;

        public RecorderConfig(int width, int height, int bitrate, File outputFile, EGLContext eglContext) {
            this.mWidth = width;
            this.mHeight = height;
            this.mBitrate = bitrate;
            this.mOutputFile = outputFile;
            this.mEglContext = eglContext;
        }

    }

    private static class RecorderHandler extends Handler {
        private final WeakReference<CameraVideoRecorder> mWeakRecorder;

        public RecorderHandler(CameraVideoRecorder recorder) {
            mWeakRecorder = new WeakReference<>(recorder);
        }

        @Override
        public void handleMessage(@NonNull Message msg) {
            Object obj = msg.obj;
            CameraVideoRecorder recorder = mWeakRecorder.get();
            if (recorder == null) {
                return;
            }

            switch (msg.what) {
                case MSG_START_RECORDING:
                    recorder.handlerStartRecording((RecorderConfig)obj);
                    break;
                case MSG_STOP_RECORDING:
                    recorder.handlerStopRecording();
                    break;
                case MSG_FRAME_AVAILABLE:
                    long timestamp = (((long) msg.arg1) << 32) |
                            (((long) msg.arg2) & 0xffffffffL);
                    recorder.handleFrameAvailable((float[]) obj, timestamp);
                    break;
                case MSG_SET_TEXTURE_ID:
                    recorder.handleSetTexture(msg.arg1);
                    break;
                case MSG_UPDATE_SHARED_CONTEXT:
                    recorder.handleUpdateSharedContext((EGLContext)obj);
                    break;
                case MSG_QUIT_RECORDING:
                    Looper.myLooper().quit();
                    break;
                default:
                    break;
            }
        }
    }

    private void handlerStartRecording(RecorderConfig config) {
        prepareRecorder(
                config.mEglContext,
                config.mWidth,
                config.mHeight,
                config.mBitrate,
                config.mOutputFile);
    }

    private void handlerStopRecording() {
        mVideoRecorder.drainEncoder(true);
        releaseRecorder();
    }

    private void handleFrameAvailable(float[] transform, long timestamp) {
        mVideoRecorder.drainEncoder(false);
        mInput.setTextureTransformMatrix(transform);
        if (mFilter == null) {
            mInput.onDrawFrame(mTextureId, glVertexBuffer, glTextureBuffer);
        } else {
            mFilter.onDrawFrame(mTextureId, glVertexBuffer, glTextureBuffer);
        }
        mWindowSurface.setPresentationTime(timestamp);
        mWindowSurface.swapBuffers();
    }

    private void handleSetTexture(int id) {
        mTextureId = id;
    }

    private void handleUpdateSharedContext(EGLContext eglContext) {
        mWindowSurface.releaseEglSurface();
        mInput.destroy();
        mEglCore.release();

        mEglCore = new EglCore(eglContext, EglCore.FLAG_RECORDABLE);
        mWindowSurface.recreate(mEglCore);
        mWindowSurface.makeCurrent();

        mInput = new BeautyCameraFilter(mContext);
        mInput.init();
        mFilter = BeautyFilterFactory.getFilter(type, mContext);
        if (mFilter != null) {
            mFilter.init();
            mFilter.onOutputSizeChanged(mVideoWidth, mVideoHeight);
            mFilter.onInputSizeChanged(mPreviewWidth, mPreviewHeight);
        }
    }

    private void prepareRecorder(EGLContext eglContext, int width, int height, int bitrate, File file) {
        try {
            mVideoRecorder = new VideoRecorderCore(width, height, bitrate, file);
        } catch (IOException e) {
            throw new RuntimeException(e);
        }
        mVideoWidth = width;
        mVideoHeight = height;
        mEglCore = new EglCore(eglContext, EglCore.FLAG_RECORDABLE);
        mWindowSurface = new WindowEglSurface(mEglCore, mVideoRecorder.getInputSurface(), true);
        mWindowSurface.makeCurrent();

        mInput = new BeautyCameraFilter(mContext);
        mInput.init();
        mFilter = BeautyFilterFactory.getFilter(type, mContext);
        if (mFilter != null) {
            mFilter.init();
            mFilter.onOutputSizeChanged(mVideoWidth, mVideoHeight);
            mFilter.onInputSizeChanged(mPreviewWidth, mPreviewHeight);
        }
    }

    private void releaseRecorder() {
        mVideoRecorder.release();
        if (mWindowSurface != null) {
            mWindowSurface.release();
            mWindowSurface = null;
        }
        if (mInput != null) {
            mInput.destroy();
            mInput = null;
        }
        if (mFilter != null) {
            mFilter.destroy();
            mFilter = null;
            type = BeautyFilterType.NONE;
        }
        if (mEglCore != null) {
            mEglCore.release();
            mEglCore = null;
        }
    }

    public void startRecording(RecorderConfig config) {
        synchronized (mReadyFence) {
            if (mRunning) {
                return;
            }
            mRunning = true;
            new Thread(this, TAG).start();
            while (!mReady) {
                try {
                    mReadyFence.wait();
                } catch (InterruptedException e) {
                    e.printStackTrace();
                }
            }
        }
        mHandler.sendMessage(mHandler.obtainMessage(MSG_START_RECORDING, config));
    }

    public void stopRecording() {
        mHandler.sendMessage(mHandler.obtainMessage(MSG_STOP_RECORDING));
        mHandler.sendMessage(mHandler.obtainMessage(MSG_QUIT_RECORDING));
    }

    public boolean isRecording() {
        synchronized (mReadyFence) {
            return mRunning;
        }
    }

    public void updateSharedContext(EGLContext eglContext) {
        mHandler.sendMessage(mHandler.obtainMessage(MSG_UPDATE_SHARED_CONTEXT, eglContext));
    }

    public void frameAvailable(SurfaceTexture surfaceTexture) {
        synchronized (mReadyFence) {
            if (!mReady)
                return;
        }
        float[] transform = new float[16];
        surfaceTexture.getTransformMatrix(transform);
        long timestamp = surfaceTexture.getTimestamp();
        if (timestamp == 0) {
            return;
        }
        mHandler.sendMessage(mHandler.obtainMessage(MSG_FRAME_AVAILABLE, (int) (timestamp >> 32), (int) timestamp, transform));
    }

    public void setTextureId(int id) {
        synchronized (mReadyFence) {
            if (!mReady)
                return;
        }
        mHandler.sendMessage(mHandler.obtainMessage(MSG_SET_TEXTURE_ID, id, 0, null));
    }

    @Override
    public void run() {
        Looper.prepare();
        synchronized (mReadyFence) {
            mHandler = new RecorderHandler(this);
            mReady = true;
            mReadyFence.notify();
        }
        Looper.loop();
        synchronized (mReadyFence) {
            mReady = false;
            mRunning = false;
            mHandler = null;
        }
    }

    public void setFilter(BeautyFilterType type) {
        this.type = type;
    }

    public void setPreviewSize(int width, int height){
        mPreviewWidth = width;
        mPreviewHeight = height;
    }

    public void setTextureBuffer(FloatBuffer glTextureBuffer) {
        this.glTextureBuffer = glTextureBuffer;
    }

    public void setCubeBuffer(FloatBuffer gLVertexBuffer) {
        this.glVertexBuffer = gLVertexBuffer;
    }

}
