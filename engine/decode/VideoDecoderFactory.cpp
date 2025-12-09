/**
 * Note: factory of video decoder
 * Date: 2025/12/9
 * Author: frank
 */

#include "VideoDecoderFactory.h"

#include "decode/FFmpegVideoDecoder.h"

//#if defined(__ANDROID__)
//#include "decode/android/MediacodecDecoder.h"
//#endif
//#if defined(__APPLE__)
//#include "decode/ios/VideoToolBoxDecoder.h"
//#endif

VideoDecoderFactory::VideoDecoderFactory() = default;

VideoDecoderFactory::~VideoDecoderFactory() = default;

std::unique_ptr<VideoDecoder>
VideoDecoderFactory::CreateVideoDecoder(VideoCodecType codecType, int codecId) {
    std::unique_ptr<VideoDecoder> decoder;

    switch (codecType) {
        case VideoCodecType::DECODE_TYPE_SOFTWARE:
            decoder = std::make_unique<FFmpegVideoDecoder>(codecId);
            break;
//#if defined(__ANDROID__)
//        case VideoCodecType::DECODE_TYPE_ANDROID:
//            decoder = std::make_unique<MediaCodecVideoDecoder>(codecId);
//            break;
//#endif
//#if defined(__APPLE__)
//            case VideoCodecType::DECODE_TYPE_IOS:
//        decoder = std::make_unique<VideoToolBoxDecoder>(codecId);
//        break;
//#endif
        default:
            break;
    }
    return decoder;
}
