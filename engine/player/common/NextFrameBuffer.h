
#ifndef NEXT_FRAME_BUFFER_H
#define NEXT_FRAME_BUFFER_H

#include "NextDefine.h"

#include "decode/VideoDecoder.h"

#ifdef __cplusplus
extern "C" {
#endif
#include "libavutil/channel_layout.h"
#ifdef __cplusplus
}
#endif

#if defined(__APPLE__)
#import <CoreVideo/CoreVideo.h>
#endif

class FrameBuffer {
public:
    FrameBuffer() = default;

    ~FrameBuffer() {
        Release();
    }

    void Release() {
        if (data) {
            delete[] data;
            data = nullptr;
        }
        if (opaque) {
            switch (pixel_format) {
                case PIXEL_FORMAT_MEDIACODEC: {
                    auto *ctx = reinterpret_cast<MediaCodecBufferContext *>(opaque);
                    ctx->release_output_buffer(ctx, false);
                    delete reinterpret_cast<MediaCodecBufferContext *>(opaque);
                    break;
                }
                case PIXEL_FORMAT_VIDEOTOOLBOX: {
#if defined(__APPLE__)
                    if (opaque &&
                    (reinterpret_cast<VideoToolBufferContext *>(opaque))->buffer) {
                  CVBufferRelease(
                      (CVPixelBufferRef)(reinterpret_cast<VideoToolBufferContext *>(opaque))
                          ->buffer);
                  delete reinterpret_cast<VideoToolBufferContext *>(opaque);
                  opaque = nullptr;
                }
#endif
                    break;
                }
                case PIXEL_FORMAT_YUV420P:
                case PIXEL_FORMAT_YUVJ420P:
                case PIXEL_FORMAT_YUV420P10LE: {
                    auto *ctx = reinterpret_cast<FFmpegBufferContext *>(opaque);
                    ctx->release_frame(ctx);
                    delete reinterpret_cast<FFmpegBufferContext *>(opaque);
                    break;
                }
                case PIXEL_FORMAT_HARMONY: {
                    auto *ctx = reinterpret_cast<HarmonyMediaBufferContext *>(opaque);
                    ctx->release_output_buffer(ctx, false);
                    delete reinterpret_cast<HarmonyMediaBufferContext *>(opaque);
                    break;
                }

                default:
                    break;
            }

            if (isAudio) {
                auto *ctx = reinterpret_cast<FFmpegBufferContext *>(opaque);
                ctx->release_frame(ctx);
                delete reinterpret_cast<FFmpegBufferContext *>(opaque);
            }

            opaque = nullptr;
        }
        for (auto &ch: channel) {
            ch = nullptr;
        }
        yBuffer = nullptr;
        uBuffer = nullptr;
        vBuffer = nullptr;
    }

    FrameBuffer(FrameBuffer &buffer) = delete;

    FrameBuffer(FrameBuffer &&buffer) = delete;

    FrameBuffer &operator=(FrameBuffer &buffer) = delete;

    FrameBuffer &operator=(FrameBuffer &&buffer) = delete;

public:
    int format    = 0;
    int serial    = -1;
    int64_t dts   = 0;
    int64_t pts   = 0;
    bool isAudio  = false;
    void *opaque  = nullptr;
    uint8_t *data = nullptr;

    int width   = 0;
    int height  = 0;
    int yStride = 0;
    int uStride = 0;
    int vStride = 0;
    uint8_t *yBuffer = nullptr;
    uint8_t *uBuffer = nullptr;
    uint8_t *vBuffer = nullptr;
    VideoPixelFormat pixel_format = PIXEL_FORMAT_UNKNOWN;

    int sampleSize  = 0;
    int sampleRate  = 0;
    int numChannels = 0;
    AVChannelLayout *channelLayout = nullptr;
    uint8_t *channel[8]{};

};

#endif //NEXT_FRAME_BUFFER_H
