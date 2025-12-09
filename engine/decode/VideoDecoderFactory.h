#ifndef RS_VIDEO_DECODER_FACTORY_H
#define RS_VIDEO_DECODER_FACTORY_H

#include <memory>
#include <vector>

#include "decode/common/VideoCodecInfo.h"
#include "decode/VideoDecoder.h"

class VideoDecoderFactory {
public:
  VideoDecoderFactory();
  ~VideoDecoderFactory();

  static std::unique_ptr<VideoDecoder> CreateVideoDecoder(VideoCodecType codecType, int codecId);

};

#endif