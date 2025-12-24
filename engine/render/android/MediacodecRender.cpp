/**
 * Note: video render of MediaCodec
 * Date: 2025/12/123
 * Author: frank
 */

#if defined(__ANDROID__)

#include "MediacodecRender.h"
#include "NextLog.h"

MediaCodecRender::MediaCodecRender()
        : VideoRender() {

}

MediaCodecRender::~MediaCodecRender() = default;

int MediaCodecRender::OnRender(VideoRenderBufferContext *bufferContext, bool render) {
    if (nullptr == bufferContext || nullptr == bufferContext->release_buffer) {
        NEXT_LOGE("MediaCodecRender", "bufferContext is null");
        return ERROR_RENDER_VIDEO_INIT;
    }
    bufferContext->release_buffer(bufferContext, render);
    return RESULT_OK;
}

#endif
