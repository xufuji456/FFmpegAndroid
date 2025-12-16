#if defined(__ANDROID__)
#ifndef ANDROID_EGL_CONTEXT_H
#define ANDROID_EGL_CONTEXT_H

#include <mutex>

#include <android/native_window.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <EGL/eglplatform.h>
#include <GLES2/gl2.h>

class EglContext {
public:
    EglContext();

    ~EglContext();

    EGLBoolean InitContext(EGLContext eglContext = nullptr, EGLNativeWindowType window = nullptr);

    EGLBoolean CreateEglSurface(EGLNativeWindowType surface);

    EGLBoolean MakeCurrent();

    EGLBoolean SetSurfaceSize(int width, int height);

    int GetSurfaceWidth();

    int GetSurfaceHeight();

    EGLBoolean SwapBuffers();

private:

    EGLConfig GetEGLConfig(int version);

    EGLBoolean SetSurfaceWithWindow();

    void EGLTerminate();

    EGLBoolean IsEglValid();

    EGLint mWidth  = 0;
    EGLint mHeight = 0;
    std::mutex mEglMutex;

    EGLConfig mEglConfig   = EGL_NO_CONFIG_KHR;
    EGLDisplay mEglDisplay = EGL_NO_DISPLAY;
    EGLContext mEglContext = EGL_NO_CONTEXT;
    EGLSurface mEglSurface = EGL_NO_SURFACE;

    EGLNativeWindowType mEglWindowType = nullptr;
};

#endif // ANDROID_EGL_CONTEXT_H
#endif // defined(__ANDROID__)
