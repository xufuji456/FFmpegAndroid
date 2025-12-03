/**
 * Note: extractor of ffmpeg
 * Date: 2025/12/3
 * Author: frank
 */

#include "NextExtractor.h"

#include "FFmpegUtil.h"
#include "NextLog.h"
#include "NextMessage.h"

#define EXTRACTOR_TAG "RsExtractor"

NextExtractor::NextExtractor(NotifyCallback &notifyCb)
        : mNotifyCb(notifyCb) {

    mFormatCtx = avformat_alloc_context();
    mFormatCtx->interrupt_callback.callback = InterruptCallback;
    mFormatCtx->interrupt_callback.opaque = static_cast<void *>(this);
}

NextExtractor::~NextExtractor() {
    NEXT_LOGD(EXTRACTOR_TAG, "RsExtractor destructor\n");
}

static int GetRotationRound(AVStream *st) {
    if (!st)
        return -1;

    int theta = std::abs(static_cast<int>(
                                 static_cast<int64_t>(round(fabs(GetRotation(st)))) % 360));

    switch (theta) {
        case 0:
        case 90:
        case 180:
        case 270:
            break;
        case 360:
            theta = 0;
            break;
        default:
            NEXT_LOGW(EXTRACTOR_TAG, "Unknown rotate degree: %d\n", theta);
            theta = 0;
            break;
    }

    return theta;
}

int NextExtractor::Open(const std::string &url, FFmpegOption &opt,
                      std::shared_ptr<MetaData> &metadata) {
    int ret = 0;
    int streamCount = 0;
    if (!mFormatCtx || url.empty() || metadata == nullptr) {
        RS_LOGE(EXTRACTOR_TAG, "extractor Open null, [%d, %d, %d]\n",
                  !mFormatCtx, url.empty(), metadata == nullptr);
        return -1;
    }

    ret = avformat_open_input(&mFormatCtx, url.c_str(), nullptr,
                              opt.format_opts ? &opt.format_opts : nullptr);
    if (ret < 0) {
        NEXT_LOGE(EXTRACTOR_TAG, "avformat_open_input error, ret=%d\n", ret);
        return ret;
    }
    NotifyListener(MSG_OPEN_INPUT);
    av_format_inject_global_side_data(mFormatCtx);
    AVDictionary **opts = FindStreamInfoOpts(mFormatCtx, opt.codec_opts);
    streamCount = (int) mFormatCtx->nb_streams;
    do {
        if (av_stristart(url.c_str(), "data:", nullptr) && streamCount > 0) {
            int i = 0;
            for (i = 0; i < streamCount; i++) {
                if (!mFormatCtx->streams[i] || !mFormatCtx->streams[i]->codecpar ||
                    mFormatCtx->streams[i]->codecpar->profile == FF_PROFILE_UNKNOWN) {
                    break;
                }
            }

            if (i == streamCount) {
                break;
            }
        }
        ret = avformat_find_stream_info(mFormatCtx, opts);
    } while (false);

    NotifyListener(MSG_FIND_STREAM_INFO);

    for (int i = 0; i < streamCount; i++) {
        av_dict_free(&opts[i]);
    }
    av_freep(&opts);

    if (ret < 0) {
        NEXT_LOGE(EXTRACTOR_TAG, "find_stream_info error, ret=%d\n", ret);
        NotifyListener(MSG_ON_ERROR, MSG_FIND_STREAM_INFO, (int32_t) ret);
        return ret;
    }

    metadata->duration   = mFormatCtx->duration;
    metadata->bit_rate   = mFormatCtx->bit_rate;
    metadata->start_time = mFormatCtx->start_time;
    for (int i = 0; i < mFormatCtx->nb_streams; i++) {
        TrackInfo info;
        AVStream *st = mFormatCtx->streams[i];
        info.rotation = GetRotationRound(st);
        info.stream_index = st->index;

        if (st->codecpar && st->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            info.pixel_fmt = st->codecpar->format;
        }
        if (st->avg_frame_rate.num > 0 && st->avg_frame_rate.den > 0) {
            info.fps_num = st->avg_frame_rate.num;
            info.fps_den = st->avg_frame_rate.den;
        }
        if (st->r_frame_rate.num > 0 && st->r_frame_rate.den > 0) {
            info.tbr_num = st->r_frame_rate.num;
            info.tbr_den = st->r_frame_rate.den;
        }
        if (st->time_base.num > 0 && st->time_base.den > 0) {
            info.time_base_num = st->time_base.num;
            info.time_base_den = st->time_base.den;
        }
        if (st->codecpar) {
            info.width           = st->codecpar->width;
            info.height          = st->codecpar->height;
            info.sar_num         = st->codecpar->sample_aspect_ratio.num;
            info.sar_den         = st->codecpar->sample_aspect_ratio.den;
            info.bit_rate        = GetBitrate(st->codecpar);
            info.codec_id        = st->codecpar->codec_id;
            info.channels        = st->codecpar->ch_layout.nb_channels;
            info.sample_fmt      = st->codecpar->format;
            info.sample_rate     = st->codecpar->sample_rate;
            info.color_space     = st->codecpar->color_space;
            info.color_range     = st->codecpar->color_range;
            info.color_transfer  = st->codecpar->color_trc;
            info.color_primaries = st->codecpar->color_primaries;
            info.stream_type     = st->codecpar->codec_type;
            info.codec_profile   = st->codecpar->profile;
            info.extra_data_size = st->codecpar->extradata_size;

            if (st->codecpar->extradata_size > 0 && st->codecpar->extradata) {
                info.extra_data = new uint8_t[st->codecpar->extradata_size + 1];
                memcpy(info.extra_data, st->codecpar->extradata,
                       st->codecpar->extradata_size);
            }
        }

        metadata->track_info.push_back(info);
    }

    return 0;
}

int NextExtractor::ReadPacket(AVPacket *pkt) {
    int ret = -1;
    if (mFormatCtx) {
        ret = av_read_frame(mFormatCtx, pkt);
    }
    return ret;
}

int NextExtractor::Seek(int64_t timestamp, int64_t rel, int seekFlags) {
    if (mFormatCtx->start_time != AV_NOPTS_VALUE) {
        timestamp += mFormatCtx->start_time;
    }
    int64_t seek_min = rel > 0 ? timestamp - rel + 2 : INT64_MIN;
    int64_t seek_max = rel < 0 ? timestamp - rel - 2 : INT64_MAX;
    int ret = avformat_seek_file(mFormatCtx, -1,
                                 seek_min, timestamp, seek_max, seekFlags);
    return ret;
}

int NextExtractor::GetError() {
    int ret = 0;
    if (mFormatCtx && mFormatCtx->pb) {
        ret = mFormatCtx->pb->error;
    }
    return ret;
}

int NextExtractor::GetStreamType(int streamIndex) {
    int ret = -1;
    if (mFormatCtx && mFormatCtx->streams && streamIndex < mFormatCtx->nb_streams) {
        ret = static_cast<int>(mFormatCtx->streams[streamIndex]->codecpar->codec_type);
    }
    return ret;
}

void NextExtractor::SetInterrupt() {
    NEXT_LOGD(EXTRACTOR_TAG, "extractor interrupt.\n");
    bAbort.store(true);
}

void NextExtractor::Close() {
    NEXT_LOGD(EXTRACTOR_TAG, "close begin\n");
    if (mFormatCtx) {
        avformat_close_input(&mFormatCtx);
        mFormatCtx = nullptr;
    }
    NEXT_LOGD(EXTRACTOR_TAG, "close end\n");
}

int NextExtractor::InterruptCallback(void *opaque) {
    return static_cast<NextExtractor *>(opaque)->bAbort.load(
            std::memory_order_relaxed);
}

void NextExtractor::NotifyListener(int32_t what, int32_t arg1, int32_t arg2, void *obj, int len) {
    if (mNotifyCb) {
        mNotifyCb(what, arg1, arg2, obj, len);
    }
}
