
#include "./OpenglContext.h"

OpenGLContext::OpenGLContext() {

#if defined(__ANDROID__)
    mGLContext = std::make_shared<EglContext>();
#elif defined(__APPLE__)
    mGLContext = std::make_shared<EaglContext>(sessionID);
#endif

}

OpenGLContext::~OpenGLContext() {
    if (mGLContext) {
        mGLContext.reset();
        mGLContext = nullptr;
    }
}

std::shared_ptr<OpenGLContext> OpenGLContext::createInstance() {
    std::shared_ptr<OpenGLContext> instance = std::make_shared<OpenGLContext>();
    return instance;
}

#if defined(__ANDROID__)
std::shared_ptr<EglContext> OpenGLContext::getEglContext() const {
    return mGLContext;
}
#elif defined(__APPLE__)
std::shared_ptr<EaglContext> OpenGLContext::getEaglContext() const {
  return mGLContext;
}
#endif
