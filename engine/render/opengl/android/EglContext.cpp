/**
 * Note: EglContext of Android
 * Date: 2025/12/16
 * Author: frank
 */

#if defined(__ANDROID__)

#include "EglContext.h"

#include "NextLog.h"

#define EGL_TAG "And_EglContext"

EglContext::EglContext() {
    mEglConfig  = EGL_NO_CONFIG_KHR;
    mEglDisplay = EGL_NO_DISPLAY;
    mEglContext = EGL_NO_CONTEXT;
    mEglSurface = EGL_NO_SURFACE;
}

EglContext::~EglContext() {
    EGLTerminate();
}

EGLBoolean EglContext::InitContext(EGLContext eglContext, EGLNativeWindowType window) {
    std::unique_lock<std::mutex> lock(mEglMutex);
    if (IsEglValid()) {
        return EGL_TRUE;
    }

    EGLTerminate();
    if (nullptr == eglContext) {
        eglContext = EGL_NO_CONTEXT;
    }
    // 1. init display
    mEglDisplay = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (mEglDisplay == EGL_NO_DISPLAY) {
        NEXT_LOGE(EGL_TAG, "eglGetDisplay error");
        return EGL_FALSE;
    }
    EGLint major, minor;
    if (!eglInitialize(mEglDisplay, &major, &minor)) {
        NEXT_LOGE(EGL_TAG, "eglInitialize error");
        return EGL_FALSE;
    }

    // 2. config: try version 3 first
    int version = 3;
    EGLConfig config = GetEGLConfig(version);
    if (nullptr != config) {
        EGLint context3Attrib[] = {EGL_CONTEXT_CLIENT_VERSION, version, EGL_NONE};
        // 3. egl context
        mEglContext = eglCreateContext(mEglDisplay, config, eglContext, context3Attrib);
        if (mEglContext == EGL_NO_CONTEXT) {
            NEXT_LOGE(EGL_TAG, "eglCreateContext version3: error=%d", eglGetError());
            // try to fallback to version 2
            version = 2;
            config = GetEGLConfig(version);
            if (nullptr != config) {
                EGLint context2Attrib[] = {EGL_CONTEXT_CLIENT_VERSION, version, EGL_NONE};
                mEglContext = eglCreateContext(mEglDisplay, config, eglContext, context2Attrib);
                if (mEglContext == EGL_NO_CONTEXT) {
                    NEXT_LOGE(EGL_TAG, "eglCreateContext version2: error=%d", eglGetError());
                    eglTerminate(mEglDisplay);
                    return EGL_FALSE;
                }
            }
        }
        mEglConfig = config;
    }
    // 4. create surface
    if (window) {
        EGLint surfaceAttrib[] = {EGL_NONE};
        mEglSurface = eglCreateWindowSurface(mEglDisplay, config, window, surfaceAttrib);
        if (EGL_NO_SURFACE == mEglSurface) {
            NEXT_LOGE(EGL_TAG, "eglCreateWindowSurface error=%d", eglGetError());
            eglTerminate(mEglDisplay);
            return EGL_FALSE;
        }
        mEglWindowType = window;

        EGLBoolean ret = SetSurfaceWithWindow();
        if (!ret) {
            NEXT_LOGE(EGL_TAG, "SetSurfaceWithWindow error");
            return EGL_FALSE;
        }
        // 5. switch current render thread
        ret = eglMakeCurrent(mEglDisplay, mEglSurface, mEglSurface, mEglContext);
        if (!ret) {
            NEXT_LOGE(EGL_TAG, "eglMakeCurrent error=%d", eglGetError());
            if (mEglSurface != EGL_NO_SURFACE) {
                ret = eglDestroySurface(mEglDisplay, mEglSurface);
            }
            if (mEglContext != EGL_NO_CONTEXT) {
                ret = eglDestroyContext(mEglDisplay, mEglContext);
            }
            eglTerminate(mEglDisplay);
            return EGL_FALSE;
        }
    }

    return EGL_TRUE;
}

EGLBoolean EglContext::CreateEglSurface(EGLNativeWindowType surface) {
    std::unique_lock<std::mutex> lck(mEglMutex);

    if (mEglWindowType) {
        if (mEglWindowType == surface) {
            EGLBoolean ret = SetSurfaceWithWindow();
            if (!ret) {
                NEXT_LOGE(EGL_TAG, "SetSurfaceWithWindow error, line=%d", __LINE__);
                return EGL_FALSE;
            }

            if (!MakeCurrent()) {
                NEXT_LOGE(EGL_TAG, "MakeCurrent error");
                return EGL_FALSE;
            }
            return EGL_TRUE;
        }

        ANativeWindow_release(mEglWindowType);
        mEglWindowType = nullptr;
        eglMakeCurrent(mEglDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);

        if (mEglSurface) {
            eglDestroySurface(mEglDisplay, mEglSurface);
            mEglSurface = EGL_NO_SURFACE;
        }
    }
    ANativeWindow_acquire(surface);

    if (!mEglSurface) {
        EGLint surfaceAttributes[] = {EGL_NONE};
        mEglSurface =
                eglCreateWindowSurface(mEglDisplay, mEglConfig, surface, surfaceAttributes);
        if (!mEglSurface) {
            NEXT_LOGE(EGL_TAG, "eglCreateWindowSurface error=%d", eglGetError());
            return EGL_FALSE;
        }

        mEglWindowType = surface;
    }

    EGLBoolean ret = SetSurfaceWithWindow();
    if (!ret) {
        NEXT_LOGE(EGL_TAG, "SetSurfaceWithWindow error");
        return EGL_FALSE;
    }

    if (!MakeCurrent()) {
        NEXT_LOGE(EGL_TAG, "MakeCurrent error=%d", glGetError());
        return EGL_FALSE;
    }

    return EGL_TRUE;
}

EGLBoolean EglContext::MakeCurrent() {
    if (EGL_NO_DISPLAY == mEglDisplay || EGL_NO_SURFACE == mEglSurface || EGL_NO_CONTEXT == mEglContext) {
        NEXT_LOGE(EGL_TAG, "MakeCurrent null, display=%p, surface=%p, context=%p",
                mEglDisplay, mEglSurface, mEglContext);
        return EGL_FALSE;
    }

    EGLBoolean ret = eglMakeCurrent(mEglDisplay, mEglSurface, mEglSurface, mEglContext);
    if (!ret) {
        NEXT_LOGE(EGL_TAG, "eglMakeCurrent error=%d", eglGetError());
        return EGL_FALSE;
    }
    return EGL_TRUE;
}

EGLBoolean EglContext::SetSurfaceSize(int width, int height) {
    std::unique_lock<std::mutex> lock(mEglMutex);
    if (!IsEglValid()) {
        return EGL_FALSE;
    }

    mWidth  = GetSurfaceWidth();
    mHeight = GetSurfaceHeight();

    if (width != mWidth || height != mHeight) {
        int format = ANativeWindow_getFormat(mEglWindowType);
        int ret = ANativeWindow_setBuffersGeometry(mEglWindowType, width, height, format);
        if (ret < 0) {
            NEXT_LOGE(EGL_TAG,"ANativeWindow_setBuffersGeometry error=%d", ret);
            return EGL_FALSE;
        }

        mWidth  = GetSurfaceWidth();
        mHeight = GetSurfaceHeight();
        return (mWidth && mHeight) ? EGL_TRUE : EGL_FALSE;
    }

    return EGL_TRUE;
}

int EglContext::GetSurfaceWidth() {
    EGLint width = 0;
    if (!eglQuerySurface(mEglDisplay, mEglSurface, EGL_WIDTH, &width)) {
        return 0;
    }

    return width;
}

int EglContext::GetSurfaceHeight() {
    EGLint height = 0;
    if (!eglQuerySurface(mEglDisplay, mEglSurface, EGL_HEIGHT, &height)) {
        return 0;
    }

    return height;
}

EGLBoolean EglContext::SwapBuffers() {
    if (eglGetCurrentContext() == EGL_NO_CONTEXT ||
        eglGetCurrentDisplay() == EGL_NO_DISPLAY ||
        eglGetCurrentSurface(EGL_DRAW) == EGL_NO_SURFACE) {
        NEXT_LOGE(EGL_TAG, "SwapBuffers null, error=%d",  eglGetError());
        return EGL_FALSE;
    }

    EGLBoolean ret = MakeCurrent();
    if (!ret) {
        return EGL_FALSE;
    }

    ret = eglSwapBuffers(mEglDisplay, mEglSurface);
    if (!ret) {
        NEXT_LOGE(EGL_TAG, "eglSwapBuffers error=%d",  eglGetError());
        return EGL_FALSE;
    }

    return EGL_TRUE;
}

EGLConfig EglContext::GetEGLConfig(int version) {
    int renderType = EGL_OPENGL_ES2_BIT;
    if (version >= 3) {
        renderType |= EGL_OPENGL_ES3_BIT_KHR;
    }
    EGLint configAttrib[] = {
            EGL_SURFACE_TYPE, EGL_WINDOW_BIT, EGL_RENDERABLE_TYPE, renderType,
            EGL_RECORDABLE_ANDROID, 1, // mediacodec support
            EGL_RED_SIZE, 8, EGL_GREEN_SIZE, 8, EGL_BLUE_SIZE, 8, EGL_ALPHA_SIZE, 8, // rgba8888
            EGL_NONE};

    EGLConfig config = nullptr;
    EGLint numConfig;
    if (!eglChooseConfig(mEglDisplay, configAttrib, &config, 1, &numConfig)) {
        NEXT_LOGE(EGL_TAG, "eglChooseConfig error=%d", eglGetError());
        return nullptr;
    }
    return config;
}

EGLBoolean EglContext::SetSurfaceWithWindow() {
    if (!IsEglValid()) {
        return EGL_FALSE;
    }

    EGLint format = 0;
    if (!eglGetConfigAttrib(mEglDisplay, mEglConfig, EGL_NATIVE_VISUAL_ID, &format)) {
        EGLTerminate();
        return EGL_FALSE;
    }

    int32_t width  = ANativeWindow_getWidth(mEglWindowType);
    int32_t height = ANativeWindow_getHeight(mEglWindowType);
    int ret = ANativeWindow_setBuffersGeometry(mEglWindowType, width, height, format);
    if (ret < 0) {
        NEXT_LOGE(EGL_TAG,"ANativeWindow_setBuffersGeometry, error=%d", ret);
        EGLTerminate();
        return EGL_FALSE;
    }
    mWidth  = GetSurfaceWidth();
    mHeight = GetSurfaceHeight();

    if (mWidth && mHeight) {
        return EGL_TRUE;
    } else {
        EGL_FALSE;
    }
}

void EglContext::EGLTerminate() {
    if (!IsEglValid()) {
        return;
    }

    if (mEglDisplay) {
        glFinish();

        EGLContext currentContext     = eglGetCurrentContext();
        EGLDisplay currentDisplay     = eglGetCurrentDisplay();
        EGLSurface currentDrawSurface = eglGetCurrentSurface(EGL_DRAW);
        EGLSurface currentReadSurface = eglGetCurrentSurface(EGL_READ);

        if (currentContext != EGL_NO_CONTEXT &&
            currentDrawSurface != EGL_NO_SURFACE &&
            currentReadSurface != EGL_NO_SURFACE &&
            currentDisplay != EGL_NO_DISPLAY &&
            currentContext == mEglContext && currentDisplay == mEglDisplay &&
            currentDrawSurface == mEglSurface && currentReadSurface == mEglSurface) {

            EGLBoolean ret = eglMakeCurrent(currentDisplay, currentDrawSurface,
                                            currentReadSurface, currentContext);
            if (!ret) {
                NEXT_LOGE(EGL_TAG, "eglMakeCurrent error=%d", eglGetError());
            }
        } else {
            if (currentContext == EGL_NO_CONTEXT ||
                currentDisplay == EGL_NO_DISPLAY ||
                currentDrawSurface == EGL_NO_SURFACE ||
                currentReadSurface == EGL_NO_SURFACE) {
                NEXT_LOGE(EGL_TAG, "eglGetCurrentContext error");
                return;
            }
            EGLBoolean ret = eglMakeCurrent(currentDisplay, currentDrawSurface,
                                            currentReadSurface, currentContext);
            if (!ret) {
                NEXT_LOGE(EGL_TAG, "eglMakeCurrent error=%d", eglGetError());
                return;
            }
        }

        EGLBoolean ret = eglMakeCurrent(currentDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        if (!ret) {
            NEXT_LOGE(EGL_TAG, "eglMakeCurrent error=%d", eglGetError());
            return;
        }

        if (currentDrawSurface != EGL_NO_SURFACE) {
            ret = eglDestroySurface(currentDisplay, currentDrawSurface);
        }

        if (currentReadSurface != EGL_NO_SURFACE && currentReadSurface != currentDrawSurface) {
            ret = eglDestroySurface(currentDisplay, currentReadSurface);
        }

        if (currentContext != EGL_NO_CONTEXT) {
            ret = eglDestroyContext(currentDisplay, currentContext);
        }

        eglTerminate(currentDisplay);
        eglReleaseThread();
    }

    mEglDisplay = EGL_NO_DISPLAY;
    mEglContext = EGL_NO_CONTEXT;
    mEglSurface = EGL_NO_SURFACE;

    if (mEglWindowType) {
        ANativeWindow_release(mEglWindowType);
        mEglWindowType = nullptr;
    }
}

EGLBoolean EglContext::IsEglValid() {
    if (mEglWindowType && mEglDisplay && mEglSurface && mEglContext) {
        return EGL_TRUE;
    }
    return EGL_FALSE;
}

#endif
