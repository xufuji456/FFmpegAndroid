/*
 * Created by frank on 2022/2/23
 *
 * part of code from William Seemann
 */

#ifndef FFMPEG_MEDIA_RETRIEVER_H_
#define FFMPEG_MEDIA_RETRIEVER_H_

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/dict.h>

#include <android/native_window_jni.h>
#include <libavfilter/avfilter.h>
#include <libavfilter/buffersrc.h>
#include <libavfilter/buffersink.h>

typedef enum {
	OPTION_PREVIOUS_SYNC = 0,
	OPTION_NEXT_SYNC = 1,
	OPTION_CLOSEST_SYNC = 2,
	OPTION_CLOSEST = 3,
} Options;

typedef struct State {
	AVFormatContext *pFormatCtx;
	int             audio_stream;
	int             video_stream;
	AVStream        *audio_st;
	AVStream        *video_st;
	int             fd;
	int64_t         offset;
	const char      *headers;
	AVCodecContext  *codecCtx;
	AVCodecContext  *scaled_codecCtx;
	ANativeWindow   *native_window;

	AVFilterContext *buffersink_ctx;
	AVFilterContext *buffersrc_ctx;
	AVFilterGraph   *filter_graph;

	struct SwsContext *sws_ctx;
	struct SwsContext *scaled_sws_ctx;
} State;

struct AVDictionary {
	int count;
	AVDictionaryEntry *elems;
};

int set_data_source(State **ps, const char* path);
int set_data_source_fd(State **ps, int fd, int64_t offset, int64_t length);
const char* extract_metadata(State **ps, const char* key);
int get_frame_at_time(State **ps, int64_t timeUs, int option, AVPacket *pkt);
int get_scaled_frame_at_time(State **ps, int64_t timeUs, int option, AVPacket *pkt, int width, int height);
int get_audio_thumbnail(State **state_ptr, AVPacket *pkt);
int set_native_window(State **ps, ANativeWindow* native_window);
void release(State **ps);

#endif /*FFMPEG_MEDIA_RETRIEVER_H_*/
