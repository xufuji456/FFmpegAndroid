/**
 * Note: interface of extractor
 * Date: 2025/12/3
 * Author: frank
 */

#ifndef EXTRACTOR_INTERFACE_H
#define EXTRACTOR_INTERFACE_H

#include <algorithm>
#include <memory>
#include <string>

#include "NextDefine.h"
#include "NextStructDefine.h"

#ifdef __cplusplus
extern "C" {
#endif
#include "libavcodec/avcodec.h"
#include "libavutil/dict.h"
#ifdef __cplusplus
}
#endif

struct FFmpegOption {
    AVDictionary *format_opts;
    AVDictionary *codec_opts;
};

class ExtractorInterface {
public:
    virtual ~ExtractorInterface() = default;

    virtual int Open(const std::string &url, FFmpegOption &opt,
                     std::shared_ptr<MetaData> &metadata) = 0;

    virtual int ReadPacket(AVPacket *pkt) = 0;

    virtual int Seek(int64_t timestamp, int64_t rel = 0, int seekFlags = 0) = 0;

    virtual int GetError() = 0;

    virtual void SetInterrupt() = 0;

    virtual int GetStreamType(int streamIndex) = 0;

    virtual void Close() = 0;
};

#endif
