package com.frank.camerafilter.recorder.egl;

import android.graphics.Bitmap;
import android.opengl.EGL14;
import android.opengl.EGLSurface;
import android.opengl.GLES20;

import java.io.BufferedOutputStream;
import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.nio.IntBuffer;

/**
 * @author xufulong
 * @date 2022/6/23 8:51 上午
 * @desc
 */
public class EglSurfaceBase {

    protected EglCore mEglCore;
    protected int mWidth = -1;
    protected int mHeight = -1;

    private EGLSurface mEGLSurface = EGL14.EGL_NO_SURFACE;

    protected EglSurfaceBase(EglCore eglCore) {
        mEglCore = eglCore;
    }

    public void createWindowSurface(Object surface) {
        if (mEGLSurface != EGL14.EGL_NO_SURFACE) {
            throw new IllegalStateException("egl surface has already created");
        }
        mEGLSurface = mEglCore.createWindowSurface(surface);
    }

    public void createOffsetScreenSurface(int width, int height) {
        if (mEGLSurface != EGL14.EGL_NO_SURFACE) {
            throw new IllegalStateException("egl surface has already created");
        }
        mWidth = width;
        mHeight = height;
        mEGLSurface = mEglCore.createOffsetScreenSurface(width, height);
    }

    public int getWidth() {
        if (mWidth <= 0) {
            mWidth = mEglCore.querySurface(mEGLSurface, EGL14.EGL_WIDTH);
        }
        return mWidth;
    }

    public int getHeight() {
        if (mHeight <= 0) {
            mHeight = mEglCore.querySurface(mEGLSurface, EGL14.EGL_HEIGHT);
        }
        return mHeight;
    }

    public void releaseEglSurface() {
        mEglCore.releaseSurface(mEGLSurface);
        mEGLSurface = EGL14.EGL_NO_SURFACE;
        mWidth = -1;
        mHeight = -1;
    }

    public void makeCurrent() {
        mEglCore.makeCurrent(mEGLSurface);
    }

    public void makeCurrentReadFrom(EglSurfaceBase readSurface) {
        mEglCore.makeCurrent(mEGLSurface, readSurface.mEGLSurface);
    }

    public boolean swapBuffers() {
        return mEglCore.swapBuffers(mEGLSurface);
    }

    public void setPresentationTime(long nsec) {
        mEglCore.setPresentationTime(mEGLSurface, nsec);
    }

    public void saveFrame(File file) throws IOException {
        if (!mEglCore.isCurrent(mEGLSurface)) {
            throw new RuntimeException("isn't current surface/context");
        }
        String fileName = file.toString();
        int width = getWidth();
        int height = getHeight();
        IntBuffer buffer = IntBuffer.allocate(width * height);
        GLES20.glReadPixels(0, 0, width, height, GLES20.GL_RGBA, GLES20.GL_UNSIGNED_BYTE, buffer);
        BufferedOutputStream outputStream = null;
        try {
            outputStream = new BufferedOutputStream(new FileOutputStream(fileName));
            Bitmap bitmap = Bitmap.createBitmap(width, height, Bitmap.Config.ARGB_8888);
            bitmap.copyPixelsFromBuffer(buffer);
            bitmap.compress(Bitmap.CompressFormat.JPEG, 100, outputStream);
            bitmap.recycle();
        } finally {
            if (outputStream != null)
                outputStream.close();
        }
    }

}
