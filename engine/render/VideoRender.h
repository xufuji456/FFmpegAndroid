/**
 * Note: interface of video render
 * Date: 2025/12/13
 * Author: frank
 */

#ifndef VIDEO_RENDER_H
#define VIDEO_RENDER_H

#if defined(__ANDROID__)
#include <android/native_window.h>
#endif

#include "VideoRenderInfo.h"
#include "NextErrorCode.h"

class VideoRender {
public:
    VideoRender() = default;

    virtual ~VideoRender() = default;

#if defined(__ANDROID__)

    virtual int Init() {return RESULT_OK;};

    virtual int SetSurface(ANativeWindow *nativeWindow) {return RESULT_OK;};

#elif defined(__APPLE__)

    virtual int Init(){return RESULT_OK;};

    virtual int InitWithFrame(CGRect cgrect) {return RESULT_OK;};

    virtual UIView *getRedRenderView() {return nil;};

#endif

    virtual int AttachFilter(VideoFilterType videoFilterType,
                             VideoFrameMetaData *inputFrameMetaData) {return RESULT_OK;};

    virtual int DetachFilter(VideoFilterType videoFilterType) {return RESULT_OK;};

    virtual int DetachAllFilter() {return RESULT_OK;};

    virtual int OnInputFrame(VideoFrameMetaData *redRenderBuffer) {return RESULT_OK;};

    virtual int OnRender() {return RESULT_OK;};

    virtual int OnRender(VideoRenderBufferContext *bufferContext, bool render) {return RESULT_OK;};

    virtual int OnRenderCacheFrame() {return RESULT_OK;};

    virtual int ReleaseContext() {return RESULT_OK;};

    virtual void Close() {};

};

#endif
