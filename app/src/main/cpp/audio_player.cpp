//
// Created by frank on 2018/2/1.
//
#include <jni.h>
#include <cstdlib>
#include <unistd.h>
#include "visualizer/frank_visualizer.h"

#ifdef __cplusplus
extern "C" {
#endif

#include "libavformat/avformat.h"
#include "libavcodec/avcodec.h"
#include "libswscale/swscale.h"
#include "libswresample/swresample.h"
#include "libavfilter/buffersink.h"
#include "libavfilter/buffersrc.h"
#include "libavfilter/avfiltergraph.h"
#include "libavutil/opt.h"
#include "ffmpeg_jni_define.h"

#ifdef __cplusplus
}
#endif

#define TAG "AudioPlayer"
#define VOLUME_VAL 0.50
#define SLEEP_TIME 1000 * 16
#define MAX_AUDIO_FRAME_SIZE 48000 * 4

int filter_again = 0;
int filter_release = 0;
const char *filter_desc = "superequalizer=6b=4:8b=5:10b=5";
FrankVisualizer *mVisualizer;

void fft_callback(JNIEnv *jniEnv, jobject thiz, jmethodID fft_method, int8_t* arg, int samples);

int init_volume_filter(AVFilterGraph **graph, AVFilterContext **src, AVFilterContext **sink,
        uint64_t channel_layout, AVSampleFormat inputFormat, int sample_rate) {
    AVFilterGraph   *filter_graph;
    AVFilterContext *buffer_ctx;
    const AVFilter  *buffer;
    AVFilterContext *volume_ctx;
    const AVFilter  *volume;
    AVFilterContext *buffersink_ctx;
    const AVFilter  *buffersink;
    AVDictionary *options_dict = nullptr;
    uint8_t ch_layout[64];
    int ret;

    /* Create a new filter graph, which will contain all the filters. */
    filter_graph = avfilter_graph_alloc();
    if (!filter_graph) {
        LOGE(TAG, "Unable to create filter graph...");
        return AVERROR(ENOMEM);
    }
    /* Create the abuffer filter: feed data into the graph. */
    buffer = avfilter_get_by_name("abuffer");
    if (!buffer) {
        LOGE(TAG, "Could not find the buffer filter...");
        return AVERROR_FILTER_NOT_FOUND;
    }
    buffer_ctx = avfilter_graph_alloc_filter(filter_graph, buffer, "src");
    if (!buffer_ctx) {
        LOGE(TAG, "Could not allocate the buffer instance...");
        return AVERROR(ENOMEM);
    }
    /* Set the filter options through the AVOptions API. */
    av_get_channel_layout_string(reinterpret_cast<char *>(ch_layout), sizeof(ch_layout), 0, channel_layout);
    av_opt_set    (buffer_ctx, "channel_layout", reinterpret_cast<char *>(ch_layout), AV_OPT_SEARCH_CHILDREN);
    av_opt_set    (buffer_ctx, "sample_fmt",     av_get_sample_fmt_name(inputFormat), AV_OPT_SEARCH_CHILDREN);
    av_opt_set_q  (buffer_ctx, "time_base",      (AVRational){ 1, sample_rate },      AV_OPT_SEARCH_CHILDREN);
    av_opt_set_int(buffer_ctx, "sample_rate",    sample_rate,                         AV_OPT_SEARCH_CHILDREN);
    ret = avfilter_init_str(buffer_ctx, nullptr);
    if (ret < 0) {
        LOGE(TAG, "Could not initialize the buffer filter:%d", ret);
        return ret;
    }

    /* Create volume filter. */
    volume = avfilter_get_by_name("volume");
    if (!volume) {
        LOGE(TAG, "Could not find the volume filter...");
        return AVERROR_FILTER_NOT_FOUND;
    }
    volume_ctx = avfilter_graph_alloc_filter(filter_graph, volume, "volume");
    if (!volume_ctx) {
        LOGE(TAG, "Could not allocate the volume instance...");
        return AVERROR(ENOMEM);
    }
    /* Passing the options is as key/value pairs in a dictionary. */
    av_dict_set(&options_dict, "volume", AV_STRINGIFY(VOLUME_VAL), 0);
    ret = avfilter_init_dict(volume_ctx, &options_dict);
    av_dict_free(&options_dict);
    if (ret < 0) {
        LOGE(TAG, "Could not initialize the volume filter:%d", ret);
        return ret;
    }

    /* Create the abuffersink filter: get the filtered data from graph. */
    buffersink = avfilter_get_by_name("abuffersink");
    if (!buffersink) {
        LOGE(TAG, "Could not find the abuffersink filter...");
        return AVERROR_FILTER_NOT_FOUND;
    }
    buffersink_ctx = avfilter_graph_alloc_filter(filter_graph, buffersink, "sink");
    if (!buffersink_ctx) {
        LOGE(TAG, "Could not allocate the abuffersink instance...");
        return AVERROR(ENOMEM);
    }
    ret = avfilter_init_str(buffersink_ctx, nullptr);
    if (ret < 0) {
        LOGE(TAG, "Could not initialize the abuffersink instance:%d", ret);
        return ret;
    }
    /* Connect the filters */
    ret = avfilter_link(buffer_ctx, 0, volume_ctx, 0);
    if (ret >= 0)
        ret = avfilter_link(volume_ctx, 0, buffersink_ctx, 0);
    if (ret < 0) {
        LOGE(TAG, "Error connecting filters:%d", ret);
        return ret;
    }
    /* Configure the graph. */
    ret = avfilter_graph_config(filter_graph, nullptr);
    if (ret < 0) {
        LOGE(TAG, "Error configuring the filter graph:%d", ret);
        return ret;
    }
    *graph = filter_graph;
    *src   = buffer_ctx;
    *sink  = buffersink_ctx;
    return 0;
}

int init_equalizer_filter(const char *filter_desc, AVCodecContext *codecCtx, AVFilterGraph **graph,
        AVFilterContext **src, AVFilterContext **sink) {
    int ret = 0;
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
        LOGE(TAG, "Cannot create buffer source:%d", ret);
        goto end;
    }
    /* buffer audio sink: to terminate the filter chain. */
    ret = avfilter_graph_create_filter(&buffersink_ctx, buffersink, "out",
                                       nullptr, nullptr, filter_graph);
    if (ret < 0) {
        LOGE(TAG, "Cannot create buffer sink:%d", ret);
        goto end;
    }

    outputs->name = av_strdup("in");
    outputs->filter_ctx = buffersrc_ctx;
    outputs->pad_idx = 0;
    outputs->next = nullptr;
    inputs->name = av_strdup("out");
    inputs->filter_ctx = buffersink_ctx;
    inputs->pad_idx = 0;
    inputs->next = nullptr;

    if ((ret = avfilter_graph_parse_ptr(filter_graph, filter_desc,
                                        &inputs, &outputs, nullptr)) < 0) {
        LOGE(TAG, "avfilter_graph_parse_ptr error:%d", ret);
        goto end;
    }
    if ((ret = avfilter_graph_config(filter_graph, nullptr)) < 0) {
        LOGE(TAG, "avfilter_graph_config error:%d", ret);
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

AUDIO_PLAYER_FUNC(void, play, jstring input_jstr, jstring filter_jstr) {
    int got_frame = 0, ret = 0;
    AVFilterGraph *audioFilterGraph;
    AVFilterContext *audioSrcContext;
    AVFilterContext *audioSinkContext;

    const char *input_cstr = env->GetStringUTFChars(input_jstr, nullptr);
    LOGI(TAG, "input url=%s", input_cstr);
    filter_desc = env->GetStringUTFChars(filter_jstr, nullptr);
    LOGE(TAG, "filter_desc=%s", filter_desc);
    av_register_all();
    AVFormatContext *pFormatCtx = avformat_alloc_context();
    if (avformat_open_input(&pFormatCtx, input_cstr, nullptr, nullptr) != 0) {
        LOGE(TAG, "Couldn't open the audio file!");
        return;
    }
    if (avformat_find_stream_info(pFormatCtx, nullptr) < 0) {
        LOGE(TAG, "Couldn't find stream info!");
        return;
    }
    int i = 0, audio_stream_idx = -1;
    for (; i < pFormatCtx->nb_streams; i++) {
        if (pFormatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
            audio_stream_idx = i;
            break;
        }
    }
    AVCodecContext *codecCtx = pFormatCtx->streams[audio_stream_idx]->codec;
    AVCodec *codec = avcodec_find_decoder(codecCtx->codec_id);
    if (codec == nullptr) {
        LOGE(TAG, "Couldn't find audio decoder!");
        return;
    }
    if (avcodec_open2(codecCtx, codec, nullptr) < 0) {
        LOGE(TAG, "Couldn't open audio decoder");
        return;
    }
    auto *packet = (AVPacket *) av_malloc(sizeof(AVPacket));
    AVFrame *frame = av_frame_alloc();
    SwrContext *swrCtx = swr_alloc();
    enum AVSampleFormat in_sample_fmt = codecCtx->sample_fmt;
    enum AVSampleFormat out_sample_fmt = AV_SAMPLE_FMT_S16;
    int in_sample_rate = codecCtx->sample_rate;
    int out_sample_rate = in_sample_rate;
    uint64_t in_ch_layout = codecCtx->channel_layout;
    uint64_t out_ch_layout = AV_CH_LAYOUT_STEREO;

    swr_alloc_set_opts(swrCtx,
                       out_ch_layout, out_sample_fmt, out_sample_rate,
                       in_ch_layout, in_sample_fmt, in_sample_rate, 0, nullptr);
    swr_init(swrCtx);
    int out_channel = av_get_channel_layout_nb_channels(out_ch_layout);
    jclass player_class = env->GetObjectClass(thiz);
    if (!player_class) {
        LOGE(TAG, "player_class not found...");
    }
    //get AudioTrack by reflection
    jmethodID audio_track_method = env->GetMethodID(player_class, "createAudioTrack",
            "(II)Landroid/media/AudioTrack;");
    if (!audio_track_method) {
        LOGE(TAG, "audio_track_method not found...");
    }
    jobject audio_track = env->CallObjectMethod(thiz, audio_track_method, out_sample_rate, out_channel);
    //call play method
    jclass audio_track_class = env->GetObjectClass(audio_track);
    jmethodID audio_track_play_mid = env->GetMethodID(audio_track_class, "play", "()V");
    env->CallVoidMethod(audio_track, audio_track_play_mid);
    //get write method
    jmethodID audio_track_write_mid = env->GetMethodID(audio_track_class, "write", "([BII)I");
    auto *out_buffer = (uint8_t *) av_malloc(MAX_AUDIO_FRAME_SIZE);

    /* Set up the filter graph. */
    AVFrame *filter_frame = av_frame_alloc();
//    ret = init_volume_filter(&audioFilterGraph, &audioSrcContext, &audioSinkContext,
//            in_ch_layout, in_sample_fmt, in_sample_rate);
    ret = init_equalizer_filter(filter_desc, codecCtx, &audioFilterGraph, &audioSrcContext, &audioSinkContext);
    if (ret < 0) {
        LOGE(TAG, "Unable to init filter graph:%d", ret);
//        goto end;
    }

    jmethodID fft_method = env->GetMethodID(player_class, "fftCallbackFromJNI", "([B)V");

    mVisualizer = new FrankVisualizer();
    mVisualizer->init_visualizer();

    //read audio frame
    while (av_read_frame(pFormatCtx, packet) >= 0 && !filter_release) {
        if (packet->stream_index != audio_stream_idx) {
            av_packet_unref(packet);
            continue;
        }
        if (filter_again) {
            filter_again = 0;
            avfilter_graph_free(&audioFilterGraph);
            if (init_equalizer_filter(filter_desc, codecCtx, &audioFilterGraph, &audioSrcContext, &audioSinkContext) < 0) {
                LOGE(TAG, "init_filter error, ret=%d\n", ret);
                goto end;
            }
            LOGE(TAG, "play again,filter_descr=_=%s", filter_desc);
        }
        ret = avcodec_decode_audio4(codecCtx, frame, &got_frame, packet);
        if (ret < 0) {
            break;
        }
        if (got_frame > 0) {

            int nb_samples = frame->nb_samples < MAX_FFT_SIZE ? frame->nb_samples : MAX_FFT_SIZE;
            if (nb_samples >= MIN_FFT_SIZE) {
                int8_t *output_data = mVisualizer->fft_run(frame->data[0], nb_samples);
                fft_callback(env, thiz, fft_method, output_data, mVisualizer->getOutputSample());
            }

            ret = av_buffersrc_add_frame(audioSrcContext, frame);
            if (ret < 0) {
                LOGE(TAG, "Error add the frame to the filter graph:%d", ret);
            }
            /* Get all the filtered output that is available. */
            while (1) {
                ret = av_buffersink_get_frame(audioSinkContext, filter_frame);
                if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
                    break;
                if (ret < 0) {
                    LOGE(TAG, "Error get the frame from the filter graph:%d", ret);
                    goto end;
                }
                //convert audio format
                swr_convert(swrCtx, &out_buffer, MAX_AUDIO_FRAME_SIZE,
                            (const uint8_t **) /*frame*/filter_frame->data, /*frame*/filter_frame->nb_samples);
                int out_buffer_size = av_samples_get_buffer_size(nullptr, out_channel,
                        /*frame*/filter_frame->nb_samples, out_sample_fmt, 1);

                jbyteArray audio_sample_array = env->NewByteArray(out_buffer_size);
                jbyte *sample_byte_array = env->GetByteArrayElements(audio_sample_array, nullptr);
                memcpy(sample_byte_array, out_buffer, (size_t) out_buffer_size);
                env->ReleaseByteArrayElements(audio_sample_array, sample_byte_array, 0);
                //call write method to play
                env->CallIntMethod(audio_track, audio_track_write_mid,
                                   audio_sample_array, 0, out_buffer_size);
                env->DeleteLocalRef(audio_sample_array);
                av_frame_unref(filter_frame);
                usleep(SLEEP_TIME);
            }
        }
        av_packet_unref(packet);
    }
end:
    av_free(out_buffer);
    swr_free(&swrCtx);
    avfilter_graph_free(&audioFilterGraph);
//    avcodec_free_context(&codecCtx);
    avformat_close_input(&pFormatCtx);
    av_frame_free(&frame);
    av_frame_free(&filter_frame);
    env->ReleaseStringUTFChars(input_jstr, input_cstr);
    env->ReleaseStringUTFChars(filter_jstr, filter_desc);
    jmethodID releaseMethod = env->GetMethodID(player_class, "releaseAudioTrack", "()V");
    env->CallVoidMethod(thiz, releaseMethod);
    filter_again = 0;
    filter_release = 0;
    mVisualizer->release_visualizer();
    LOGE(TAG, "audio release...");
}

AUDIO_PLAYER_FUNC(void, again, jstring filter_jstr) {
    if (!filter_jstr) return;
    filter_again = 1;
    filter_desc = env->GetStringUTFChars(filter_jstr, nullptr);
}

AUDIO_PLAYER_FUNC(void, release) {
    filter_release = 1;
}

void fft_callback(JNIEnv *jniEnv, jobject thiz, jmethodID fft_method, int8_t * arg, int samples) {
    jbyteArray dataArray = jniEnv->NewByteArray(samples);
    jniEnv->SetByteArrayRegion(dataArray, 0, samples, arg);
    jniEnv->CallVoidMethod(thiz, fft_method, dataArray);
    jniEnv->DeleteLocalRef(dataArray);
}