#if defined(__ANDROID__)
#ifndef MEDIACODEC_RENDER_H
#define MEDIACODEC_RENDER_H

#include "../../render/VideoRender.h"

class MediaCodecRender : public VideoRender {
public:
    MediaCodecRender();

    ~MediaCodecRender() override;

    int OnRender(VideoRenderBufferContext *bufferContext, bool render) override;
};

#endif // MEDIACODEC_RENDER_H
#endif // defined(__ANDROID__)
