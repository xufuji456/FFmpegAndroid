#ifndef OPENGL_CONTEXT_H
#define OPENGL_CONTEXT_H

#if defined(__ANDROID__)

#include "../android/EglContext.h"

#elif defined(__APPLE__)

#include "../ios/eagl_context.h"

#endif

class OpenGLContext {
public:
    OpenGLContext();

    ~OpenGLContext();

    static std::shared_ptr<OpenGLContext> createInstance();

#if defined(__ANDROID__)

    std::shared_ptr<EglContext> getEglContext() const;

#elif defined(__APPLE__)
    std::shared_ptr<EaglContext> getEaglContext() const;
#endif

private:
#if (__ANDROID__)
    std::shared_ptr<EglContext> mGLContext;
#elif defined(__APPLE__)
    std::shared_ptr<EaglContext> mGLContext;
#endif

};

#endif // OPENGL_CONTEXT_H
