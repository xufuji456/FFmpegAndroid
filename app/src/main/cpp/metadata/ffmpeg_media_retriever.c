/*
 * Created by frank on 2022/2/23
 *
 * part of code from William Seemann
 */

#include <ffmpeg_media_retriever.h>
#include <stdio.h>
#include <unistd.h>
#include <android/log.h>
#include <metadata_util.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>

#define TAG "ffmpeg_retriever"
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, TAG, __VA_ARGS__)

const int TARGET_IMAGE_FORMAT = AV_PIX_FMT_RGBA;
const int TARGET_IMAGE_CODEC = AV_CODEC_ID_PNG;


int is_supported_format(int codec_id, int pix_fmt) {
    if ((codec_id == AV_CODEC_ID_PNG ||
         codec_id == AV_CODEC_ID_MJPEG ||
         codec_id == AV_CODEC_ID_BMP) &&
        pix_fmt == AV_PIX_FMT_RGBA) {
        return 1;
    }

    return 0;
}

int get_scaled_context(State *s, AVCodecContext *pCodecCtx, int width, int height) {
    const AVCodec *targetCodec = avcodec_find_encoder(TARGET_IMAGE_CODEC);
    if (!targetCodec) {
        LOGE("avcodec_find_decoder() failed to find encoder\n");
        return FAILURE;
    }

    s->scaled_codecCtx = avcodec_alloc_context3(targetCodec);
    if (!s->scaled_codecCtx) {
        LOGE("avcodec_alloc_context3 failed\n");
        return FAILURE;
    }

    AVCodecParameters *codecP         = s->video_st->codecpar;
    s->scaled_codecCtx->width         = width;
    s->scaled_codecCtx->height        = height;
    s->scaled_codecCtx->pix_fmt       = TARGET_IMAGE_FORMAT;
    s->scaled_codecCtx->codec_type    = AVMEDIA_TYPE_VIDEO;
	s->scaled_codecCtx->bit_rate      = codecP->bit_rate;
    s->scaled_codecCtx->time_base.num = pCodecCtx->time_base.num;
    s->scaled_codecCtx->time_base.den = pCodecCtx->time_base.den;

    if (avcodec_open2(s->scaled_codecCtx, targetCodec, NULL) < 0) {
        LOGE("avcodec_open2() failed\n");
        return FAILURE;
    }

	if (codecP->width > 0 && codecP->height > 0 && codecP->format != AV_PIX_FMT_NONE && width > 0 && height > 0) {
		s->scaled_sws_ctx = sws_getContext(codecP->width,
										   codecP->height,
										   codecP->format,
										   width,
										   height,
										   TARGET_IMAGE_FORMAT,
										   SWS_BILINEAR,
										   NULL,
										   NULL,
										   NULL);
	}

    return SUCCESS;
}

int stream_component_open(State *s, int stream_index) {
	AVFormatContext *pFormatCtx = s->pFormatCtx;
	AVCodecContext *codecCtx;
	const AVCodec *codec;

	if (stream_index < 0 || stream_index >= pFormatCtx->nb_streams) {
		return FAILURE;
	}

	AVCodecParameters *codecPar = pFormatCtx->streams[stream_index]->codecpar;
	codec = avcodec_find_decoder(codecPar->codec_id);
	if (codec == NULL) {
		LOGE("avcodec_find_decoder() failed to find decoder=%d", codecPar->codec_id);
	    return FAILURE;
	}
	codecCtx = avcodec_alloc_context3(codec);
	avcodec_parameters_to_context(codecCtx, codecPar);
    if (avcodec_open2(codecCtx, codec, NULL) < 0) {
		LOGE("avcodec_open2() failed\n");
		return FAILURE;
	}

	switch(codecCtx->codec_type) {
		case AVMEDIA_TYPE_AUDIO:
			s->audio_stream = stream_index;
		    s->audio_st = pFormatCtx->streams[stream_index];
            s->audio_codec = codecCtx;
			break;
		case AVMEDIA_TYPE_VIDEO:
			s->video_stream = stream_index;
		    s->video_st = pFormatCtx->streams[stream_index];
            s->video_codec = codecCtx;
			const AVCodec *targetCodec = avcodec_find_encoder(AV_CODEC_ID_PNG);
			if (!targetCodec) {
			    LOGE("avcodec_find_decoder() failed to find encoder\n");
				return FAILURE;
			}
		    s->codecCtx = avcodec_alloc_context3(targetCodec);
			if (!s->codecCtx) {
				LOGE("avcodec_alloc_context3 failed\n");
				return FAILURE;
			}

			AVCodecParameters *codecP  = s->video_st->codecpar;
			s->codecCtx->width         = codecP->width;
			s->codecCtx->height        = codecP->height;
			s->codecCtx->bit_rate      = codecP->bit_rate;
			s->codecCtx->pix_fmt       = TARGET_IMAGE_FORMAT;
			s->codecCtx->codec_type    = AVMEDIA_TYPE_VIDEO;
            s->codecCtx->time_base.num = s->video_st->avg_frame_rate.den;
            s->codecCtx->time_base.den = s->video_st->avg_frame_rate.num;

			if (avcodec_open2(s->codecCtx, targetCodec, NULL) < 0) {
				LOGE("avcodec_open2() failed\n");
				return FAILURE;
			}

			if (codecP->width > 0 && codecP->height > 0 && codecP->format != AV_PIX_FMT_NONE) {
				s->sws_ctx = sws_getContext(codecP->width,
											codecP->height,
											codecP->format,
											codecP->width,
											codecP->height,
											TARGET_IMAGE_FORMAT,
											SWS_BILINEAR,
											NULL,
											NULL,
											NULL);
			}
			break;
		default:
			break;
	}

	return SUCCESS;
}

int set_data_source_inner(State **state_ptr, const char* path) {
    int i;
	int audio_index = -1;
	int video_index = -1;
	State *state = *state_ptr;

    AVDictionary *options = NULL;
    av_dict_set(&options, "user-agent", "FFmpegMetadataRetriever", 0);

    state->pFormatCtx = avformat_alloc_context();
    if (state->offset > 0) {
        state->pFormatCtx->skip_initial_bytes = state->offset;
    }

    if (avformat_open_input(&state->pFormatCtx, path, NULL, &options) != 0) {
		LOGE("avformat_open_input fail...");
    	return FAILURE;
    }

	if (avformat_find_stream_info(state->pFormatCtx, NULL) < 0) {
		LOGE("avformat_find_stream_info fail...");
    	return FAILURE;
	}

    // Find the first audio and video stream
	for (i = 0; i < state->pFormatCtx->nb_streams; i++) {
		if (state->pFormatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO && video_index < 0) {
			video_index = i;
		}

		if (state->pFormatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO && audio_index < 0) {
			audio_index = i;
		}

		set_codec(state->pFormatCtx, i);
	}

	if (audio_index >= 0) {
		stream_component_open(state, audio_index);
	}

	if (video_index >= 0) {
		stream_component_open(state, video_index);
		state->codecCtx->thread_count = 3; // using multi-thread to decode
		state->codecCtx->thread_type  = FF_THREAD_FRAME; // FF_THREAD_SLICE
	}

	set_duration(state->pFormatCtx);
	set_mimetype(state->pFormatCtx);
    set_file_size(state->pFormatCtx);
    set_frame_rate(state->pFormatCtx, state->video_st);
	set_sample_rate(state->pFormatCtx, state->audio_st);
	set_pixel_format(state->pFormatCtx, state->video_st);
	set_channel_count(state->pFormatCtx, state->audio_st);
	set_channel_layout(state->pFormatCtx, state->audio_st);
	set_video_resolution(state->pFormatCtx, state->video_st);
	set_rotation(state->pFormatCtx, state->audio_st, state->video_st);

	*state_ptr = state;
	return SUCCESS;
}

void init_ffmpeg(State **state_ptr) {
	State *state = *state_ptr;

	if (state && state->pFormatCtx) {
		avformat_close_input(&state->pFormatCtx);
	}
	if (state && state->fd != -1) {
		close(state->fd);
	}
	if (!state) {
		state = av_mallocz(sizeof(State));
	}

	state->pFormatCtx = NULL;
	state->audio_stream = -1;
	state->video_stream = -1;
	state->audio_st = NULL;
	state->video_st = NULL;
	state->fd = -1;
	state->offset = 0;
	state->headers = NULL;

	*state_ptr = state;
}

int set_data_source(State **state_ptr, const char* path) {
	State *state = *state_ptr;
	ANativeWindow *native_window = NULL;

	if (state && state->native_window) {
		native_window = state->native_window;
	}

	init_ffmpeg(&state);
	state->native_window = native_window;
	*state_ptr = state;
	
	return set_data_source_inner(state_ptr, path);
}

int set_data_source_fd(State **state_ptr, int fd, int64_t offset, int64_t length) {
    char path[256] = "";
	State *state = *state_ptr;
	ANativeWindow *native_window = NULL;

	if (state && state->native_window) {
		native_window = state->native_window;
	}
	init_ffmpeg(&state);
	state->native_window = native_window;
	int dummy_fd = dup(fd);
    char str[20];
    sprintf(str, "pipe:%d", dummy_fd);
    strcat(path, str);
    state->fd = dummy_fd;
    state->offset = offset;
	*state_ptr = state;
    
    return set_data_source_inner(state_ptr, path);
}

const char* extract_metadata(State **state_ptr, const char* key) {
    char* value = NULL;
	State *state = *state_ptr;
    
	if (!state || !state->pFormatCtx) {
		return value;
	}

	return extract_metadata_internal(state->pFormatCtx, state->audio_st, state->video_st, key);
}

int init_ffmpeg_filters(State *state, const char *filters_descr, AVFormatContext *fmt_ctx, AVCodecContext *dec_ctx) {
	char args[512];
	int ret = 0;
	const AVFilter *buffersrc  = avfilter_get_by_name("buffer");
	const AVFilter *buffersink = avfilter_get_by_name("buffersink");
	AVFilterInOut *outputs = avfilter_inout_alloc();
	AVFilterInOut *inputs  = avfilter_inout_alloc();
	int i;
	int video_stream_index = 0;
	AVFilterContext *buffersink_ctx;
	AVFilterContext *buffersrc_ctx;
	AVFilterGraph *filter_graph;
	for (i = 0; i < fmt_ctx->nb_streams; i++) {
		if (fmt_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
			video_stream_index = i;
			break;
		}
	}
	AVRational time_base = fmt_ctx->streams[video_stream_index]->time_base;
	enum AVPixelFormat pix_fmts[] = { /*AV_PIX_FMT_YUV420P*/AV_PIX_FMT_RGBA, AV_PIX_FMT_NONE };

	filter_graph = avfilter_graph_alloc();
	if (!outputs || !inputs || !filter_graph) {
		ret = AVERROR(ENOMEM);
		goto end;
	}

	/* buffer video source: the decoded frames from the decoder will be inserted here. */
	snprintf(args, sizeof(args),
			 "video_size=%dx%d:pix_fmt=%d:time_base=%d/%d:pixel_aspect=%d/%d",
			 dec_ctx->width, dec_ctx->height, dec_ctx->pix_fmt,
			 time_base.num, time_base.den,
			 dec_ctx->sample_aspect_ratio.num, dec_ctx->sample_aspect_ratio.den);

	ret = avfilter_graph_create_filter(&buffersrc_ctx, buffersrc, "in",
									   args, NULL, filter_graph);
	if (ret < 0) {
		av_log(NULL, AV_LOG_ERROR, "Cannot create buffer source\n");
		goto end;
	}

	/* buffer video sink: to terminate the filter chain. */
	ret = avfilter_graph_create_filter(&buffersink_ctx, buffersink, "out",
									   NULL, NULL, filter_graph);
	if (ret < 0) {
		av_log(NULL, AV_LOG_ERROR, "Cannot create buffer sink\n");
		goto end;
	}

	ret = av_opt_set_int_list(buffersink_ctx, "pix_fmts", pix_fmts,
							  AV_PIX_FMT_NONE, AV_OPT_SEARCH_CHILDREN);
	if (ret < 0) {
		av_log(NULL, AV_LOG_ERROR, "Cannot set output pixel format\n");
		goto end;
	}

	outputs->name       = av_strdup("in");
	outputs->filter_ctx = buffersrc_ctx;
	outputs->pad_idx    = 0;
	outputs->next       = NULL;

	inputs->name        = av_strdup("out");
	inputs->filter_ctx  = buffersink_ctx;
	inputs->pad_idx     = 0;
	inputs->next        = NULL;

	if ((ret = avfilter_graph_parse_ptr(filter_graph, filters_descr,
										&inputs, &outputs, NULL)) < 0)
		goto end;

	if ((ret = avfilter_graph_config(filter_graph, NULL)) < 0)
		goto end;

	state->buffersrc_ctx  = buffersrc_ctx;
	state->buffersink_ctx = buffersink_ctx;
	state->filter_graph   = filter_graph;

end:
	avfilter_inout_free(&inputs);
	avfilter_inout_free(&outputs);

	return ret;
}

void convert_image(State *state, AVCodecContext *pCodecCtx, AVFrame *pFrame, AVPacket *avpkt,
		int *got_packet_ptr, int width, int height) {

	AVFrame *frame;
	*got_packet_ptr = 0;
    int rotateDegree = 0;
	AVCodecContext *codecCtx;
	struct SwsContext *scaleCtx;

	if (width != -1 && height != -1) {
		if (state->scaled_codecCtx == NULL ||
			state->scaled_sws_ctx == NULL) {
			get_scaled_context(state, pCodecCtx, width, height);
		}

		codecCtx = state->scaled_codecCtx;
		scaleCtx = state->scaled_sws_ctx;
	} else {
		codecCtx = state->codecCtx;
		scaleCtx = state->sws_ctx;
	}

	if (!scaleCtx) {
		LOGE("scale context is null!");
		return;
	}
	if (width == -1) {
		width = pCodecCtx->width;
	}
	if (height == -1) {
		height = pCodecCtx->height;
	}

	frame = av_frame_alloc();

	// Determine required buffer size and allocate buffer
	int numBytes  = av_image_get_buffer_size(TARGET_IMAGE_FORMAT, codecCtx->width, codecCtx->height, 1);
	void * buffer = (uint8_t *) av_malloc(numBytes * sizeof(uint8_t));

	frame->format = TARGET_IMAGE_FORMAT;
	frame->width  = codecCtx->width;
	frame->height = codecCtx->height;

	av_image_fill_arrays(frame->data,
				         frame->linesize,
				         buffer,
				         TARGET_IMAGE_FORMAT,
				         codecCtx->width,
				         codecCtx->height,
				   1);

	sws_scale(scaleCtx,
			  (const uint8_t * const *) pFrame->data,
              pFrame->linesize,
			  0,
			  pFrame->height,
			  frame->data,
			  frame->linesize);

    if (state->video_st) {
        AVDictionaryEntry *entry = av_dict_get(state->video_st->metadata, ROTATE, NULL, AV_DICT_MATCH_CASE);
		if (entry && entry->value) {
            rotateDegree = atoi(entry->value);
		}
    }
    if (rotateDegree == 90 || rotateDegree == 270) {
        if (!state->buffersrc_ctx || !state->buffersink_ctx || !state->filter_graph) {
            const char* filter_str = "transpose=clock";
            if (rotateDegree == 270) {
                filter_str = "transpose=cclock";
            }
		    init_ffmpeg_filters(state, filter_str, state->pFormatCtx, codecCtx);
        }
        
        if (state->buffersrc_ctx && state->buffersink_ctx && state->filter_graph) {
            int filter_ret = av_buffersrc_add_frame_flags(state->buffersrc_ctx, frame, AV_BUFFERSRC_FLAG_KEEP_REF);
            if (filter_ret >= 0) {
                AVFrame *dst_frame = av_frame_alloc();
                filter_ret = av_buffersink_get_frame(state->buffersink_ctx, dst_frame);
                if (filter_ret >= 0) {
                    codecCtx->width = dst_frame->width;
                    codecCtx->height = dst_frame->height;
                    av_frame_free(&frame);
                    frame = dst_frame;
                }
            }
        }
    }

    avcodec_send_frame(codecCtx, frame);
    int ret = avcodec_receive_packet(codecCtx, avpkt);
    *got_packet_ptr = (ret == 0 ? 1 : 0);

    if (rotateDegree == 90 || rotateDegree == 270) {
        codecCtx->width = frame->height;
        codecCtx->height = frame->width;
    }

	if (ret >= 0 && state->native_window) {
		ANativeWindow_setBuffersGeometry(state->native_window, width, height, WINDOW_FORMAT_RGBA_8888);
		ANativeWindow_Buffer windowBuffer;

		if (ANativeWindow_lock(state->native_window, &windowBuffer, NULL) == 0) {
			for (int h = 0; h < height; h++)  {
				memcpy(windowBuffer.bits + h * windowBuffer.stride * 4,
					   buffer + h * frame->linesize[0],
					   width*4);
			}

			ANativeWindow_unlockAndPost(state->native_window);
		}
	}

	av_frame_free(&frame);
	if (buffer) {
		free(buffer);
	}
	if (ret < 0 || !*got_packet_ptr) {
		av_packet_unref(avpkt);
	}
}

int get_audio_thumbnail(State **state_ptr, AVPacket *pkt) {
	int i = 0;
	int got_packet = 0;
	AVFrame *frame = NULL;
	State *state = *state_ptr;

	if (!state || !state->pFormatCtx) {
		return FAILURE;
	}

	// find the first attached picture, if available
	for (i = 0; i < state->pFormatCtx->nb_streams; i++) {
		if (state->pFormatCtx->streams[i]->disposition & AV_DISPOSITION_ATTACHED_PIC) {
			if (pkt) {
				av_packet_unref(pkt);
				av_init_packet(pkt);
			}
			av_packet_ref(pkt, &state->pFormatCtx->streams[i]->attached_pic);
			got_packet = 1;

			if (pkt->stream_index == state->video_stream) {
				int codec_id = state->video_st->codecpar->codec_id;
				int pix_fmt  = state->video_st->codecpar->format;

				if (!is_supported_format(codec_id, pix_fmt)) {
					int got_frame = 0;

					frame = av_frame_alloc();

					if (!frame) {
						break;
					}

					AVCodecContext *pCodecContext = state->video_codec;
                    avcodec_send_packet(pCodecContext, pkt);
                    int ret = avcodec_receive_frame(pCodecContext, frame);
					if (ret == 0) {
                        got_frame = 1;
					} else {
                        break;
					}

					if (got_frame) {
						AVPacket convertedPkt;
						av_init_packet(&convertedPkt);
						convertedPkt.size = 0;
						convertedPkt.data = NULL;

						convert_image(state, pCodecContext, frame, &convertedPkt, &got_packet, -1, -1);

						av_packet_unref(pkt);
						av_init_packet(pkt);
						av_packet_ref(pkt, &convertedPkt);
						av_packet_unref(&convertedPkt);

						break;
					}
				} else {
					av_packet_unref(pkt);
					av_init_packet(pkt);
					av_packet_ref(pkt, &state->pFormatCtx->streams[i]->attached_pic);

					got_packet = 1;
					break;
				}
			}
		}
	}

	av_frame_free(&frame);
	if (got_packet) {
		return SUCCESS;
	} else {
		return FAILURE;
	}
}

void decode_frame(State *state, AVPacket *pkt, int *got_frame, int64_t desired_frame_number, int width, int height) {
	AVFrame *frame = av_frame_alloc();
	if (!frame) {
		return;
	}

	while (av_read_frame(state->pFormatCtx, pkt) >= 0) {

		if (pkt->stream_index == state->video_stream) {
			int codec_id = state->video_st->codecpar->codec_id;
			int pix_fmt  = state->video_st->codecpar->format;

			if (!is_supported_format(codec_id, pix_fmt)) {
				*got_frame = 0;

				AVCodecContext *pCodecContext = state->video_codec;
				avcodec_send_packet(pCodecContext, pkt);
                int ret = avcodec_receive_frame(pCodecContext, frame);
				if (ret == 0) {
					*got_frame = 1;
				} else if (ret == AVERROR(EAGAIN)) {
					continue;
				} else {
					LOGE("decode frame fail...");
                    break;
				}

                if (*got_frame) {
                    if (desired_frame_number == -1 || frame->pts >= desired_frame_number != 0) {
                        if (pkt->data) {
                            av_packet_unref(pkt);
                        }
                        av_init_packet(pkt);
                        convert_image(state, pCodecContext, frame, pkt, got_frame, width, height);
                        break;
                    }
                }
			} else {
				*got_frame = 1;
				break;
			}
		}
	}

	av_frame_free(&frame);
}

int get_frame_at_time(State **state_ptr, int64_t timeUs, int option, AVPacket *pkt) {
	return get_scaled_frame_at_time(state_ptr, timeUs, option, pkt, -1, -1);
}

int get_scaled_frame_at_time(State **state_ptr, int64_t timeUs, int option, AVPacket *pkt, int width, int height) {
	int flags = 0;
	int ret = -1;
	int got_packet = 0;
	State *state = *state_ptr;
	Options opt = option;
	int64_t desired_frame_number = -1;

	if (!state || !state->pFormatCtx || state->video_stream < 0) {
		return FAILURE;
	}

	if (timeUs > -1) {
		int stream_index = state->video_stream;
		int64_t seek_time = av_rescale_q(timeUs, AV_TIME_BASE_Q, state->pFormatCtx->streams[stream_index]->time_base);
		int64_t seek_stream_duration = state->pFormatCtx->streams[stream_index]->duration;

		if (seek_stream_duration > 0 && seek_time > seek_stream_duration) {
			seek_time = seek_stream_duration;
		}
		if (seek_time < 0) {
			return FAILURE;
		}

		if (opt == OPTION_CLOSEST) {
			desired_frame_number = seek_time;
			flags = AVSEEK_FLAG_BACKWARD;
		} else if (opt == OPTION_CLOSEST_SYNC) {
			flags = 0;
		} else if (opt == OPTION_NEXT_SYNC) {
			flags = 0;
		} else if (opt == OPTION_PREVIOUS_SYNC) {
			flags = AVSEEK_FLAG_BACKWARD;
		}

		ret = av_seek_frame(state->pFormatCtx, stream_index, seek_time, flags);

		if (ret < 0) {
			return FAILURE;
		} else {
			if (state->audio_stream >= 0) {
				avcodec_flush_buffers(state->audio_codec);
			}

			if (state->video_stream >= 0) {
				avcodec_flush_buffers(state->video_codec);
			}
		}
	}

	decode_frame(state, pkt, &got_packet, desired_frame_number, width, height);

	if (got_packet) {
		return SUCCESS;
	} else {
		return FAILURE;
	}
}

int set_native_window(State **state_ptr, ANativeWindow* native_window) {

	State *state = *state_ptr;

	if (native_window == NULL) {
		return FAILURE;
	}
	if (!state) {
		init_ffmpeg(&state);
	}

	state->native_window = native_window;
	*state_ptr = state;

	return SUCCESS;
}

void release_retriever(State **state_ptr) {

	State *state = *state_ptr;
	
    if (state) {
        if (state->audio_st && state->audio_codec) {
            avcodec_close(state->audio_codec);
        }
        
        if (state->video_st && state->video_codec) {
            avcodec_close(state->video_codec);
        }
        
        if (state->pFormatCtx) {
    		avformat_close_input(&state->pFormatCtx);
    	}
    	
    	if (state->fd != -1) {
    		close(state->fd);
    	}

		if (state->sws_ctx) {
			sws_freeContext(state->sws_ctx);
			state->sws_ctx = NULL;
		}

    	if (state->codecCtx) {
    		avcodec_close(state->codecCtx);
    	    av_free(state->codecCtx);
    	}

    	if (state->scaled_codecCtx) {
    		avcodec_close(state->scaled_codecCtx);
    	    av_free(state->scaled_codecCtx);
    	}

		if (state->scaled_sws_ctx) {
			sws_freeContext(state->scaled_sws_ctx);
		}

		if (state->native_window != NULL) {
			ANativeWindow_release(state->native_window);
			state->native_window = NULL;
		}

		if (state->buffersrc_ctx) {
			avfilter_free(state->buffersrc_ctx);
		}
		if (state->buffersink_ctx) {
			avfilter_free(state->buffersink_ctx);
		}
		if (state->filter_graph) {
			avfilter_graph_free(&state->filter_graph);
		}

    	av_freep(&state);
    }
}
