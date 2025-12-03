#ifndef FFMPEG_UTIL_H
#define FFMPEG_UTIL_H

#ifdef __cplusplus
extern "C" {
#endif
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libavutil/avstring.h"
#include "libavutil/dict.h"
#include "libavutil/display.h"
#include "libavutil/eval.h"
#include "libavutil/opt.h"
#ifdef __cplusplus
}
#endif

int64_t GetBitrate(AVCodecParameters *codecpar);

double GetRotation(AVStream *st);

AVDictionary **FindStreamInfoOpts(AVFormatContext *s,
                                  AVDictionary *codec_opts);

AVDictionary *FilterCodecOpts(AVDictionary *opts, enum AVCodecID codec_id,
                              AVFormatContext *s, AVStream *st, AVCodec *codec);
#endif
