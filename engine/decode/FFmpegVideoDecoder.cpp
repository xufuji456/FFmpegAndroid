/**
 * Note: video decoder with FFmpeg
 * Date: 2025/12/8
 * Author: frank
 */

#include "decode/FFmpegVideoDecoder.h"

#include "NextLog.h"

#define FFMPEG_VIDEO_TAG "FFmpegVideoDec"

static void ReleaseFrame(FFmpegBufferContext *context) {
    if (context && context->av_frame) {
        auto *frame = reinterpret_cast<AVFrame *>(context->av_frame);
        av_frame_unref(frame);
        av_frame_free(&frame);
    }
}

FFmpegVideoDecoder::FFmpegVideoDecoder(int codecId)
        : VideoDecoder(codecId) {}

FFmpegVideoDecoder::~FFmpegVideoDecoder() {
    NEXT_LOGD(FFMPEG_VIDEO_TAG, "~FFmpegVideoDecoder destructor");
}

int FFmpegVideoDecoder::Init(const MetaData *metadata) {
    mCodecContext = avcodec_alloc_context3(nullptr);

    mCodecContext->codec_type   = AVMEDIA_TYPE_VIDEO;
    mCodecContext->thread_count = 0; // auto
    mCodecContext->thread_type  = FF_THREAD_FRAME;
    mCodecContext->time_base    = {1, 1000};

    mCodecContext->codec_id = (AVCodecID) mCodecId;

    auto *codec = const_cast<AVCodec *>(avcodec_find_decoder(mCodecContext->codec_id));
    if (!codec) {
        NEXT_LOGE(FFMPEG_VIDEO_TAG, "avcodec_find_decoder fail, name=%s",
                avcodec_get_name(mCodecContext->codec_id));
        Release();
        return ERROR_DECODE_VIDEO_OPEN;
    }

    AVDictionary *opts = nullptr;
    if (avcodec_open2(mCodecContext, codec, &opts) < 0) {
        NEXT_LOGE(FFMPEG_VIDEO_TAG, "avcodec_open2 fail, name=%s, w=%d, h=%d",
                avcodec_get_name(mCodecContext->codec_id),
                mCodecContext->width, mCodecContext->height);
        Release();
        av_dict_free(&opts);
        return ERROR_DECODE_VIDEO_OPEN;
    }
    av_dict_free(&opts);

    return RESULT_OK;
}

int FFmpegVideoDecoder::Decode(const AVPacket *pkt) {
    if (!mCodecContext || !mVideoDecodeCallback) {
        return ERROR_DECODE_NOT_INIT;
    }

    AVFrame *frame = av_frame_alloc();
    int ret = avcodec_receive_frame(mCodecContext, frame);

    if (ret >= 0) {
        size_t bufferSize = 1;
        std::unique_ptr<MixedBuffer> output_buffer =
                std::make_unique<MixedBuffer>(BufferType::BUFFER_VIDEO_FRAME, bufferSize);

        VideoFrameMetadata *meta = output_buffer->GetVideoFrameMetadata();
        meta->width    = frame->width;
        meta->height   = frame->height;
        meta->stride_y = frame->linesize[0];
        meta->stride_u = frame->linesize[1];
        meta->stride_v = frame->linesize[2];

        meta->buffer_y = frame->data[0];
        meta->buffer_u = frame->data[1];
        meta->buffer_v = frame->data[2];

        meta->buffer_context = reinterpret_cast<void *>(new FFmpegBufferContext{
                .av_frame = frame,
                .release_frame = ReleaseFrame,
        });

        switch (frame->format) {
            case AV_PIX_FMT_YUVJ420P:
                meta->pixel_format = VideoPixelFormat::PIXEL_FORMAT_YUVJ420P;
                break;
            case AV_PIX_FMT_YUV420P10LE:
                meta->pixel_format = VideoPixelFormat::PIXEL_FORMAT_YUV420P10LE;
                break;
            case AV_PIX_FMT_YUV420P:
                meta->pixel_format = VideoPixelFormat::PIXEL_FORMAT_YUV420P;
                break;
            default:
                meta->pixel_format = VideoPixelFormat::PIXEL_FORMAT_YUV420P;
                NEXT_LOGW(FFMPEG_VIDEO_TAG, "unsupported pixel format %d", frame->format);
                break;
        }

        meta->pts = frame->best_effort_timestamp;
        meta->dts = frame->pkt_dts;

        mVideoDecodeCallback->OnDecodedFrame(std::move(output_buffer));
    } else if (ret == AVERROR_EOF) {
        av_frame_unref(frame);
        av_frame_free(&frame);
        return ERROR_PLAYER_EOF;
    } else {
        av_frame_unref(frame);
        av_frame_free(&frame);
    }

    AVPacket packet;
    av_init_packet(&packet);
    packet.data = pkt->data;
    packet.size = pkt->size;
    packet.pts  = pkt->pts;
    packet.dts  = pkt->dts;

    if (!bFlushState) {
        if (packet.size == 0) {
            bFlushState = true;
        }
        ret = avcodec_send_packet(mCodecContext, bFlushState ? nullptr : &packet);
        if (ret == AVERROR(EAGAIN)) {
            return ERROR_PLAYER_TRY_AGAIN;
        }
    }

    return RESULT_OK;
}

int FFmpegVideoDecoder::SetVideoFormat(const MetaData *metadata) {
    if (!metadata || metadata->video_index < 0) {
        NEXT_LOGE(FFMPEG_VIDEO_TAG, "metadata is invalid");
        return ERROR_DECODE_INVALID;
    }

    if (mCodecContext == nullptr) {
        NEXT_LOGE(FFMPEG_VIDEO_TAG, "alloc codec_context failed...");
        return ERROR_DECODE_NOT_INIT;
    }

    auto trackInfo = metadata->track_info[metadata->video_index];

    if (trackInfo.extra_data && trackInfo.extra_data_size > 0) {
        mCodecContext->extradata = reinterpret_cast<uint8_t *>(
                av_malloc(trackInfo.extra_data_size + AV_INPUT_BUFFER_PADDING_SIZE));
        mCodecContext->extradata_size = trackInfo.extra_data_size;
        memcpy(mCodecContext->extradata, trackInfo.extra_data, trackInfo.extra_data_size);
    }

    auto *codec = const_cast<AVCodec *>(avcodec_find_decoder(mCodecContext->codec_id));
    if (!codec) {
        Release();
        return ERROR_DECODE_VIDEO_OPEN;
    }

    int ret = 0;
    if ((ret = avcodec_close(mCodecContext)) < 0) {
        Release();
        return ERROR_DECODE_VIDEO_OPEN;
    }

    AVDictionary *opts = nullptr;
    av_dict_set(&opts, "threads", "auto", 0);
    av_dict_set(&opts, "refcounted_frames", "1", 0);

    if ((ret = avcodec_open2(mCodecContext, codec, &opts)) < 0) {
        av_dict_free(&opts);
        Release();
        return ERROR_DECODE_VIDEO_OPEN;
    }
    av_dict_free(&opts);

    return RESULT_OK;
}

int FFmpegVideoDecoder::Flush() {
    bFlushState = false;
    if (mCodecContext) {
        avcodec_flush_buffers(mCodecContext);
    }
    return RESULT_OK;
}

int FFmpegVideoDecoder::Release() {
    NEXT_LOGI(FFMPEG_VIDEO_TAG, "Release...");
    if (mCodecContext) {
        avcodec_free_context(&mCodecContext);
        mCodecContext = nullptr;
    }
    return RESULT_OK;
}

void FFmpegVideoDecoder::SetDecodeCallback(VideoDecodeCallback *callback) {
    mVideoDecodeCallback = callback;
}
