//
// Created by xu fulong on 2022/9/4.
//

#include "ff_audio_player.h"

#define AUDIO_TAG "AudioPlayer"
#define BUFFER_SIZE (48000 * 10)

const char *FILTER_DESC = "superequalizer=6b=4:8b=5:10b=5";

int initFilter(const char *filterDesc, AVCodecContext *codecCtx, AVFilterGraph **graph,
                          AVFilterContext **src, AVFilterContext **sink) {
    int ret;
    char args[512];
    AVFilterContext *buffersrc_ctx;
    AVFilterContext *buffersink_ctx;
    AVRational time_base       = codecCtx->time_base;
    AVFilterInOut *inputs      = avfilter_inout_alloc();
    AVFilterInOut *outputs     = avfilter_inout_alloc();
    const AVFilter *buffersrc  = avfilter_get_by_name("abuffer");
    const AVFilter *buffersink = avfilter_get_by_name("abuffersink");

    AVFilterGraph *filter_graph = avfilter_graph_alloc();
    if (!outputs || !inputs || !filter_graph) {
        ret = AVERROR(ENOMEM);
        goto end;
    }

    /* buffer audio source: the decoded frames from the decoder will be inserted here. */
    if (!codecCtx->channel_layout)
        codecCtx->channel_layout = static_cast<uint64_t>(av_get_default_channel_layout(
                codecCtx->channels));
    snprintf(args, sizeof(args),
             "time_base=%d/%d:sample_rate=%d:sample_fmt=%s:channel_layout=0x%" PRIx64 "",
             time_base.num, time_base.den, codecCtx->sample_rate,
             av_get_sample_fmt_name(codecCtx->sample_fmt), codecCtx->channel_layout);

    ret = avfilter_graph_create_filter(&buffersrc_ctx, buffersrc, "in",
                                       args, nullptr, filter_graph);
    if (ret < 0) {
        LOGE(AUDIO_TAG, "Cannot create buffer source:%d", ret);
        goto end;
    }
    /* buffer audio sink: to terminate the filter chain. */
    ret = avfilter_graph_create_filter(&buffersink_ctx, buffersink, "out",
                                       nullptr, nullptr, filter_graph);
    if (ret < 0) {
        LOGE(AUDIO_TAG, "Cannot create buffer sink:%d", ret);
        goto end;
    }

    outputs->name       = av_strdup("in");
    outputs->filter_ctx = buffersrc_ctx;
    outputs->pad_idx    = 0;
    outputs->next       = nullptr;
    inputs->name        = av_strdup("out");
    inputs->filter_ctx  = buffersink_ctx;
    inputs->pad_idx     = 0;
    inputs->next        = nullptr;

    if ((ret = avfilter_graph_parse_ptr(filter_graph, filterDesc,
                                        &inputs, &outputs, nullptr)) < 0) {
        LOGE(AUDIO_TAG, "avfilter_graph_parse_ptr error:%d", ret);
        goto end;
    }
    if ((ret = avfilter_graph_config(filter_graph, nullptr)) < 0) {
        LOGE(AUDIO_TAG, "avfilter_graph_config error:%d", ret);
        goto end;
    }
    *graph = filter_graph;
    *src   = buffersrc_ctx;
    *sink  = buffersink_ctx;
end:
    avfilter_inout_free(&inputs);
    avfilter_inout_free(&outputs);
    return ret;
}

FFAudioPlayer::FFAudioPlayer() {
    m_state = new AudioPlayerState();
    m_visualizer = nullptr;
}

FFAudioPlayer::~FFAudioPlayer() {
    delete m_state;
}

int FFAudioPlayer::open(const char *path) {
    if (!path)
        return -1;

    int ret;
    const AVCodec *codec;
    m_state->inputFrame = av_frame_alloc();
    m_state->packet     = av_packet_alloc();
    m_state->outBuffer  = new uint8_t [BUFFER_SIZE];

    // open input stream
    ret = avformat_open_input(&m_state->formatContext, path, nullptr, nullptr);
    if (ret < 0) {
        LOGE(AUDIO_TAG, "avformat_open_input error=%s", av_err2str(ret));
        return ret;
    }
    // (if need)find info: width、height、sample_rate、duration
    avformat_find_stream_info(m_state->formatContext, nullptr);
    // find audio index
    for (int i=0; i<m_state->formatContext->nb_streams; i++) {
        if (AVMEDIA_TYPE_AUDIO == m_state->formatContext->streams[i]->codecpar->codec_type) {
            m_state->audioIndex = i;
            break;
        }
    }
    if (m_state->audioIndex == -1) {
        return -1;
    }
    // find audio decoder
    codec = avcodec_find_decoder(m_state->formatContext->streams[m_state->audioIndex]->codecpar->codec_id);
    m_state->codecContext = avcodec_alloc_context3(codec);
    avcodec_parameters_to_context(m_state->codecContext, m_state->formatContext->streams[m_state->audioIndex]->codecpar);
    // open decoder
    ret = avcodec_open2(m_state->codecContext, codec, nullptr);
    if (ret < 0) {
        LOGE(AUDIO_TAG, "avcodec_open2 error=%s", av_err2str(ret));
        return ret;
    }
    // input and output params
    int in_sample_rate       = m_state->codecContext->sample_rate;
    auto in_sample_fmt       = m_state->codecContext->sample_fmt;
    int in_ch_layout         = (int)m_state->codecContext->channel_layout;
    m_state->out_sample_rate = in_sample_rate;
    m_state->out_sample_fmt  = AV_SAMPLE_FMT_S16;
    m_state->out_ch_layout   = AV_CH_LAYOUT_STEREO;
    m_state->out_channel     = m_state->codecContext->channels;
    // init resample context
    m_state->swrContext = swr_alloc();
    swr_alloc_set_opts(m_state->swrContext, m_state->out_ch_layout, m_state->out_sample_fmt, m_state->out_sample_rate,
                       in_ch_layout, in_sample_fmt, in_sample_rate, 0, nullptr);
    swr_init(m_state->swrContext);
    // init filter graph
    m_state->filterFrame = av_frame_alloc();
    initFilter(FILTER_DESC, m_state->codecContext, &m_state->audioFilterGraph,
                   &m_state->audioSrcContext, &m_state->audioSinkContext);
    // init visualizer
    m_visualizer = new FrankVisualizer();
    m_visualizer->init_visualizer();

    return 0;
}

int FFAudioPlayer::getChannel() const {
    return m_state->out_channel;
}

int FFAudioPlayer::getSampleRate() const {
    return m_state->out_sample_rate;
}

int FFAudioPlayer::decodeAudio() {
    int ret;
    if (m_state->exitPlaying.load()) {
        return -1;
    }
    // demux: read a frame(should be demux thread)
    ret = av_read_frame(m_state->formatContext, m_state->packet);
    if (ret < 0) {
        return ret;
    }
    // see if audio packet
    if (m_state->packet->stream_index != m_state->audioIndex) {
        return 0;
    }
    // decode audio frame(should be decode thread)
    ret = avcodec_send_packet(m_state->codecContext, m_state->packet);
    if (ret < 0) {
        LOGE(AUDIO_TAG, "avcodec_send_packet=%s", av_err2str(ret));
    }
    ret = avcodec_receive_frame(m_state->codecContext, m_state->inputFrame);
    if (ret < 0) {
        if (ret == AVERROR(EAGAIN)) {
            return 0;
        } else {
            return ret;
        }
    }

    // visualizer: do fft
    int nb_samples = m_state->inputFrame->nb_samples < MAX_FFT_SIZE ? m_state->inputFrame->nb_samples : MAX_FFT_SIZE;
    if (m_enableVisualizer && nb_samples >= MIN_FFT_SIZE) {
        m_visualizer->fft_run(m_state->inputFrame->data[0], nb_samples);
    }

    // change filter
    if (m_state->filterAgain) {
        m_state->filterAgain = false;
        avfilter_graph_free(&m_state->audioFilterGraph);
        if ((ret = initFilter(m_state->filterDesc, m_state->codecContext, &m_state->audioFilterGraph,
                              &m_state->audioSrcContext, &m_state->audioSinkContext)) < 0) {
            LOGE(AUDIO_TAG, "init_filter error, ret=%d\n", ret);
            return ret;
        }
    }

    // put into filter
    ret = av_buffersrc_add_frame(m_state->audioSrcContext, m_state->inputFrame);
    if (ret < 0) {
        LOGE(AUDIO_TAG, "av_buffersrc_add_frame error=%s", av_err2str(ret));
    }
    // drain from filter
    ret = av_buffersink_get_frame(m_state->audioSinkContext, m_state->filterFrame);
    if (ret == AVERROR(EAGAIN)) {
        return 0;
    } else if (ret == AVERROR_EOF) {
        LOGE(AUDIO_TAG, "enf of stream...");
        return ret;
    } else if (ret < 0) {
        LOGE(AUDIO_TAG, "av_buffersink_get_frame error:%s", av_err2str(ret));
        return ret;
    }

    // convert audio format and sample_rate
    swr_convert(m_state->swrContext, &m_state->outBuffer, BUFFER_SIZE,
            (const uint8_t **)(m_state->filterFrame->data), m_state->filterFrame->nb_samples);
    // get buffer size after converting
    int buffer_size = av_samples_get_buffer_size(nullptr, m_state->out_channel,
                                                 m_state->filterFrame->nb_samples, m_state->out_sample_fmt, 1);

    av_frame_unref(m_state->inputFrame);
    av_frame_unref(m_state->filterFrame);
    av_packet_unref(m_state->packet);
    return buffer_size;
}

uint8_t *FFAudioPlayer::getDecodeFrame() const {
    return m_state->outBuffer;
}

void FFAudioPlayer::setEnableVisualizer(bool enable) {
    m_enableVisualizer = enable;
}

bool FFAudioPlayer::enableVisualizer() const {
    return m_enableVisualizer;
}

int8_t* FFAudioPlayer::getFFTData() const {
    if (!m_visualizer)
        return nullptr;
    return m_visualizer->getFFTData();
}

int FFAudioPlayer::getFFTSize() const {
    if (!m_visualizer)
        return 0;
    return m_visualizer->getOutputSample();
}

void FFAudioPlayer::setFilterAgain(bool again) {
    m_state->filterAgain = again;
}

void FFAudioPlayer::setFilterDesc(const char *filterDescription) {
    m_state->filterDesc = filterDescription;
}

void FFAudioPlayer::setExit(bool exit) {
    m_state->exitPlaying = exit;
}

void FFAudioPlayer::close() {
    if (!m_state)
        return;
    if (m_state->formatContext) {
        avformat_close_input(&m_state->formatContext);
    }
    if (m_state->codecContext) {
        avcodec_free_context(&m_state->codecContext);
    }
    if (m_state->packet) {
        av_packet_free(&m_state->packet);
    }
    if (m_state->inputFrame) {
        av_frame_free(&m_state->inputFrame);
    }
    if (m_state->swrContext) {
        swr_close(m_state->swrContext);
    }
    avfilter_free(m_state->audioSrcContext);
    avfilter_free(m_state->audioSinkContext);
    if (m_state->audioFilterGraph) {
        avfilter_graph_free(&m_state->audioFilterGraph);
    }
    delete[] m_state->outBuffer;
    if (m_visualizer) {
        m_visualizer->release_visualizer();
    }
}