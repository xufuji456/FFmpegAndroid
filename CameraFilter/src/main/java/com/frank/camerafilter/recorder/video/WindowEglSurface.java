package com.frank.camerafilter.recorder.video;

import android.view.Surface;

import com.frank.camerafilter.recorder.gles.EglCore;
import com.frank.camerafilter.recorder.gles.EglSurfaceBase;

/**
 * @author xufulong
 * @date 2022/6/23 9:15 上午
 * @desc
 */
public class WindowEglSurface extends EglSurfaceBase {

    private Surface mSurface;
    private boolean mReleaseSurface;

    public WindowEglSurface(EglCore eglCore, Surface surface) {
        this(eglCore, surface, false);
    }

    public WindowEglSurface(EglCore eglCore, Surface surface, boolean releaseSurface) {
        super(eglCore);
        createWindowSurface(surface);
        mSurface = surface;
        mReleaseSurface = releaseSurface;
    }

    public void release() {
        releaseEglSurface();
        if (mSurface != null && mReleaseSurface) {
            mSurface.release();
        }
        mSurface = null;
    }

    public void recreate(EglCore newEglCore) {
        if (mSurface == null) {
            throw new RuntimeException("Surface is null");
        }
        mEglCore = newEglCore;
        createWindowSurface(mSurface);
    }

}
