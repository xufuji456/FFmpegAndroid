/**
 * Note: audio decoder with FFmpeg
 * Date: 2025/12/7
 * Author: frank
 */

#include "FFmpegAudioDecoder.h"
#include "NextErrorCode.h"
#include "NextLog.h"

#define FFMPEG_AUDIO_TAG "FFmpegAudioDec"

FFmpegAudioDecoder::FFmpegAudioDecoder() {}

FFmpegAudioDecoder::~FFmpegAudioDecoder() {
    NEXT_LOGD(FFMPEG_AUDIO_TAG, "~FFmpegAudioDecoder destructor");
}

int FFmpegAudioDecoder::Init(AudioCodecConfig &config) {
    int ret = RESULT_OK;
    if (!config.channels || !config.sample_rate) {
        return ERROR_DECODE_INVALID;
    }
    mCodecContext = avcodec_alloc_context3(nullptr);

    mCodecContext->profile      = config.profile;
    mCodecContext->codec_id     = (AVCodecID) config.codec_id;
    mCodecContext->time_base    = {1, 1000}; // TODO: timebase
    mCodecContext->codec_type   = AVMEDIA_TYPE_AUDIO;
    mCodecContext->sample_rate  = config.sample_rate;
    mCodecContext->thread_count = 1;
    mCodecContext->ch_layout.nb_channels = config.channels;

    if (config.extradata_size > 0) {
        mCodecContext->extradata = reinterpret_cast<uint8_t *>(
                av_malloc(config.extradata_size + AV_INPUT_BUFFER_PADDING_SIZE));
        mCodecContext->extradata_size = config.extradata_size;
        memcpy(mCodecContext->extradata, config.extradata, config.extradata_size);
    }

    auto *codec = const_cast<AVCodec *>(avcodec_find_decoder(mCodecContext->codec_id));
    if (!codec) {
        NEXT_LOGE(FFMPEG_AUDIO_TAG, "avcodec_find_decoder fail, name=%s",
                avcodec_get_name(mCodecContext->codec_id));
        Release();
        return ERROR_DECODE_AUDIO_OPEN;
    }

    AVDictionary *opts = nullptr;
    av_dict_set(&opts, "refcounted_frames", "1", 0);
    if ((ret = avcodec_open2(mCodecContext, codec, nullptr)) < 0) {
        NEXT_LOGE(FFMPEG_AUDIO_TAG, "avcodec_open2 fail, msg=%s", av_err2str(ret));
        av_dict_free(&opts);
        Release();
        return ERROR_DECODE_AUDIO_OPEN;
    }
    av_dict_free(&opts);

    return RESULT_OK;
}

int FFmpegAudioDecoder::Decode(const AVPacket *pkt) {
    if (!pkt || !mCodecContext || !mAudioDecodedCallback) {
        return ERROR_DECODE_NOT_INIT;
    }

    int ret = avcodec_send_packet(mCodecContext, pkt);
    if (ret < 0) {
        return ret;
    }

    AVFrame *frame = av_frame_alloc();
    ret = avcodec_receive_frame(mCodecContext, frame);
    if (ret < 0) {
        av_frame_free(&frame);
        switch (ret) {
            case AVERROR(EAGAIN):
                return ERROR_PLAYER_TRY_AGAIN;
            case AVERROR_EOF:
                NEXT_LOGI(FFMPEG_AUDIO_TAG, "avcodec_receive_frame EOF...");
                return ERROR_PLAYER_EOF;
            default:
                return ret;
        }
    }

    mAudioDecodedCallback->OnDecodedFrame(frame);

    return RESULT_OK;
}

int FFmpegAudioDecoder::Flush() {
    if (mCodecContext) {
        avcodec_flush_buffers(mCodecContext);
    }
    return RESULT_OK;
}

int FFmpegAudioDecoder::Release() {
    NEXT_LOGI(FFMPEG_AUDIO_TAG, "Release...");
    if (mCodecContext) {
        avcodec_free_context(&mCodecContext);
        mCodecContext = nullptr;
    }
    return RESULT_OK;
}

void FFmpegAudioDecoder::SetDecodeCallback(AudioDecodeCallback *callback) {
    mAudioDecodedCallback = callback;
}
