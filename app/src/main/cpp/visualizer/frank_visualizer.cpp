//
// Created by frank on 2021/8/16.
//

#include "frank_visualizer.h"

#include <android/log.h>
#define LOG_TAG "frank_visualizer"
#define LOGE(...) ((void)__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, \
                   __VA_ARGS__))


void short_to_float_array (const short *in, float *out, int len) {
    for (int i = 0; i < len; ++i) {
        out [i] = (float) (in [i] / (1.0 * 0x8000)) ;
    }
}

// float FFT algorithm
void fft_float(filter_sys_t *p_sys) {
    int nb_samples = p_sys->nb_samples;
    int out_samples = p_sys->out_samples;

    fft_state *p_state = nullptr; /* internal FFT data */
    DEFINE_WIND_CONTEXT(wind_ctx); /* internal window data */

    unsigned i;
    float p_output[out_samples];           /* Raw FFT Result  */
    int16_t p_buffer1[out_samples];        /* Buffer on which we perform the FFT*/
    float *p_buffl;
    if (p_sys->convert_to_float) {
        float p_buff[out_samples];
        short_to_float_array((const short *) p_sys->data, p_buff, out_samples);
        p_buffl = p_buff;
    } else {
        p_buffl = (float *) (p_sys->data);
    }

    int16_t  *p_buffs;                         /* int16_t converted buffer */
    int16_t  *p_s16_buff;                      /* int16_t converted buffer */

    if (!nb_samples) {
        LOGE("no samples yet...");
        goto release;
    }

    /* Allocate the buffer only if the number of samples change */
    if (nb_samples != p_sys->i_prev_nb_samples) {
        if (p_sys->p_prev_s16_buff) delete [] (p_sys->p_prev_s16_buff);
        p_sys->p_prev_s16_buff = new int16_t [nb_samples * p_sys->i_channels];
        if (!p_sys->p_prev_s16_buff)
            goto release;
        p_sys->i_prev_nb_samples = nb_samples;
    }
    p_buffs = p_s16_buff = p_sys->p_prev_s16_buff;

    /* Convert the buffer to int16_t */
    for (i = nb_samples * p_sys->i_channels; i--;) {
        union {float f; int32_t i;} u{};

        u.f = *p_buffl + 384.f;
        if (u.i > 0x43c07fff)
            *p_buffs = 32767;
        else if (u.i < 0x43bf8000)
            *p_buffs = -32768;
        else
            *p_buffs = u.i - 0x43c00000;

        p_buffl++; p_buffs++;
    }
    p_state = visual_fft_init();
    if (!p_state) {
        LOGE("unable to initialize FFT transform...");
        goto release;
    }
    if (!window_init(out_samples, p_sys->wind_param, &wind_ctx)) {
        LOGE("unable to initialize FFT window...");
        goto release;
    }
    p_buffs = p_s16_buff;
    for (i = 0 ; i < out_samples; i++) {
        p_output[i] = 0;
        p_buffer1[i] = *p_buffs;

        p_buffs += p_sys->i_channels;
        if (p_buffs >= &p_s16_buff[nb_samples * p_sys->i_channels])
            p_buffs = p_s16_buff;
    }
    window_scale_in_place (p_buffer1, &wind_ctx);
    fft_perform (p_buffer1, p_output, p_state);

    for (i = 0; i < out_samples; ++i) {
        int16_t temp = p_output[i] * (2 ^ 16)
                           / ((out_samples / 2 * 32768) ^ 2);
        p_sys->output[i] = temp & 0xFF;
    }

release:
    window_close(&wind_ctx);
    fft_close(p_state);
}

int fft_fixed_internal(uint8_t *fft, const uint8_t *waveform, int mCaptureSize) {
    int32_t workspace[mCaptureSize >> 1];
    int32_t nonzero = 0;

    for (uint32_t i = 0; i < mCaptureSize; i += 2) {
        workspace[i >> 1] =
                ((waveform[i] ^ 0x80) << 24) | ((waveform[i + 1] ^ 0x80) << 8);
        nonzero |= workspace[i >> 1];
    }

    if (nonzero) {
        fixed_fft_real(mCaptureSize >> 1, workspace);
    }

    for (uint32_t i = 0; i < mCaptureSize; i += 2) {
        short tmp = workspace[i >> 1] >> 21;
        while (tmp > 127 || tmp < -128) tmp >>= 1;
        fft[i] = tmp;
        tmp = workspace[i >> 1];
        tmp >>= 5;
        while (tmp > 127 || tmp < -128) tmp >>= 1;
        fft[i + 1] = tmp;
    }

    return 0;
}

// fixed FFT algorithm
void fft_fixed(filter_sys_t *p_sys) {
    int nb_samples = p_sys->nb_samples;
    int out_samples = p_sys->out_samples;

    DEFINE_WIND_CONTEXT(wind_ctx);
    if (!nb_samples) {
        LOGE("no samples yet...");
        goto release;
    }

    if (!window_init(out_samples, p_sys->wind_param, &wind_ctx)) {
        LOGE("unable to initialize FFT window...");
        goto release;
    }

    window_scale_in_place ((int16_t *) p_sys->data, &wind_ctx);
    fft_fixed_internal((uint8_t *) p_sys->output, p_sys->data, out_samples);

release:
    window_close(&wind_ctx);
}

FrankVisualizer::FrankVisualizer() {
    LOGE("FrankVisualizer init...");
}

FrankVisualizer::~FrankVisualizer() {
    LOGE("FrankVisualizer release...");
}

int8_t* FrankVisualizer::fft_run(uint8_t *input_buffer, int nb_samples) {
    mFftLock.lock();
    fft_context->nb_samples = nb_samples;
    memcpy(fft_context->data, input_buffer, static_cast<size_t>(nb_samples));
    filter_sys_t *p_sys = fft_context;
#ifdef FIXED_FFT
    fft_fixed(p_sys);
#else
    fft_float(p_sys);
#endif
    mFftLock.unlock();
    return fft_context->output;
}

int FrankVisualizer::getOutputSample() {
    if (fft_context) {
        return fft_context->out_samples;
    }
    return 0;
}

int FrankVisualizer::init_visualizer() {
    fft_context = new filter_sys_t();
    filter_sys_t *p_filter = fft_context;
    if (!p_filter)
        return -1;

    p_filter->convert_to_float = false;
    p_filter->i_channels = 1;
    p_filter->i_prev_nb_samples = 0;
    p_filter->p_prev_s16_buff = nullptr;
    p_filter->data = nullptr;
    p_filter->data_size = 0;
    p_filter->nb_samples = 0;
#ifdef FIXED_FFT
    p_filter->out_samples = 512;
#else
    p_filter->out_samples = FFT_BUFFER_SIZE;
#endif

    p_filter->wind_param = new window_param();
    /* Fetch the FFT window parameters */
    window_get_param(p_filter->wind_param);
    p_filter->data_size = MAX_FFT_SIZE;
    p_filter->data = new uint8_t[MAX_FFT_SIZE];
    memset(p_filter->data, 0, MAX_FFT_SIZE);
    p_filter->output = new int8_t[p_filter->out_samples];
    memset(p_filter->output, 0, p_filter->out_samples);
    return 0;
}

void FrankVisualizer::release_visualizer() {
    mFftLock.lock();
    filter_sys_t *p_filter = fft_context;
    if (!p_filter) return;
    if (p_filter->p_prev_s16_buff) {
        delete [] (p_filter->p_prev_s16_buff);
    }
    if (p_filter->wind_param) {
        delete (p_filter->wind_param);
    }
    if (p_filter->data) {
        delete [] (p_filter->data);
    }
    if (p_filter->output) {
        delete [] (p_filter->output);
    }
    delete p_filter;
    mFftLock.unlock();
}