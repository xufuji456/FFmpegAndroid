#ifndef MIXED_BUFFER_H
#define MIXED_BUFFER_H

#include <cstdint>
#include <vector>

#include "VideoCodecInfo.h"
#include "NextDefine.h"

#ifdef __cplusplus
extern "C" {
#endif
#include "libavutil/pixfmt.h"
#include "libavutil/channel_layout.h"
#ifdef __cplusplus
}
#endif

struct MediaCodecBufferContext {
    void *opaque;
    void *decoder;
    int buffer_index;
    void *media_codec;
    int decoder_serial;

    void (*release_output_buffer)(MediaCodecBufferContext *context, bool render);
};

struct HarmonyMediaBufferContext {
    void *opaque;
    void *decoder;
    int buffer_index;
    void *video_decoder;
    int decoder_serial;

    void (*release_output_buffer)(HarmonyMediaBufferContext *context, bool render);
};

struct FFmpegBufferContext {
    void *opaque;
    void *av_frame;

    void (*release_frame)(FFmpegBufferContext *context);
};

struct VideoToolBufferContext {
    void *buffer = nullptr;
};

enum class BufferType {
    BUFFER_VIDEO_FRAME  = 1,
    BUFFER_AUDIO_FRAME  = 2,
    BUFFER_VIDEO_PACKET = 3,
    BUFFER_AUDIO_PACKET = 4,
    BUFFER_VIDEO_FORMAT = 5
};

struct VideoFrameMetadata {
    int width;
    int height;

    int64_t pts; // ms
    int64_t dts; // ms

    int stride_y;
    int stride_u;
    int stride_v;
    uint8_t *buffer_y = nullptr;
    uint8_t *buffer_u = nullptr;
    uint8_t *buffer_v = nullptr;

    VideoPixelFormat pixel_format;
    void *buffer_context = nullptr;  // Release buffer

};

const int MAX_PLANAR = 8;

struct AudioFrameMetadata {
    int64_t pts; // ms
    int sample_rate;
    int num_samples;
    int num_channels;
    int sample_format;
    AVChannelLayout *channel_layout;

    uint8_t *channel[MAX_PLANAR];
    void *buffer_context = nullptr; // Release buffer
};

enum VideoPacketFormat {
    PKT_FORMAT_AVCC      = 1,
    PKT_FORMAT_ANNEXB    = 2,
    PKT_FORMAT_EXTRADATA = 3
};

struct VideoPacketMetadata {
    int offset = 0;
    int64_t pts; // ms
    int64_t dts; // ms
    uint32_t decode_flags;
    VideoPacketFormat format;
};

struct AudioPacketMetadata {
    int64_t pts; // ms
    int sample_rate;
    int num_channels;
};

struct VideoFormatMetadata {

    ~VideoFormatMetadata() {
        if (hardware_context) {
            delete hardware_context;
            hardware_context = nullptr;
        }
    }

    int width;
    int height;
    bool is_hdr = false;

    HardWareContext *hardware_context = nullptr;
    std::vector<int> items;

    int rotate_degree = 0;

    AVColorSpace colorspace;
    AVColorRange color_range;
    AVColorPrimaries color_primaries;
    AVColorTransferCharacteristic color_trc;

    int sar_num = 0;
    int sar_den = 1;
};

class MixedBuffer {
public:
    MixedBuffer(BufferType type, uint8_t *data, int size, bool own_data);

    MixedBuffer(BufferType type, int capacity);

    ~MixedBuffer();

    int GetSize() const;

    uint8_t *GetData() const;

    BufferType GetType() const;

    uint8_t *ObtainData();

    VideoFrameMetadata *GetVideoFrameMetadata() const;

    AudioFrameMetadata *GetAudioFrameMetadata() const;

    VideoPacketMetadata *GetVideoPacketMetadata() const;

    AudioPacketMetadata *GetAudioPacketMetadata() const;

    VideoFormatMetadata *GetVideoFormatMetadata() const;

    void UpdateBuffer(uint8_t *data, int size, bool ownData);

private:
    void InitType(BufferType type);

    int        mSize;
    uint8_t   *mData;
    bool       bOwnData;
    BufferType mBufferType;

    std::unique_ptr<VideoFrameMetadata>  mVideoFrameMetadata;
    std::unique_ptr<AudioFrameMetadata>  mAudioFrameMetadata;
    std::unique_ptr<VideoPacketMetadata> mVideoPacketMetadata;
    std::unique_ptr<AudioPacketMetadata> mAudioPacketMetadata;
    std::unique_ptr<VideoFormatMetadata> mVideoFormatMetadata;
};

#endif
