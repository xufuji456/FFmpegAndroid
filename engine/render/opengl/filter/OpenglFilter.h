#ifndef OPENGL_FILTER_H
#define OPENGL_FILTER_H

#include "../../VideoRenderInfo.h"
#include "./BaseFilter.h"

class OpenGLFilter : public BaseFilter {
public:
    OpenGLFilter();

    ~OpenGLFilter() override;

    static std::shared_ptr<BaseFilter>
    create(VideoFrameMetaData *inputFrameMetaData, std::shared_ptr<OpenGLContext> context);

    int init(VideoFrameMetaData *inputFrameMetaData, std::shared_ptr<OpenGLContext> context) override;

    int OnRender() override;

};

#endif //OPENGL_FILTER_H
