package com.frank.camerafilter.recorder.gles;

import android.graphics.SurfaceTexture;
import android.opengl.EGL14;
import android.opengl.EGLConfig;
import android.opengl.EGLContext;
import android.opengl.EGLDisplay;
import android.opengl.EGLExt;
import android.opengl.EGLSurface;
import android.util.Log;
import android.view.Surface;

/**
 * Core EGL state (display, context, config).
 * <p>
 * The EGLContext must only be attached to one thread at a time.  This class is not thread-safe.
 */

public final class EglCore {

    private final static String TAG = EglCore.class.getSimpleName();

    public final static int FLAG_RECORDABLE = 0x01;

    public final static int FLAG_TRY_GLES3  = 0x02;

    private final static int EGL_RECORDABLE_ANDROID = 0x3142;

    private int mGlVersion = -1;
    private EGLConfig mEGLConfig = null;
    private EGLDisplay mEGLDisplay = EGL14.EGL_NO_DISPLAY;
    private EGLContext mEGLContext = EGL14.EGL_NO_CONTEXT;

    public EglCore() {
        this(null, 0);
    }

    public EglCore(EGLContext sharedContext, int flag) {
        mEGLDisplay = EGL14.eglGetDisplay(EGL14.EGL_DEFAULT_DISPLAY);
        int[] version = new int[2];
        if (!EGL14.eglInitialize(mEGLDisplay, version, 0, version, 1)) {
            throw new RuntimeException("unable to init EGL14");
        }

        if ((flag & FLAG_TRY_GLES3) != 0) {
            initEGLContext(sharedContext, flag, 3);
        }
        if (mEGLContext == EGL14.EGL_NO_CONTEXT) {
            initEGLContext(sharedContext, flag, 2);
        }

        int[] value = new int[1];
        EGL14.eglQueryContext(mEGLDisplay, mEGLContext, EGL14.EGL_CONTEXT_CLIENT_VERSION, value, 0);
        Log.i(TAG, "EGLContext client version=" + value[0]);
    }

    private void initEGLContext(EGLContext sharedContext, int flag, int version) {
        EGLConfig config = getConfig(flag, version);
        if (config == null) {
            throw new RuntimeException("unable to find suitable EGLConfig");
        }
        int[] attributeList = {EGL14.EGL_CONTEXT_CLIENT_VERSION, version, EGL14.EGL_NONE};
        EGLContext context = EGL14.eglCreateContext(mEGLDisplay, config, sharedContext, attributeList, 0);
        if (EGL14.eglGetError() == EGL14.EGL_SUCCESS) {
            mEGLConfig = config;
            mEGLContext = context;
            mGlVersion = version;
        }
    }

    private EGLConfig getConfig(int flag, int version) {
        int renderType = EGL14.EGL_OPENGL_ES2_BIT;
        if (version >= 3) {
            renderType |= EGLExt.EGL_OPENGL_ES3_BIT_KHR;
        }

        int[] attributeList = {
                EGL14.EGL_RED_SIZE, 8,
                EGL14.EGL_GREEN_SIZE, 8,
                EGL14.EGL_BLUE_SIZE, 8,
                EGL14.EGL_ALPHA_SIZE, 8,
                //EGL14.EGL_DEPTH_SIZE, 16,
                //EGL14.EGL_STENCIL_SIZE, 8,
                EGL14.EGL_RENDERABLE_TYPE, renderType,
                EGL14.EGL_NONE, 0,
                EGL14.EGL_NONE
        };

        if ((flag & FLAG_RECORDABLE) != 0) {
            attributeList[attributeList.length - 3] = EGL_RECORDABLE_ANDROID;
            attributeList[attributeList.length - 2] = 1;
        }
        int[] numConfigs = new int[1];
        EGLConfig[] configs = new EGLConfig[1];
        if (!EGL14.eglChooseConfig(mEGLDisplay, attributeList, 0, configs,
                0, configs.length, numConfigs, 0)) {
            Log.e(TAG, "unable to find RGB8888 / " + version + " EGLConfig");
            return null;
        }
        return configs[0];
    }

    public void release() {
        if (mEGLDisplay != EGL14.EGL_NO_DISPLAY) {
            EGL14.eglMakeCurrent(mEGLDisplay, EGL14.EGL_NO_SURFACE, EGL14.EGL_NO_SURFACE, EGL14.EGL_NO_CONTEXT);
            EGL14.eglDestroyContext(mEGLDisplay, mEGLContext);
            EGL14.eglReleaseThread();
            EGL14.eglTerminate(mEGLDisplay);
        }
        mEGLConfig  = null;
        mEGLDisplay = EGL14.EGL_NO_DISPLAY;
        mEGLContext = EGL14.EGL_NO_CONTEXT;
    }

    @Override
    protected void finalize() throws Throwable {
        try {
            if (mEGLDisplay != EGL14.EGL_NO_DISPLAY) {
                release();
            }
        } finally {
            super.finalize();
        }
    }

    public void releaseSurface(EGLSurface eglSurface) {
        if (mEGLDisplay != EGL14.EGL_NO_DISPLAY) {
            EGL14.eglDestroySurface(mEGLDisplay, eglSurface);
        }
    }

    public EGLSurface createWindowSurface(Object surface) {
        if (!(surface instanceof Surface) && !(surface instanceof SurfaceTexture)) {
            throw new RuntimeException("invalid surface:" + surface);
        }

        int[] surfaceAttr = {EGL14.EGL_NONE};
        EGLSurface eglSurface = EGL14.eglCreateWindowSurface(mEGLDisplay, mEGLConfig, surface, surfaceAttr, 0);
        if (eglSurface == null) {
            throw new RuntimeException("window surface is null");
        }
        return eglSurface;
    }

    public EGLSurface createOffsetScreenSurface(int width, int height) {
        int[] surfaceAttr = {EGL14.EGL_WIDTH, width,
                EGL14.EGL_HEIGHT, height,
                EGL14.EGL_NONE};
        EGLSurface eglSurface = EGL14.eglCreatePbufferSurface(mEGLDisplay, mEGLConfig, surfaceAttr, 0);
        if (eglSurface == null) {
            throw new RuntimeException("offset-screen surface is null");
        }
        return eglSurface;
    }

    public void makeCurrent(EGLSurface eglSurface) {
        if (!EGL14.eglMakeCurrent(mEGLDisplay, eglSurface, eglSurface, mEGLContext)) {
            throw new RuntimeException("eglMakeCurrent failed!");
        }
    }

    public void makeCurrent(EGLSurface drawSurface, EGLSurface readSurface) {
        if (!EGL14.eglMakeCurrent(mEGLDisplay, drawSurface, readSurface, mEGLContext)) {
            throw new RuntimeException("eglMakeCurrent failed!");
        }
    }

    public boolean swapBuffers(EGLSurface eglSurface) {
        return EGL14.eglSwapBuffers(mEGLDisplay, eglSurface);
    }

    public void setPresentationTime(EGLSurface eglSurface, long nsec) {
        EGLExt.eglPresentationTimeANDROID(mEGLDisplay, eglSurface, nsec);
    }

    public boolean isCurrent(EGLSurface eglSurface) {
        return mEGLContext.equals(EGL14.eglGetCurrentContext())
                && eglSurface.equals(EGL14.eglGetCurrentSurface(EGL14.EGL_DRAW));
    }

    public int querySurface(EGLSurface eglSurface, int what) {
        int[] value = new int[1];
        EGL14.eglQuerySurface(mEGLDisplay, eglSurface, what, value, 0);
        return value[0];
    }

    public String queryString(int what) {
        return EGL14.eglQueryString(mEGLDisplay, what);
    }

    public int getVersion() {
        return mGlVersion;
    }

}
