/**
 * Note: information of video render
 * Date: 2025/12/12
 * Author: frank
 */

#ifndef VIDEO_RENDER_INFO_H
#define VIDEO_RENDER_INFO_H

#include "NextDefine.h"

#ifdef __cplusplus
extern "C" {
#endif
#include "libavutil/pixfmt.h"
#include "libavutil/rational.h"
#ifdef __cplusplus
}
#endif

#if defined(__APPLE__)
#import <CoreVideo/CoreVideo.h>
#endif

enum VideoRenderType {
    VIDEO_RENDER_UNKNOWN      = 0,
    VIDEO_RENDER_OPENGL       = 1,
    VIDEO_RENDER_METAL        = 2,
    VIDEO_RENDER_MEDIACODEC   = 3,
    VIDEO_RENDER_HARMONY      = 4,
    VIDEO_RENDER_SAMPLEBUFFER = 5
};

enum VideoFilterType {
    VIDEO_FILTER_OPENGL = 0,
    VIDEO_FILTER_METAL  = 1
};

enum RotationMode {
    ROTATION_NO     = 0,
    ROTATION_90     = 1,
    ROTATION_180    = 2,
    ROTATION_270    = 3,
    ROTATION_FLIP_V = 4, // vertical flip
    ROTATION_FLIP_H = 5  // horizontal flip
};

enum AspectRatioMode {
    ASPECT_RATIO_FIT_WIDTH,  // default mode
    ASPECT_RATIO_FIT_HEIGHT, // fit height
    ASPECT_RATIO_FIT,        // fit w and h of video
    ASPECT_RATIO_FILL,       // fill w and h of screen
    ASPECT_SCALE_FILL        // scale w and h to fill screen
};

typedef struct VideoFrameMetaData {
#if defined(__APPLE__)
    CVPixelBufferRef pixel_buffer;
#endif
    uint8_t *pitches[3];
    int linesize[3];
    int32_t stride;

    int view_width;
    int view_height;
    int frame_width;
    int frame_height;

    AVRational sar;
    AVColorSpace color_space;
    AVColorRange color_range;
    RotationMode rotation_mode;
    AspectRatioMode aspect_ratio;
    AVColorPrimaries color_primaries;
    VideoPixelFormat pixel_format;
    AVColorTransferCharacteristic color_trc;
} VideoFrameMetaData;

typedef struct VideoRenderBufferContext {
    int buffer_index;
    void *buffer_data;
    int decoder_serial;
    void *decoder;
    void *opaque;

    void (*release_buffer)(struct VideoRenderBufferContext *context, bool render);
} VideoRenderBufferContext;

#endif
