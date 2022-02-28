/*
 * Created by frank on 2022/2/23
 *
 * part of code from William Seemann
 */

#include "metadata_util.h"
#include <stdio.h>
#include "include/libavutil/pixdesc.h"

void set_duration(AVFormatContext *ic) {
	char value[20] = "0";
	long duration = 0;

	if (ic) {
		if (ic->duration != AV_NOPTS_VALUE) {
			duration = ((ic->duration / AV_TIME_BASE) * 1000);
		}
	}

	sprintf(value, "%ld", duration);
	av_dict_set(&ic->metadata, DURATION, value, 0);
}

void set_file_size(AVFormatContext *ic) {
	char value[20] = "0";

	int64_t size = ic->pb ? avio_size(ic->pb) : -1;
	sprintf(value, "%"PRId64, size);
	av_dict_set(&ic->metadata, FILE_SIZE, value, 0);
}

void set_mimetype(AVFormatContext *ic) {
	if (ic->iformat == NULL || ic->iformat->name == NULL){
		return;
	}
	const char *name = ic->iformat->name;
	av_dict_set(&ic->metadata, MIME_TYPE, name, 0);
}

void set_codec(AVFormatContext *ic, int i) {
    const char *codec_type = av_get_media_type_string(ic->streams[i]->codecpar->codec_type);
	if (!codec_type) {
		return;
	}

    const char *codec_name = avcodec_get_name(ic->streams[i]->codecpar->codec_id);

	if (strcmp(codec_type, "audio") == 0) {
		av_dict_set(&ic->metadata, AUDIO_CODEC, codec_name, 0);
    } else if (strcmp(codec_type, "video") == 0) {
	   	av_dict_set(&ic->metadata, VIDEO_CODEC, codec_name, 0);
	}
}

void set_sample_rate(AVFormatContext *ic, AVStream *stream) {
	char value[10] = "0";
	if (stream) {
		sprintf(value, "%d", stream->codecpar->sample_rate);
		av_dict_set(&ic->metadata, SAMPLE_RATE, value, 0);
	}
}

void set_channel_count(AVFormatContext *ic, AVStream *stream) {
	char value[10] = "0";
	if (stream) {
		sprintf(value, "%d", stream->codecpar->channels);
		av_dict_set(&ic->metadata, CHANNEL_COUNT, value, 0);
	}
}

void set_channel_layout(AVFormatContext *ic, AVStream *stream) {
	char value[20] = "0";
	if (stream) {
		av_get_channel_layout_string(value, 20,
				stream->codecpar->channels, stream->codecpar->channel_layout);
		av_dict_set(&ic->metadata, CHANNEL_LAYOUT, value, 0);
	}
}

void set_pixel_format(AVFormatContext *ic, AVStream *stream) {
    if (stream) {
        const char *name = av_get_pix_fmt_name(stream->codecpar->format);
        av_dict_set(&ic->metadata, PIXEL_FORMAT, name, 0);
    }
}

void set_video_resolution(AVFormatContext *ic, AVStream *video_st) {
	char value[20] = "0";
	if (video_st) {
		sprintf(value, "%d", video_st->codecpar->width);
	    av_dict_set(&ic->metadata, VIDEO_WIDTH, value, 0);

		sprintf(value, "%d", video_st->codecpar->height);
	    av_dict_set(&ic->metadata, VIDEO_HEIGHT, value, 0);
	}
}

void set_rotation(AVFormatContext *ic, AVStream *audio_st, AVStream *video_st) {
	if (!extract_metadata_internal(ic, audio_st, video_st, ROTATE) && video_st && video_st->metadata) {
		AVDictionaryEntry *entry = av_dict_get(video_st->metadata, ROTATE, NULL, AV_DICT_MATCH_CASE);

		if (entry && entry->value) {
			av_dict_set(&ic->metadata, ROTATE, entry->value, 0);
		} else {
			av_dict_set(&ic->metadata, ROTATE, "0", 0);
		}
	}
}

void set_frame_rate(AVFormatContext *ic, AVStream *video_st) {
	char value[20] = "0";

	if (video_st && video_st->avg_frame_rate.den && video_st->avg_frame_rate.num) {
		double d = av_q2d(video_st->avg_frame_rate);
		uint64_t v = lrintf((float)d * 100);
		if (v % 100) {
			sprintf(value, "%3.2f", d);
		} else if (v % (100 * 1000)) {
			sprintf(value,  "%1.0f", d);
		} else {
			sprintf(value, "%1.0fk", d / 1000);
		}

		av_dict_set(&ic->metadata, FRAME_RATE, value, 0);
	}
}

const char* extract_metadata_internal(AVFormatContext *ic, AVStream *audio_st, AVStream *video_st, const char* key) {
    char* value = NULL;
    
	if (!ic) {
		return value;
	}
    
	if (key) {
		if (av_dict_get(ic->metadata, key, NULL, AV_DICT_MATCH_CASE)) {
			value = av_dict_get(ic->metadata, key, NULL, AV_DICT_MATCH_CASE)->value;
		} else if (audio_st && av_dict_get(audio_st->metadata, key, NULL, AV_DICT_MATCH_CASE)) {
			value = av_dict_get(audio_st->metadata, key, NULL, AV_DICT_MATCH_CASE)->value;
		} else if (video_st && av_dict_get(video_st->metadata, key, NULL, AV_DICT_MATCH_CASE)) {
			value = av_dict_get(video_st->metadata, key, NULL, AV_DICT_MATCH_CASE)->value;
		}
	}
	
	return value;	
}