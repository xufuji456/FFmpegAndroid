/**
 * Note: base filter of OpenGL
 *
 * Date: 2025/12/22
 * Author: frank
 */

#include "./BaseFilter.h"

#include <chrono>
#include <cmath>
#include <sstream>

#include "../common/OpenGLUtil.h"
#include "../common/OpenglContext.h"
#include "../common/RotationHelper.h"
#include "NextErrorCode.h"
#include "NextLog.h"

#define OPENGL_FILTER "OpenglFilter"

BaseFilter::BaseFilter() {
    mTextureScale  = 1.0f;
    mCurrentWidth  = 0;
    mCurrentHeight = 0;
}

BaseFilter::~BaseFilter() {
    if (mProgram >= 0) {
        glDeleteProgram(mProgram);
        mProgram = -1;
    }
}

int BaseFilter::init(VideoFrameMetaData *inputFrameMetaData, std::shared_ptr<OpenGLContext> context) {
    return RESULT_OK;
}

bool BaseFilter::InitWithFragmentShader(const std::string &fragmentShader, int inputTexNum) {
    mInputTexNum = inputTexNum;
    return InitWithShader(GetVertexShaderString(inputTexNum), fragmentShader);
}

bool BaseFilter::InitWithShader(
        const std::string &vertexShader,
        const std::string &fragmentShader) {
    mProgram = CreateProgram(vertexShader, fragmentShader);
    if (mProgram == -1) {
        NEXT_LOGE(OPENGL_FILTER, "create program error!\n");
        return false;
    }
    glUseProgram(mProgram);
    glGetAttribLocation(mProgram, "position");
    glEnableVertexAttribArray(mPositionAttribute);
    return true;
}

int BaseFilter::OnRender() {
#if defined(__ANDROID__) || defined(__HARMONY__)
    EGLBoolean res = mGLContext->getEglContext()->MakeCurrent();
    if (!res) {
        NEXT_LOGE(OPENGL_FILTER, "MakeCurrent error\n");
        return ERROR_RENDER_HANDLE;
    }
#endif
    glUseProgram(mProgram);
#if defined(__ANDROID__) || defined(__HARMONY__)
    glViewport(0, 0, mCurrentWidth, mCurrentHeight);
    mGLContext->getEglContext()->SetSurfaceSize(mCurrentWidth, mCurrentHeight);
#elif defined(__APPLE__)
    mGLContext->getEaglContext()->useAsCurrent();
    mGLContext->getEaglContext()->activeFramebuffer();
#endif
    glClearColor(mBackgroundColor.r, mBackgroundColor.g,
                 mBackgroundColor.b, mBackgroundColor.a);
    glClear(GL_COLOR_BUFFER_BIT);

    for (int i = 0; i < mInputTexNum; i++) {
        int texIdx = i;
        glActiveTexture(GL_TEXTURE0 + texIdx);
        glBindTexture(GL_TEXTURE_2D, mTextures[i]);

        std::string strColorMap = (texIdx == 0 ? "colorMap" : "colorMap" + std::to_string(texIdx));
        GLint uColorMap = glGetUniformLocation(mProgram, strColorMap.c_str());
        glUniform1i(uColorMap, texIdx);
        std::string strCoordinate = (texIdx == 0 ? "texCoord" : "texCoord" + std::to_string(texIdx));
        GLuint texCoordAttribute = glGetAttribLocation(mProgram, strCoordinate.c_str());

        glEnableVertexAttribArray(texCoordAttribute);
        glVertexAttribPointer(
                texCoordAttribute, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float),
                GetTextureCoordinate(mInputFrameData->rotation_mode));
    }

    GLint uMvp = glGetUniformLocation(mProgram, "uMvp");
    glUniformMatrix4fv(uMvp, 1, GL_FALSE, const_cast<GLfloat *>(ModelViewProjectionMatrix));
    glVertexAttribPointer(mPositionAttribute, 2, GL_FLOAT,
                          GL_FALSE, 2 * sizeof(float), GetVertexCoordinate());
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

#if defined(__ANDROID__) || defined(__HARMONY__)
    mGLContext->getEglContext()->SwapBuffers();
#elif defined(__APPLE__)
    mGLContext->getEaglContext()->presentBufferForDisplay();
    mGLContext->getEaglContext()->inActiveFramebuffer();
    mGLContext->getEaglContext()->useAsPrev();
#endif
    return RESULT_OK;
}

void BaseFilter::UpdateParam() {
    int width  = mInputFrameData->frame_width;
    int height = mInputFrameData->frame_height;
    if (mInputFrameData->rotation_mode == ROTATION_90 || mInputFrameData->rotation_mode == ROTATION_270) {
        width  = mInputFrameData->frame_height;
        height = mInputFrameData->frame_width;
        mInputFrameData->frame_width  = width;
        mInputFrameData->frame_height = height;
    }
    if (mTextureScale != 1.0) {
        width  = static_cast<int>(mTextureScale * (float)width);
        height = static_cast<int>(mTextureScale * (float)height);
        mInputFrameData->frame_width  = width;
        mInputFrameData->frame_height = height;
    }

    if (mInputFrameData->linesize[0] > mInputFrameData->frame_width) {
        mPaddingPixels = mInputFrameData->linesize[0] - mInputFrameData->frame_width;
    }

//    bool onlyTexture = false;
//    if (getOpenGLFilterBaseType() == VIDEO_FILTER_OPENGL) {
//        onlyTexture = true;
//    }
    if (mTextureId == -1 || mCurrentWidth != width || mCurrentHeight != height) {
        if (mTextureId != -1) {
            glDeleteTextures(1, &mTextureId);
        }
        if (mFrameBufferId != -1) {
            glDeleteFramebuffers(1, &mFrameBufferId);
        }
        mCurrentWidth  = width;
        mCurrentHeight = height;
#if defined(__ANDROID__) || defined(__HARMONY__)
        mTextureId = CreateTexture();
#elif defined(__APPLE__)
        if (getOpenGLFilterBaseType() != VIDEO_FILTER_OPENGL) {
            mTextureId = CreateTexture();
            if (!onlyTexture) {
                mFrameBufferId = CreateFramebuffer(mCurrentWidth, mCurrentHeight, mTextureId);
            }
        }
#endif
    }
}

void BaseFilter::SetInputFrameMetaData(
        VideoFrameMetaData *inputFrameMetaData) {
    mInputFrameData = inputFrameMetaData;
}

void BaseFilter::SetInputTexture(GLuint textureId, int index) {
    mTextures[index] = textureId;
}

const GLfloat *BaseFilter::GetVertexCoordinate() {
    DefaultVertexCoordinate();

    if (mInputFrameData->view_width < 0 ||
        mInputFrameData->view_height < 0 ||
        mInputFrameData->frame_width < 0 ||
        mInputFrameData->frame_height < 0) {
        NEXT_LOGE(OPENGL_FILTER, "GetVertexCoordinate error, view_w=%d, view_h=%d, frame_w=%d, frame_h=%d",
                  mInputFrameData->view_width,
                  mInputFrameData->view_height,
                  mInputFrameData->frame_width,
                  mInputFrameData->frame_height);
        return mVertexCoordinate;
    }

    int frameWidth  = mInputFrameData->frame_width;
    int frameHeight = mInputFrameData->frame_height;
    float x_Value = 1.0f;
    float y_Value = 1.0f;

    switch (mInputFrameData->aspect_ratio) {
        case ASPECT_RATIO_FIT_WIDTH: {
            if (mInputFrameData->sar.num > 0 && mInputFrameData->sar.den > 0) {
                frameHeight = frameHeight * mInputFrameData->sar.den / mInputFrameData->sar.num;
            }
            y_Value = (((float)frameHeight * 1.0f / (float)frameWidth) *
                    (float)mInputFrameData->view_width / (float)mInputFrameData->view_height);
            mVertexCoordinate[1] = -1.0f * y_Value;
            mVertexCoordinate[3] = -1.0f * y_Value;
            mVertexCoordinate[5] = 1.0f * y_Value;
            mVertexCoordinate[7] = 1.0f * y_Value;
        }
            break;
        case ASPECT_RATIO_FIT_HEIGHT: {
            if (mInputFrameData->sar.num > 0 && mInputFrameData->sar.den > 0) {
                frameWidth = frameWidth * mInputFrameData->sar.num / mInputFrameData->sar.den;
            }
            x_Value = (((float)frameWidth * 1.0f / (float)frameHeight) *
                    (float)mInputFrameData->view_height / (float)mInputFrameData->view_width);
            mVertexCoordinate[0] = -1.0f * x_Value;
            mVertexCoordinate[2] = 1.0f * x_Value;
            mVertexCoordinate[4] = -1.0f * x_Value;
            mVertexCoordinate[6] = 1.0f * x_Value;
        }
            break;
        case ASPECT_RATIO_FIT: {
            if (mInputFrameData->sar.num > 0 && mInputFrameData->sar.den > 0) {
                frameWidth = frameWidth * mInputFrameData->sar.num / mInputFrameData->sar.den;
            }
            const float dW = static_cast<float>(mInputFrameData->view_width) / (float)mInputFrameData->frame_width;
            const float dH = static_cast<float>(mInputFrameData->view_height) / (float)mInputFrameData->frame_height;

            float dd = 1.0f;
            float nW = 1.0f;
            float nH = 1.0f;
            dd = fmin(dW, dH);

            nW = ((float)mInputFrameData->frame_width * dd / static_cast<float>(mInputFrameData->view_width));
            nH = ((float)mInputFrameData->frame_height * dd / static_cast<float>(mInputFrameData->view_height));

            mVertexCoordinate[0] = -nW;
            mVertexCoordinate[1] = -nH;
            mVertexCoordinate[2] = nW;
            mVertexCoordinate[3] = -nH;
            mVertexCoordinate[4] = -nW;
            mVertexCoordinate[5] = nH;
            mVertexCoordinate[6] = nW;
            mVertexCoordinate[7] = nH;
        }
            break;
        case ASPECT_RATIO_FILL: {
            if (mInputFrameData->sar.num > 0 && mInputFrameData->sar.den > 0) {
                frameWidth = frameWidth * mInputFrameData->sar.num / mInputFrameData->sar.den;
            }
            float dW = static_cast<float>(mInputFrameData->view_width) / (float)mInputFrameData->frame_width;
            float dH = static_cast<float>(mInputFrameData->view_height) / (float)mInputFrameData->frame_height;

            float dd = 1.0f;
            float nW = 1.0f;
            float nH = 1.0f;
            dd = fmax(dW, dH);

            nW = ((float)mInputFrameData->frame_width * dd / static_cast<float>(mInputFrameData->view_width));
            nH = ((float)mInputFrameData->frame_height * dd / static_cast<float>(mInputFrameData->view_height));

            mVertexCoordinate[0] = -nW;
            mVertexCoordinate[1] = -nH;
            mVertexCoordinate[2] = nW;
            mVertexCoordinate[3] = -nH;
            mVertexCoordinate[4] = -nW;
            mVertexCoordinate[5] = nH;
            mVertexCoordinate[6] = nW;
            mVertexCoordinate[7] = nH;
        }
            break;
        default:
            break;
    }
    return mVertexCoordinate;
}

const GLfloat* BaseFilter::GetTextureCoordinate(const RotationMode &rotationMode) {
    // TODO: update textureCoordinate when rotate changed
    UpdateTextureCoordinate(rotationMode);
    if (mPaddingPixels > 0) {
        GLfloat cropSize = (GLfloat) mPaddingPixels / (GLfloat) mInputFrameData->linesize[0];
        CropTextureCoordinate(rotationMode, cropSize);
    }
    return mTextureCoordinate;
}

void BaseFilter::UpdateTextureCoordinate(const RotationMode &rotationMode) {
    switch (rotationMode) {
        case ROTATION_90:
            memcpy(mTextureCoordinate, TextureRotation90, 8 * sizeof(float));
            break;
        case ROTATION_180:
            memcpy(mTextureCoordinate, TextureRotation180, 8 * sizeof(float));
            break;
        case ROTATION_270:
            memcpy(mTextureCoordinate, TextureRotation270, 8 * sizeof(float));
            break;
        case ROTATION_FLIP_V:
            memcpy(mTextureCoordinate, TextureRotationFlipV, 8 * sizeof(float));
            break;
        case ROTATION_FLIP_H:
            memcpy(mTextureCoordinate, TextureRotationFlipH, 8 * sizeof(float));
            break;
        case ROTATION_NO:
        default:
            memcpy(mTextureCoordinate, TextureRotationNo, 8 * sizeof(float));
            break;
    }
}

void BaseFilter::DefaultVertexCoordinate() {
    mVertexCoordinate[0] = -1.0f;
    mVertexCoordinate[1] = -1.0f;
    mVertexCoordinate[2] = 1.0f;
    mVertexCoordinate[3] = -1.0f;
    mVertexCoordinate[4] = -1.0f;
    mVertexCoordinate[5] = 1.0f;
    mVertexCoordinate[6] = 1.0f;
    mVertexCoordinate[7] = 1.0f;
}

void BaseFilter::CropTextureCoordinate(RotationMode rotationMode, GLfloat cropSize) {
    switch (rotationMode) {
        case ROTATION_90:
            mTextureCoordinate[0] = mTextureCoordinate[0] - cropSize;
            mTextureCoordinate[2] = mTextureCoordinate[2] - cropSize;
            break;
        case ROTATION_180:
            mTextureCoordinate[0] = mTextureCoordinate[0] - cropSize;
            mTextureCoordinate[4] = mTextureCoordinate[4] - cropSize;
            break;
        case ROTATION_270:
            mTextureCoordinate[4] = mTextureCoordinate[4] - cropSize;
            mTextureCoordinate[6] = mTextureCoordinate[6] - cropSize;
            break;
        case ROTATION_FLIP_V:
            mTextureCoordinate[2] = mTextureCoordinate[2] - cropSize;
            mTextureCoordinate[6] = mTextureCoordinate[6] - cropSize;
            break;
        case ROTATION_FLIP_H:
            mTextureCoordinate[0] = mTextureCoordinate[0] - cropSize;
            mTextureCoordinate[4] = mTextureCoordinate[4] - cropSize;
            break;
        case ROTATION_NO:
        default:
            mTextureCoordinate[2] = mTextureCoordinate[2] - cropSize;
            mTextureCoordinate[6] = mTextureCoordinate[6] - cropSize;
            break;
    }
}

int BaseFilter::getPixelFormat() const {
    return mPixelFormat;
}
