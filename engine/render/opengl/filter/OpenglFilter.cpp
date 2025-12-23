/**
 * Note: OpenGL filter
 *
 * Date: 2025/12/23
 * Author: frank
 */

#include "./OpenglFilter.h"

#include "NextErrorCode.h"
#include "NextLog.h"

#ifndef OPENGL_FILTER
#define OPENGL_FILTER "OpenglFilter"
#endif

OpenGLFilter::OpenGLFilter()
        : BaseFilter() {

}

OpenGLFilter::~OpenGLFilter() = default;

std::shared_ptr<BaseFilter>
OpenGLFilter::create(VideoFrameMetaData *inputFrameMetaData,
                           std::shared_ptr<OpenGLContext> context) {
    std::shared_ptr<OpenGLFilter> ptr = std::make_shared<OpenGLFilter>();

    int ret = ptr->init(inputFrameMetaData, context);
    if (ret != RESULT_OK) {
        NEXT_LOGE(OPENGL_FILTER, "OpenGL Filter InitInternal error, ret=%d\n", ret);
        return nullptr;
    }

    return ptr;
}

int OpenGLFilter::init(VideoFrameMetaData *inputFrameMetaData,
                             std::shared_ptr<OpenGLContext> context) {
    std::string error{};
    mGLContext = context;
    mInputFrameData = inputFrameMetaData;
    mFixelFormat = inputFrameMetaData->pixel_format;
    switch (mInputFrameData->pixel_format) {
        case PIXEL_FORMAT_RGB565:
        case PIXEL_FORMAT_RGB888:
        case PIXEL_FORMAT_RGBA8888: {
            if (!InitWithFragmentShader(GetFragmentShader(), 1)) {
                error = "init rgb shader error!";
            }
        }
            break;
        case PIXEL_FORMAT_YUV420P: {
            std::string shader = GetYUV420p2RGBFragmentShader(mInputFrameData->color_range == AVCOL_RANGE_JPEG);
            if (!InitWithFragmentShader(shader, 3)) {
                error = "init yuv420p shader error!";
            }
        }
            break;
        case PIXEL_FORMAT_YUV420SP: {
            std::string shader = GetYUV420sp2RGBFragmentShader(mInputFrameData->color_range == AVCOL_RANGE_JPEG);
            if (!InitWithFragmentShader(shader, 2)) {
                error = "init yuv420sp shader error!";
            }
        }
            break;
        case PIXEL_FORMAT_VIDEOTOOLBOX: {
            std::string shader = GetYUV420sp2RGBFragmentShader(mInputFrameData->color_range == AVCOL_RANGE_JPEG);
            if (!InitWithFragmentShader(shader, 2)) {
                error = "init yuv420sp_vtb shader error!";
            }
        }
            break;
        case PIXEL_FORMAT_YUV420P10LE: {
            std::string shader = GetYUV420p10le2RGBFragmentShader(mInputFrameData->color_range == AVCOL_RANGE_JPEG);
            if (!InitWithFragmentShader(shader, 2)) {
                error = "init yuv420p10le shader error!";
            }
        }
            break;
        case PIXEL_FORMAT_MEDIACODEC:
            break;
        default:
            break;
    }
    if (!error.empty()) {
        NEXT_LOGE(OPENGL_FILTER, "%s\n", error.c_str());
        return ERROR_RENDER_VIDEO_INIT;
    }
    return RESULT_OK;
}

int OpenGLFilter::OnRender() {
    const float *matrix = nullptr;
    GLint uColorConversion = glGetUniformLocation(mProgram, "uColorConversion");
    switch (mInputFrameData->color_space) {
        case AVCOL_SPC_RGB:
            break;
        case AVCOL_SPC_BT709:
        case AVCOL_SPC_SMPTE240M: {
            matrix = mInputFrameData->color_range == AVCOL_RANGE_JPEG ?
                    Bt709FullRangeYUV2RGBMatrix : Bt709LimitRangeYUV2RGBMatrix;
            glUniformMatrix3fv(uColorConversion, 1, GL_FALSE, const_cast<GLfloat *>(matrix));
            break;
        }
        case AVCOL_SPC_BT470BG:
        case AVCOL_SPC_SMPTE170M: {
            matrix = mInputFrameData->color_range == AVCOL_RANGE_JPEG ?
                     Bt601FullRangeYUV2RGBMatrix : Bt601LimitRangeYUV2RGBMatrix;
            glUniformMatrix3fv(uColorConversion, 1, GL_FALSE, const_cast<GLfloat *>(matrix));
        }
            break;
        case AVCOL_SPC_BT2020_NCL:
        case AVCOL_SPC_BT2020_CL: {
            matrix = mInputFrameData->color_range == AVCOL_RANGE_JPEG ?
                     Bt2020FullRangeYUV2RGBMatrix : Bt2020LimitRangeYUV2RGBMatrix;
            glUniformMatrix3fv(uColorConversion, 1, GL_FALSE, const_cast<GLfloat *>(matrix));
        }
            break;
        default: {
            if (mInputFrameData->pixel_format == PIXEL_FORMAT_YUV420P ||
                mInputFrameData->pixel_format == PIXEL_FORMAT_YUV420SP ||
                mInputFrameData->pixel_format == PIXEL_FORMAT_VIDEOTOOLBOX ||
                mInputFrameData->pixel_format == PIXEL_FORMAT_YUV420P10LE) {
                matrix = mInputFrameData->color_range == AVCOL_RANGE_JPEG ?
                         Bt709FullRangeYUV2RGBMatrix : Bt709LimitRangeYUV2RGBMatrix;
                glUniformMatrix3fv(uColorConversion, 1, GL_FALSE, const_cast<GLfloat *>(matrix));
            }
        }
            break;
    }
    return BaseFilter::OnRender();
}
