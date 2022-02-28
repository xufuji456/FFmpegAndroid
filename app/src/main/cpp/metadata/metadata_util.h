/*
 * Created by frank on 2022/2/23
 *
 * part of code from William Seemann
 */

#ifndef METADATA_UTIL_H_
#define METADATA_UTIL_H_

#include "libavutil/opt.h"
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"

static const char *DURATION       = "duration";
static const char *AUDIO_CODEC    = "audio_codec";
static const char *VIDEO_CODEC    = "video_codec";
static const char *ROTATE         = "rotate";
static const char *FRAME_RATE     = "frame_rate";
static const char *FILE_SIZE      = "file_size";
static const char *VIDEO_WIDTH    = "video_width";
static const char *VIDEO_HEIGHT   = "video_height";
static const char *MIME_TYPE      = "mime_type";
static const char *SAMPLE_RATE    = "sample_rate";
static const char *CHANNEL_COUNT  = "channel_count";
static const char *CHANNEL_LAYOUT = "channel_layout";
static const char *PIXEL_FORMAT   = "pixel_format";

static const int SUCCESS = 0;
static const int FAILURE = -1;

void set_duration(AVFormatContext *ic);
void set_file_size(AVFormatContext *ic);
void set_mimetype(AVFormatContext *ic);
void set_codec(AVFormatContext *ic, int i);
void set_sample_rate(AVFormatContext *ic,    AVStream *stream);
void set_channel_count(AVFormatContext *ic,  AVStream *stream);
void set_channel_layout(AVFormatContext *ic, AVStream *stream);
void set_pixel_format(AVFormatContext *ic, AVStream *stream);
void set_video_resolution(AVFormatContext *ic, AVStream *video_st);
void set_rotation(AVFormatContext *ic, AVStream *audio_st, AVStream *video_st);
void set_frame_rate(AVFormatContext *ic, AVStream *video_st);
const char* extract_metadata_internal(AVFormatContext *ic, AVStream *audio_st, AVStream *video_st, const char* key);

#endif /*METADATA_UTIL_H_*/
