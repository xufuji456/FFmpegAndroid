//
// Created by xu fulong on 2022/5/13.
//

#include <cut_video.h>

#ifdef __ANDROID__
#include <android/log.h>
#define LOGE(FORMAT, ...) __android_log_print(ANDROID_LOG_ERROR, "CutVideo", FORMAT, ##__VA_ARGS__)
#else
#include <stdio.h>
#define LOGE(FORMAT, ...) printf(FORMAT, ##__VA_ARGS__)
#endif

int CutVideo::open_output_file(AVFormatContext *ifmt_ctx, const char *filename)
{
    int ret;
    avformat_alloc_output_context2(&ofmt_ctx, nullptr, nullptr, filename);
    if (!ofmt_ctx) {
        LOGE("Could not create output context\n");
        return AVERROR_UNKNOWN;
    }

    for (int i = 0; i < ifmt_ctx->nb_streams; i++) {
        AVStream* in_stream = ifmt_ctx->streams[i];
        AVCodecParameters* codecpar = in_stream->codecpar;

        AVCodec* dec = avcodec_find_decoder(codecpar->codec_id);
        AVStream* out_stream = avformat_new_stream(ofmt_ctx, dec);
        if (!out_stream) {
            LOGE("Failed allocating output stream\n");
            ret = AVERROR_UNKNOWN;
            return ret;
        }
        avcodec_parameters_copy(out_stream->codecpar, codecpar);
    }

    av_dump_format(ofmt_ctx, 0, filename, 1);
    if (!(ofmt_ctx->oformat->flags & AVFMT_NOFILE)) {
        ret = avio_open(&ofmt_ctx->pb, filename, AVIO_FLAG_WRITE);
        if (ret < 0) {
            LOGE("Could not open output file %s\n", filename);
            return ret;
        }
    }
    /* init muxer, write output file header */
    ret = avformat_write_header(ofmt_ctx, nullptr);
    if (ret < 0) {
        LOGE("Error occurred when opening output file\n");
        return ret;
    }

    return 0;
}

void CutVideo:: setParam(int64_t start_time, int64_t duration) {
    m_startTime = start_time;
    m_duration  = duration;
}

AVPacket* CutVideo::copy_packet(AVFormatContext *ifmt_ctx, AVPacket *packet) {
    AVStream* in_stream;
    AVStream* out_stream;
    auto* pkt = (AVPacket*)av_malloc(sizeof(AVPacket));
    av_new_packet(pkt, 0);
    if (0 == av_packet_ref(pkt, packet)) {
        in_stream  = ifmt_ctx->streams[pkt->stream_index];
        out_stream = ofmt_ctx->streams[pkt->stream_index];
        // 转换pts与dts
        pkt->pts = av_rescale_q_rnd(pkt->pts, in_stream->time_base, out_stream->time_base,
                                    static_cast<AVRounding>(AV_ROUND_NEAR_INF |
                                                            AV_ROUND_PASS_MINMAX));
        pkt->dts = av_rescale_q_rnd(pkt->dts, in_stream->time_base, out_stream->time_base,
                                    static_cast<AVRounding>(AV_ROUND_NEAR_INF |
                                                            AV_ROUND_PASS_MINMAX));
        pkt->duration = av_rescale_q(pkt->duration, in_stream->time_base, out_stream->time_base);
        return pkt;
    }
    return nullptr;
}

int CutVideo::write_internal(AVFormatContext *ifmt_ctx, AVPacket *packet)
{
    int ret;
    AVPacket *pkt = copy_packet(ifmt_ctx, packet);
    if (pkt == nullptr) {
        LOGE("packet is NULL\n");
        return -1;
    }
    // sub the first timestamp offset
    pkt->pts = pkt->pts - start_pts;
    pkt->dts = pkt->dts - start_dts;
    // write packet into file
    if ((ret = av_interleaved_write_frame(ofmt_ctx, pkt)) < 0) {
        LOGE("Error muxing packet\n");
    }
    av_packet_unref(pkt);
    return ret;
}

void CutVideo::write_output_file(AVFormatContext *ifmt_ctx, AVPacket *packet) {
    int64_t timestamp = packet->pts * av_q2d(ifmt_ctx->streams[packet->stream_index]->time_base);
    if (timestamp >= m_startTime && timestamp < m_startTime + m_duration) {
//        if (start_pts == 0 && start_dts == 0) {
//            start_pts = packet->pts;
//            start_dts = packet->dts;
//        }
        LOGE("write frame timestamp=%ld\n", timestamp);
        write_internal(ifmt_ctx, packet);
    }
}

void CutVideo::close_output_file() {
    if (!ofmt_ctx)
        return;
    av_write_trailer(ofmt_ctx);
    if (!(ofmt_ctx->oformat->flags & AVFMT_NOFILE)) {
        avio_close(ofmt_ctx->pb);
    }
    avformat_free_context(ofmt_ctx);
}