/**
 * Note: video render with OpenGL
 * Date: 2025/12/23
 * Author: frank
 */

#include "OpenglVideoRender.h"

#include "CommonUtil.h"
#include "common/OpenglContext.h"
#include "common/OpenGLUtil.h"
#include "filter/OpenglFilter.h"
#include "NextLog.h"

#if defined(__ANDROID__)
#include "android/EglContext.h"
#elif defined(__APPLE__)
#include "ios/eagl_context.h"
#endif

#define OPENGL_RENDER "OpenglRender"

OpenGLVideoRender::OpenGLVideoRender()
        : VideoRender() {

}

OpenGLVideoRender::~OpenGLVideoRender() {
    ReleaseContext();
}

void PrintString(const char *name, GLenum string) {
    const char *value = (const char *) glGetString(string);
    NEXT_LOGI(OPENGL_RENDER, "%s = %s \n", name, value);
}

int OpenGLVideoRender::ReleaseContext() {
    mGLContext.reset();
#if defined(__APPLE__)
    mRenderGLView = nullptr;
    mRenderGLTexture[0] = nullptr;
    mRenderGLTexture[1] = nullptr;
    if (mCachedPixelBuffer) {
      CVBufferRelease(mCachedPixelBuffer);
      mCachedPixelBuffer = nullptr;
    }
#endif

    return RESULT_OK;
}

#if defined(__ANDROID__)

int OpenGLVideoRender::Init() {
    if (mGLContext) {
        mGLContext.reset();
    }
    mGLContext = OpenGLContext::createInstance();
    return RESULT_OK;
}

int OpenGLVideoRender::SetSurface(ANativeWindow *nativeWindow) {
    std::unique_lock<std::mutex> lck(mRenderMutex);
    if (mGLContext == nullptr || nullptr == mGLContext->getEglContext()) {
        NEXT_LOGE(OPENGL_RENDER, "EglContext=nullptr\n");
        return ERROR_RENDER_VIDEO_CTX;
    }
    if (nativeWindow == nullptr) {
        NEXT_LOGE(OPENGL_RENDER, "nativeWindow=nullptr\n");
        return ERROR_RENDER_VIDEO_SUR;
    }
    if (!mGLContext->getEglContext()->InitContext()) {
        NEXT_LOGE(OPENGL_RENDER, "init EglContext error\n");
        return ERROR_RENDER_VIDEO_CTX;
    }
    if (!mGLContext->getEglContext()->CreateEglSurface(nativeWindow)) {
        NEXT_LOGE(OPENGL_RENDER, "CreateEglSurface error\n");
        return ERROR_RENDER_VIDEO_CTX;
    }

    PrintString("Version", GL_VERSION);
    PrintString("Vendor", GL_VENDOR);
    PrintString("Renderer", GL_RENDERER);
    PrintString("Extensions", GL_EXTENSIONS);

    mRenderWidth  = mGLContext->getEglContext()->GetSurfaceWidth();
    mRenderHeight = mGLContext->getEglContext()->GetSurfaceHeight();

    return RESULT_OK;
}

#elif defined(__APPLE__)

int OpenGLVideoRender::init() {
  if (mGLContext) {
    mGLContext.reset();
  }
  mGLContext = Context::creatInstance();
  if (mGLContext == nullptr) {
    NEXT_LOGE(OPENGL_RENDER, "createInstance error\n");
  }
  return RESULT_OK;
}

int OpenGLVideoRender::initWithFrame(CGRect cgrect /* = {{0,0}, {0,0}} */) {
  if (nullptr == mGLContext->getEaglContext()) {
    NEXT_LOGE(OPENGL_RENDER, "eglContext==nullptr\n");
    return ERROR_RENDER_VIDEO_CTX;
  }

  if (!mGLContext->getEaglContext()->init(cgrect, this)) {
    NEXT_LOGE(OPENGL_RENDER, "EaglContext init error\n");
    return ERROR_RENDER_VIDEO_CTX;
  }

  mGLContext->opengl_print_string("Version", GL_VERSION);
  mGLContext->opengl_print_string("Vendor", GL_VENDOR);
  mGLContext->opengl_print_string("Renderer", GL_RENDERER);
  mGLContext->opengl_print_string("Extensions", GL_EXTENSIONS);

  mRenderWidth  = mGLContext->getEaglContext()->getFrameWidth();
  mRenderHeight = mGLContext->getEaglContext()->getFrameHeight();

  return RESULT_OK;
}

UIView *OpenGLVideoRender::getRedRenderView() {
  mRenderGLView = mGLContext->getEaglContext()->getRedRenderGLView();
  if (nullptr == mRenderGLView) {
    NEXT_LOGE(OPENGL_RENDER, "EaglContext init error\n");
    return nullptr;
  }
  return mRenderGLView;
}

#endif

int OpenGLVideoRender::SetGravity(AspectRatioMode rendererGravity) {
    mRendererGravity = rendererGravity;

#if defined(__APPLE__)
    mRenderWidth  = mGLContext->getEaglContext()->getFrameWidth();
    mRenderHeight = mGLContext->getEaglContext()->getFrameHeight();
    if (mInputFrameData != nullptr) {
      mInputFrameData->view_width = mRenderWidth;
      mInputFrameData->view_height = mRenderHeight;
    }
#endif

    return RESULT_OK;
}

int OpenGLVideoRender::AttachFilter(VideoFilterType videoFilterType,
                                    VideoFrameMetaData *inputFrameMetaData) {
    std::unique_lock<std::mutex> lck(mRenderMutex);
#if defined(__APPLE__)
    mGLContext->getEaglContext()->useAsCurrent();
#endif
    std::shared_ptr<BaseFilter> filter = nullptr;
    switch (videoFilterType) {
        case VIDEO_FILTER_OPENGL: {
            if (nullptr == mOpenglFilter) {
                UpdateInputFrameData(inputFrameMetaData);
                int ret = CreateOnScreenRender(&mRenderMetaData);
                if (ret != RESULT_OK) {
                    NEXT_LOGE(OPENGL_RENDER, "CreateScreenRender error\n");
                    return ret;
                }
            }
        }
            break;
        default:
            NEXT_LOGE(OPENGL_RENDER, "Don't support filter:%d\n", (int) videoFilterType);
            break;
    }
#if defined(__APPLE__)
    mGLContext->getEaglContext()->useAsPrev();
#endif
    return RESULT_OK;
}

int OpenGLVideoRender::OnInputFrame(VideoFrameMetaData *redRenderBuffer) {
    std::unique_lock<std::mutex> lck(mRenderMutex);
#if defined(__APPLE__)
    if (mRenderGLView && mGLContext->getEaglContext()->isDisplay()) {
      mGLContext->getEaglContext()->useAsCurrent();
      int ret = _setInputFrame(redRenderBuffer);
      if (ret != RESULT_OK) {
        NEXT_LOGE(OPENGL_RENDER, "setInputImage error\n");
        return ret;
      }
      mGLContext->getEaglContext()->useAsPrev();
    }

#elif defined(__ANDROID__)
    int ret = SetInputFrame(redRenderBuffer);
    if (ret != RESULT_OK) {
        NEXT_LOGE(OPENGL_RENDER, "SetInputFrame error\n");
        return ret;
    }

#endif

    return RESULT_OK;
}

int OpenGLVideoRender::OnRender() {
    std::unique_lock<std::mutex> lock(mRenderMutex);
#if defined(__APPLE__)
    if (mRenderGLView && mGLContext->getEaglContext()->isDisplay()) {
      mGLContext->getEaglContext()->useAsCurrent();
      OnScreenRender();
      mGLContext->getEaglContext()->useAsPrev();
    }
#elif defined(__ANDROID__)
    OnScreenRender();
#endif
    return RESULT_OK;
}

int OpenGLVideoRender::OnRenderCacheFrame() {
#if defined(__APPLE__)
    std::unique_lock<std::mutex> lock(mRenderMutex);
    if (mRenderGLView && mGLContext->getEaglContext()->isDisplay() &&
        mCachedPixelBuffer) {
      if (mCachedPixelBuffer) {
        CVBufferRetain(mCachedPixelBuffer);
      }
      if (nullptr == mTextures[0] || nullptr == mTextures[1]) {
        if (nullptr != mTextures[0]) {
          mTextures[0].reset();
        }
        if (nullptr != mTextures[1]) {
          mTextures[1].reset();
        }
        mTextures[0] = Framebuffer::create();
        mTextures[1] = Framebuffer::create();
      }

      AVColorSpace colorspace = mGLContext->getEaglContext()->getColorSpace(
          mInputFrameData->pixel_buffer);
      if (colorspace != AVCOL_SPC_NB) {
        mInputFrameData->color_space = colorspace;
      }

      glActiveTexture(GL_TEXTURE0);
      mRenderGLTexture[0] =
          mGLContext->getEaglContext()->getGLTextureWithPixelBuffer(
              mRenderGLView.textureCache, mCachedPixelBuffer, 0,
              GL_TEXTURE_2D, GL_RED_EXT, GL_UNSIGNED_BYTE);
      if (mRenderGLTexture[0] == nullptr) {
        if (mCachedPixelBuffer) {
          CVBufferRelease(mCachedPixelBuffer);
        }
        mRenderGLTexture[0] = nullptr;
        mRenderGLTexture[1] = nullptr;
        NEXT_LOGE(OPENGL_RENDER, "renderGLTexture[0] is nullptr\n");
        return ERROR_RENDER_INPUT;
      }
      mTextures[0]->setTexture(mRenderGLTexture[0].texture);
      mTextures[0]->setWidth(mRenderGLTexture[0].width);
      mTextures[0]->setHeight(mRenderGLTexture[0].height);
      glBindTexture(GL_TEXTURE_2D, mTextures[0]->getTexture());
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
      glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
      glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

      glActiveTexture(GL_TEXTURE1);
      mRenderGLTexture[1] =
          mGLContext->getEaglContext()->getGLTextureWithPixelBuffer(
              mRenderGLView.textureCache, mCachedPixelBuffer, 1,
              GL_TEXTURE_2D, GL_RG_EXT, GL_UNSIGNED_BYTE);
      if (mRenderGLTexture[1] == nullptr) {
        if (mCachedPixelBuffer) {
          CVBufferRelease(mCachedPixelBuffer);
        }
        mRenderGLTexture[0] = nullptr;
        mRenderGLTexture[1] = nullptr;
        NEXT_LOGE(OPENGL_RENDER, "renderGLTexture[1] is nullptr\n");
        return ERROR_RENDER_INPUT;
      }
      mTextures[1]->setTexture(mRenderGLTexture[1].texture);
      mTextures[1]->setWidth(mRenderGLTexture[1].width);
      mTextures[1]->setHeight(mRenderGLTexture[1].height);
      glBindTexture(GL_TEXTURE_2D, mTextures[1]->getTexture());
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
      glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
      glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

      if (mCachedPixelBuffer) {
        CVBufferRelease(mCachedPixelBuffer);
      }
      if (mOpenglFilter) {
        if (mTextures[0]) {
          mInputFrameData = &mRenderMetaData;
        }
      }
      OnScreenRender();
    }
#endif
    return RESULT_OK;
}

int OpenGLVideoRender::SetInputFrame(VideoFrameMetaData *inputFrameMetaData) {
    if (inputFrameMetaData == nullptr) {
        NEXT_LOGE(OPENGL_RENDER, "inputFrameMetaData is nullptr\n");
        return ERROR_RENDER_INPUT;
    }
#if defined(__ANDROID__)
    EGLBoolean res = mGLContext->getEglContext()->MakeCurrent();
    if (!res) {
        NEXT_LOGE(OPENGL_RENDER, "SetInputFrame MakeCurrent error\n");
        return ERROR_RENDER_HANDLE;
    }
#endif
#if defined(__APPLE__)
    inputFrameMetaData->aspect_ratio = mRendererGravity;
#endif
    inputFrameMetaData->view_width = mRenderWidth;
    inputFrameMetaData->view_height = mRenderHeight;

    switch (inputFrameMetaData->pixel_format) {
        case PIXEL_FORMAT_RGB565:
            break;
        case PIXEL_FORMAT_RGB888:
            break;
        case PIXEL_FORMAT_RGBA8888:
            if (nullptr == inputFrameMetaData->pitches[0]) {
                NEXT_LOGE(OPENGL_RENDER, "inputFrameMetaData->pitches is nullptr\n");
                return ERROR_RENDER_INPUT;
            }
            if (-1 == mTextures[0] ||
                mRenderMetaData.frame_width != inputFrameMetaData->frame_width ||
                mRenderMetaData.frame_height != inputFrameMetaData->frame_height ||
                mRenderMetaData.linesize[0] != inputFrameMetaData->linesize[0]) {
                UpdateInputFrameData(inputFrameMetaData);
                if (mTextures[0] != -1) {
                    glDeleteTextures(1, &mTextures[0]);
                }
                mTextures[0] = CreateTexture();
            }

            glBindTexture(GL_TEXTURE_2D, mTextures[0]);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, inputFrameMetaData->linesize[0],
                         inputFrameMetaData->frame_height, 0, GL_RGBA,
                         GL_UNSIGNED_BYTE, inputFrameMetaData->pitches[0]);
            glBindTexture(GL_TEXTURE_2D, 0);
            break;
        case PIXEL_FORMAT_YUV420P:
            if (nullptr == inputFrameMetaData->pitches[0] ||
                nullptr == inputFrameMetaData->pitches[1] ||
                nullptr == inputFrameMetaData->pitches[2]) {
                NEXT_LOGE(OPENGL_RENDER, "inputFrameMetaData->pitches is nullptr\n");
                return ERROR_RENDER_INPUT;
            }

            if (-1 == mTextures[0] ||
                mRenderMetaData.frame_width != inputFrameMetaData->frame_width ||
                mRenderMetaData.frame_height != inputFrameMetaData->frame_height ||
                mRenderMetaData.linesize[0] != inputFrameMetaData->linesize[0] ||
                mRenderMetaData.linesize[1] != inputFrameMetaData->linesize[1] ||
                mRenderMetaData.linesize[2] != inputFrameMetaData->linesize[2]) {
                UpdateInputFrameData(inputFrameMetaData);
                if (mTextures[0] != -1) {
                    glDeleteTextures(1, &mTextures[0]);
                    glDeleteTextures(1, &mTextures[1]);
                    glDeleteTextures(1, &mTextures[2]);
                }
                mTextures[0] = CreateTexture();
                mTextures[1] = CreateTexture();
                mTextures[2] = CreateTexture();
            }

            glBindTexture(GL_TEXTURE_2D, mTextures[0]);
            glTexImage2D(
                    GL_TEXTURE_2D, 0, GL_LUMINANCE, inputFrameMetaData->linesize[0],
                    inputFrameMetaData->frame_height, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE,
                    inputFrameMetaData->pitches[0]);
            glBindTexture(GL_TEXTURE_2D, mTextures[1]);
            glTexImage2D(
                    GL_TEXTURE_2D, 0, GL_LUMINANCE, inputFrameMetaData->linesize[1],
                    inputFrameMetaData->frame_height / 2, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE,
                    inputFrameMetaData->pitches[1]);
            glBindTexture(GL_TEXTURE_2D, mTextures[2]);
            glTexImage2D(
                    GL_TEXTURE_2D, 0, GL_LUMINANCE, inputFrameMetaData->linesize[2],
                    inputFrameMetaData->frame_height / 2, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE,
                    inputFrameMetaData->pitches[2]);
            break;
        case PIXEL_FORMAT_YUV420SP:
            if (nullptr == inputFrameMetaData->pitches[0] ||
                nullptr == inputFrameMetaData->pitches[1]) {
                NEXT_LOGE(OPENGL_RENDER, "inputFrameMetaData->pitches is nullptr\n");
                return ERROR_RENDER_INPUT;
            }
            if (mTextures[0] == -1 ||
                mRenderMetaData.frame_width != inputFrameMetaData->frame_width ||
                mRenderMetaData.frame_height != inputFrameMetaData->frame_height ||
                mRenderMetaData.linesize[0] != inputFrameMetaData->linesize[0] ||
                mRenderMetaData.linesize[1] != inputFrameMetaData->linesize[1]) {
                UpdateInputFrameData(inputFrameMetaData);
                if (mTextures[0] != -1) {
                    glDeleteTextures(1, &mTextures[0]);
                    glDeleteTextures(1, &mTextures[1]);
                }
                mTextures[0] = CreateTexture();
                mTextures[1] = CreateTexture();
            }

            glBindTexture(GL_TEXTURE_2D, mTextures[0]);
            glTexImage2D(
                    GL_TEXTURE_2D, 0, GL_LUMINANCE, inputFrameMetaData->linesize[0],
                    inputFrameMetaData->frame_height, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE,
                    inputFrameMetaData->pitches[0]);
            glBindTexture(GL_TEXTURE_2D, mTextures[1]);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE_ALPHA,
                         inputFrameMetaData->linesize[1] / 2,
                         inputFrameMetaData->frame_height / 2, 0,
                         GL_LUMINANCE_ALPHA, GL_UNSIGNED_BYTE,
                         inputFrameMetaData->pitches[1]);
            glBindTexture(GL_TEXTURE_2D, 0);
            break;
        case PIXEL_FORMAT_VIDEOTOOLBOX: {
#if defined(__APPLE__)
            if (nullptr == inputFrameMetaData->pixel_buffer) {
              NEXT_LOGE(OPENGL_RENDER, "inputFrameMetaData->pitches is nullptr\n");
              return ERROR_RENDER_INPUT;
            }

            if (nullptr == mTextures[0] || nullptr == mTextures[1]) {
              if (nullptr != mTextures[0]) {
                mTextures[0].reset();
              }
              if (nullptr != mTextures[1]) {
                mTextures[1].reset();
              }
              mTextures[0] = Framebuffer::create();
              mTextures[1] = Framebuffer::create();
            }
            if (inputFrameMetaData->pixel_buffer) {
              CVBufferRetain(inputFrameMetaData->pixel_buffer);
            }

            AVColorSpace colorspace = mGLContext->getEaglContext()->getColorSpace(
                inputFrameMetaData->pixel_buffer);
            if (colorspace != AVCOL_SPC_NB) {
              inputFrameMetaData->color_space = colorspace;
            }

            glActiveTexture(GL_TEXTURE0);
            mRenderGLTexture[0] =
                mGLContext->getEaglContext()->getGLTextureWithPixelBuffer(
                    mRenderGLView.textureCache, inputFrameMetaData->pixel_buffer, 0,
                    GL_TEXTURE_2D, GL_RED_EXT, GL_UNSIGNED_BYTE);
            if (mRenderGLTexture[0] == nullptr) {
              if (inputFrameMetaData->pixel_buffer) {
                CVBufferRelease(inputFrameMetaData->pixel_buffer);
              }
              mRenderGLTexture[0] = nullptr;
              mRenderGLTexture[1] = nullptr;
              NEXT_LOGE(OPENGL_RENDER, "renderGLTexture[0] is nullptr\n");
              return ERROR_RENDER_INPUT;
            }
            mTextures[0]->setTexture(mRenderGLTexture[0].texture);
            mTextures[0]->setWidth(mRenderGLTexture[0].width);
            mTextures[0]->setHeight(mRenderGLTexture[0].height);
            glBindTexture(GL_TEXTURE_2D, mTextures[0]->getTexture());
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

            glActiveTexture(GL_TEXTURE1);
            mRenderGLTexture[1] =
                mGLContext->getEaglContext()->getGLTextureWithPixelBuffer(
                    mRenderGLView.textureCache, inputFrameMetaData->pixel_buffer, 1,
                    GL_TEXTURE_2D, GL_RG_EXT, GL_UNSIGNED_BYTE);
            if (mRenderGLTexture[1] == nullptr) {
              if (inputFrameMetaData->pixel_buffer) {
                CVBufferRelease(inputFrameMetaData->pixel_buffer);
              }
              mRenderGLTexture[0] = nullptr;
              mRenderGLTexture[1] = nullptr;
              NEXT_LOGE(OPENGL_RENDER, "renderGLTexture[1] is nullptr\n");
              return ERROR_RENDER_INPUT;
            }
            mTextures[1]->setTexture(mRenderGLTexture[1].texture);
            mTextures[1]->setWidth(mRenderGLTexture[1].width);
            mTextures[1]->setHeight(mRenderGLTexture[1].height);
            glBindTexture(GL_TEXTURE_2D, mTextures[1]->getTexture());
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

            if (mCachedPixelBuffer) {
              CVBufferRelease(mCachedPixelBuffer);
              mCachedPixelBuffer = nullptr;
            }
            mCachedPixelBuffer = inputFrameMetaData->pixel_buffer;
            if (mCachedPixelBuffer) {
              CVBufferRetain(mCachedPixelBuffer);
            }
            if (inputFrameMetaData->pixel_buffer) {
              CVBufferRelease(inputFrameMetaData->pixel_buffer);
            }
#endif
        }
            break;
        case PIXEL_FORMAT_YUV420P10LE:
            if (nullptr == inputFrameMetaData->pitches[0] ||
                nullptr == inputFrameMetaData->pitches[1] ||
                nullptr == inputFrameMetaData->pitches[2]) {
                NEXT_LOGE(OPENGL_RENDER, "inputFrameMetaData->pitches is nullptr\n");
                return ERROR_RENDER_INPUT;
            }

            if (mTextures[0] == -1 ||
                mRenderMetaData.frame_width != inputFrameMetaData->frame_width ||
                mRenderMetaData.frame_height != inputFrameMetaData->frame_height ||
                mRenderMetaData.linesize[0] != inputFrameMetaData->linesize[0] ||
                mRenderMetaData.linesize[1] != inputFrameMetaData->linesize[1] ||
                mRenderMetaData.linesize[2] != inputFrameMetaData->linesize[2]) {
                UpdateInputFrameData(inputFrameMetaData);
                if (mTextures[0] != -1) {
                    glDeleteTextures(1, &mTextures[0]);
                    glDeleteTextures(1, &mTextures[1]);
                    glDeleteTextures(1, &mTextures[2]);
                }
                mTextures[0] = CreateTexture();
                mTextures[1] = CreateTexture();
                mTextures[2] = CreateTexture();
            }

            glBindTexture(GL_TEXTURE_2D, mTextures[0]);
            glTexImage2D(
                    GL_TEXTURE_2D, 0, GL_LUMINANCE, inputFrameMetaData->linesize[0] / 2,
                    inputFrameMetaData->frame_height, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE,
                    inputFrameMetaData->pitches[0]);
            glBindTexture(GL_TEXTURE_2D, mTextures[1]);
            glTexImage2D(
                    GL_TEXTURE_2D, 0, GL_LUMINANCE, inputFrameMetaData->linesize[1] / 2,
                    inputFrameMetaData->frame_height, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE,
                    inputFrameMetaData->pitches[1]);
            glBindTexture(GL_TEXTURE_2D, mTextures[2]);
            glTexImage2D(
                    GL_TEXTURE_2D, 0, GL_LUMINANCE, inputFrameMetaData->linesize[2] / 2,
                    inputFrameMetaData->frame_height, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE,
                    inputFrameMetaData->pitches[2]);
            break;
        case PIXEL_FORMAT_MEDIACODEC:
            break;
        default:
            break;
    }

    UpdateInputFrameData(inputFrameMetaData);

    if (nullptr == mOpenglFilter ||
        mOpenglFilter->getPixelFormat() != inputFrameMetaData->pixel_format) {
        if (mOpenglFilter != nullptr) {
            mOpenglFilter.reset();
            mOpenglFilter = nullptr;
        }
        int ret = CreateOnScreenRender(&mRenderMetaData);
        if (ret != RESULT_OK) {
            NEXT_LOGE(OPENGL_RENDER, "createScreenRender error\n");
            return ret;
        }
    }

    if (mOpenglFilter && mTextures[0] != -1) {
        mInputFrameData = inputFrameMetaData;
    }
    return RESULT_OK;
}

int OpenGLVideoRender::OnScreenRender() {
    if (nullptr == mOpenglFilter) {
        NEXT_LOGE(OPENGL_RENDER, "openglFilterDevice is nullptr\n");
        return ERROR_RENDER_VIDEO_INIT;
    }
    if (nullptr == mInputFrameData) {
        NEXT_LOGE(OPENGL_RENDER, "inputFrameData is nullptr\n");
        return ERROR_RENDER_VIDEO_INIT;
    }

    switch (mInputFrameData->pixel_format) {
        case PIXEL_FORMAT_RGB565:
        case PIXEL_FORMAT_RGB888:
        case PIXEL_FORMAT_RGBA8888:
            mOpenglFilter->SetInputTexture(mTextures[0], 0);
            break;
        case PIXEL_FORMAT_YUV420P:
        case PIXEL_FORMAT_YUV420SP:
            for (int i = 0; i < 3; i++) {
                mOpenglFilter->SetInputTexture(mTextures[i], i);
            }
            break;
        case PIXEL_FORMAT_VIDEOTOOLBOX:
            for (int i = 0; i < 2; i++) {
                mOpenglFilter->SetInputTexture(mTextures[i], i);
            }
            break;
        case PIXEL_FORMAT_YUV420P10LE:
            break;
        case PIXEL_FORMAT_MEDIACODEC:
            break;
        default:
            break;
    }

#if defined(__ANDROID__)
    EGLBoolean res = mGLContext->getEglContext()->MakeCurrent();
    if (!res) {
        NEXT_LOGE(OPENGL_RENDER, "MakeCurrent error\n");
        return ERROR_RENDER_VIDEO_CTX;
    }
#endif
    mOpenglFilter->SetInputFrameMetaData(mInputFrameData);
    mOpenglFilter->UpdateParam();
    mOpenglFilter->OnRender();
#if defined(__APPLE__)
    mRenderGLTexture[0] = nullptr;
    mRenderGLTexture[1] = nullptr;
#endif

    uint64_t current = CurrentTimeMs();
    uint64_t delta = (current > mLastFrameTime) ? current - mLastFrameTime : 0;
    if (delta <= 0) {
        mLastFrameTime = current;
    } else if (delta >= 1000) {
        mFps = static_cast<double>(mFrameCount) * 1000 / static_cast<double>(delta);
        mLastFrameTime = current;
        mFrameCount = 0;
    } else {
        mFrameCount++;
    }

    return RESULT_OK;
}

int OpenGLVideoRender::CreateOnScreenRender(VideoFrameMetaData *inputFrameMetaData) {
    mOpenglFilter = OpenGLFilter::create(inputFrameMetaData, mGLContext);
    if (nullptr == mOpenglFilter) {
        NEXT_LOGE(OPENGL_RENDER, "openglFilter nullptr\n");
        return ERROR_RENDER_FILTER;
    }

    return RESULT_OK;
}

void OpenGLVideoRender::UpdateInputFrameData(VideoFrameMetaData *inputFrameMetaData) {
    mRenderMetaData = *inputFrameMetaData;
}
