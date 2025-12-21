/**
 * Note: interface of video decoder
 * Date: 2025/12/21
 * Author: frank
 */

#include "decode/VideoDecoder.h"

VideoDecoder::VideoDecoder(int codecId)
            : mCodecId(codecId) {}

VideoDecoder::~VideoDecoder() = default;

void VideoDecoder::SetDecodeCallback(VideoDecodeCallback *callback) {
    mVideoDecodeCallback = callback;
}
