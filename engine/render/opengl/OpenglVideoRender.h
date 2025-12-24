#ifndef OPENGL_VIDEO_RENDER_H
#define OPENGL_VIDEO_RENDER_H

#include <ctime>
#include <map>
#include <memory>
#include <mutex>
#include <sys/time.h>

#include "../VideoRender.h"
#include "common/OpenglContext.h"
#include "filter/BaseFilter.h"

#if defined(__APPLE__)
#include "./ios/redrender_gl_view.h"
#include <mach/mach_time.h>
#endif

class OpenGLVideoRender : public VideoRender {
public:
    OpenGLVideoRender();

    ~OpenGLVideoRender() override;

    int ReleaseContext() override;

#if defined(__ANDROID__)

    int Init() override;

    int SetSurface(ANativeWindow *nativeWindow) override;

#elif defined(__APPLE__)

    int init() override;

    int initWithFrame(CGRect cgrect = {{0, 0}, {0, 0}}) override;
    UIView *getRedRenderView() override;

#endif

    // filter chain management
    int AttachFilter(VideoFilterType videoFilterType,
                     VideoFrameMetaData *inputFrameMetaData) override;

    // input Frame
    int OnInputFrame(VideoFrameMetaData *redRenderBuffer) override;

    // render
    int OnRender() override;

    int OnRenderCacheFrame() override;

    int SetGravity(AspectRatioMode rendererGravity);

private:
    int SetInputFrame(VideoFrameMetaData *inputFrameMetaData);

    int OnScreenRender();

    int CreateOnScreenRender(VideoFrameMetaData *inputFrameMetaData);

    void UpdateInputFrameData(VideoFrameMetaData *inputFrameMetaData);

private:

#if defined(__APPLE__)
    CVPixelBufferRef mCachedPixelBuffer{nullptr};
#endif

    VideoFrameMetaData mRenderMetaData{
#if defined(__APPLE__)
            nullptr,
#endif
            {nullptr, nullptr, nullptr}, {0, 0, 0}, 0,
            0, 0, 0, 0, {0, 0},
            AVCOL_SPC_RGB, AVCOL_RANGE_UNSPECIFIED, ROTATION_NO, ASPECT_RATIO_FIT_WIDTH,
            AVCOL_PRI_RESERVED0, PIXEL_FORMAT_UNKNOWN, AVCOL_TRC_RESERVED0
    };


    GLuint mTextures[3] = {0};
    uint64_t mLastFrameTime = 0;

    double mFps       = 0;
    int mFrameCount   = 0;
    int mRenderWidth  = 0;
    int mRenderHeight = 0;

    std::mutex mRenderMutex;
    std::shared_ptr<OpenGLContext> mGLContext;
    VideoFrameMetaData *mInputFrameData = nullptr;
    AspectRatioMode mRendererGravity{ASPECT_RATIO_FIT};
    std::shared_ptr<BaseFilter> mOpenglFilter = nullptr;

#if defined(__APPLE__)
    RedRenderGLView *mRenderGLView{nullptr};
    RedRenderGLTexture *mRenderGLTexture[2] = {nullptr};
#endif

};

#endif //OPENGL_VIDEO_RENDER_H
