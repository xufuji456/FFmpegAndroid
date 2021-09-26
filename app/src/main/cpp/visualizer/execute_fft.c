//
// Created by frank on 2021/8/16.
//

#include "execute_fft.h"

#include <android/log.h>
#define LOG_TAG "execute_fft"
#define LOGE(...) ((void)__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, \
                   __VA_ARGS__))

#define NB_BANDS 20
#define ROTATION_INCREMENT .1f
#define BAR_DECREMENT .075f

int open_visualizer(filter_sys_t *p_sys)
{
    if (p_sys == NULL)
        return -1;

    /* Create the object for the thread */
    p_sys->i_channels = 1;
    p_sys->i_prev_nb_samples = 0;
    p_sys->p_prev_s16_buff = NULL;

    window_param *w_param = (window_param*) malloc(sizeof(window_param));
    p_sys->wind_param = *w_param;

    /* Fetch the FFT window parameters */
    window_get_param(&p_sys->wind_param);

    /* Create the FIFO for the audio data. */
    vlc_queue_t *queue = vlc_queue_init(5);
    p_sys->queue = *queue;
    p_sys->dead = false;

    pthread_create (&p_sys->thread, NULL, fft_thread, p_sys);

    return 0;
}

block_t *filter_audio(filter_sys_t *p_sys, void *p_in_buf)
{
    return (block_t *) vlc_queue_push(&p_sys->queue, p_in_buf);
}

void close_visualizer(filter_sys_t *p_filter)
{
    filter_sys_t *p_sys = p_filter;
    /* Terminate the thread. */
    vlc_queue_free(&p_sys->queue);
    pthread_join(p_sys->thread, NULL);

    free(p_sys->p_prev_s16_buff);
    free(&p_sys->wind_param);
    if (p_sys->data) {
        free(p_sys->data);
    }
    if (p_sys->output) {
        free(p_sys->output);
    }
    free(p_sys);
}

static void *fft_thread(void *p_data)
{
    filter_sys_t *p_sys = (filter_sys_t*)p_data;
    block_t *block;

    float height[NB_BANDS] = {0};
    LOGE("start FFT thread...");

    while ((block = (block_t *) vlc_queue_pop(&p_sys->queue)))
    {
        LOGE("running FFT transform...");
        /* Horizontal scale for 20-band equalizer */
        const unsigned xscale[] = {0,1,2,3,4,5,6,7,8,11,15,20,27,
                                   36,47,62,82,107,141,184,255};

        fft_state *p_state = NULL; /* internal FFT data */
        DEFINE_WIND_CONTEXT(wind_ctx); /* internal window data */

        unsigned i, j;
        float p_output[FFT_BUFFER_SIZE];           /* Raw FFT Result  */
        int16_t p_buffer1[FFT_BUFFER_SIZE];        /* Buffer on which we perform
                                                      the FFT (first channel) */
        int16_t p_dest[FFT_BUFFER_SIZE];           /* Adapted FFT result */
        float *p_buffl = (float*)block->p_buffer;  /* Original buffer */

        int16_t  *p_buffs;                         /* int16_t converted buffer */
        int16_t  *p_s16_buff;                      /* int16_t converted buffer */

        if (!block->i_nb_samples) {
            LOGE("no samples yet...");
            goto release;
        }

        /* Allocate the buffer only if the number of samples change */
        if (block->i_nb_samples != p_sys->i_prev_nb_samples)
        {
            free(p_sys->p_prev_s16_buff);
            p_sys->p_prev_s16_buff = (short *) malloc(block->i_nb_samples *
                                            p_sys->i_channels *
                                            sizeof(int16_t));
            if (!p_sys->p_prev_s16_buff)
                goto release;
            p_sys->i_prev_nb_samples = block->i_nb_samples;
        }
        p_buffs = p_s16_buff = p_sys->p_prev_s16_buff;

        /* Convert the buffer to int16_t */
        for (i = block->i_nb_samples * p_sys->i_channels; i--;)
        {
            union {float f; int32_t i;} u;

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
        if (!p_state)
        {
            LOGE("unable to initialize FFT transform...");
            goto release;
        }
        if (!window_init(FFT_BUFFER_SIZE, &p_sys->wind_param, &wind_ctx))
        {
            LOGE("unable to initialize FFT window...");
            goto release;
        }
        p_buffs = p_s16_buff;
        for (i = 0 ; i < FFT_BUFFER_SIZE; i++)
        {
            p_output[i] = 0;
            p_buffer1[i] = *p_buffs;

            p_buffs += p_sys->i_channels;
            if (p_buffs >= &p_s16_buff[block->i_nb_samples * p_sys->i_channels])
                p_buffs = p_s16_buff;
        }
        window_scale_in_place (p_buffer1, &wind_ctx);
        fft_perform (p_buffer1, p_output, p_state);

        for (i = 0; i< FFT_BUFFER_SIZE; ++i)
            p_dest[i] = p_output[i] *  (2 ^ 16)
                        / ((FFT_BUFFER_SIZE / 2 * 32768) ^ 2);

        for (i = 0 ; i < NB_BANDS; i++)
        {
            /* Decrease the previous size of the bar. */
            height[i] -= BAR_DECREMENT;
            if (height[i] < 0)
                height[i] = 0;

            int y = 0;
            /* We search the maximum on one scale
               to determine the current size of the bar. */
            for (j = xscale[i]; j < xscale[i + 1]; j++)
            {
                if (p_dest[j] > y)
                    y = p_dest[j];
            }
            /* Calculate the height of the bar */
            float new_height = y != 0 ? logf(y) * 0.4f : 0;
            height[i] = new_height > height[i]
                        ? new_height : height[i];
        }

        usleep(10*1000 /*block->i_pts + (block->i_length / 2)*/);
        block->fft_callback.callback(p_dest);

release:
        window_close(&wind_ctx);
        fft_close(p_state);
    }

    return NULL;
}


int init_visualizer(filter_sys_t *p_filter)
{
    if (p_filter == NULL)
        return -1;

    /* Create the object for the thread */
    p_filter->i_channels = 1;
    p_filter->i_prev_nb_samples = 0;
    p_filter->p_prev_s16_buff = NULL;

    window_param *w_param = (window_param*) malloc(sizeof(window_param));
    p_filter->wind_param = *w_param;

    /* Fetch the FFT window parameters */
    window_get_param(&p_filter->wind_param);

    p_filter->data = NULL;
    p_filter->data_size = 0;
    p_filter->nb_samples = 0;
    p_filter->out_samples = FFT_BUFFER_SIZE;
    p_filter->output = (int8_t *) (malloc(p_filter->out_samples * sizeof(int8_t)));
    return 0;
}

void release_visualizer(filter_sys_t *p_filter)
{
    if (!p_filter) return;
    if (p_filter->p_prev_s16_buff) {
        free(p_filter->p_prev_s16_buff);
    }
    free(&p_filter->wind_param);
    if (p_filter->data) {
        free(p_filter->data);
    }
    if (p_filter->output) {
        free(p_filter->output);
    }
    free(p_filter);
}

int ensure_memory(filter_sys_t *fft_filter, int nb_samples) {
    if (nb_samples != fft_filter->nb_samples && nb_samples > fft_filter->out_samples) {
        if (!fft_filter->data) {
            fft_filter->data_size = nb_samples;
            fft_filter->data = (uint8_t *) (malloc(nb_samples * sizeof(uint8_t)));
        } else if (nb_samples > fft_filter->data_size) {
            fft_filter->data_size = nb_samples;
            fft_filter->data = (uint8_t *) (realloc(fft_filter->data, nb_samples * sizeof(uint8_t)));
        }
        if (!fft_filter->data) return -1;
        fft_filter->nb_samples = nb_samples;
    }
    return 0;
}

void fft_once(filter_sys_t *p_sys)
{
    int nb_samples = p_sys->nb_samples;
    int out_samples = p_sys->out_samples;

    fft_state *p_state = NULL; /* internal FFT data */
    DEFINE_WIND_CONTEXT(wind_ctx); /* internal window data */

    unsigned i;
    float p_output[out_samples];           /* Raw FFT Result  */
    int16_t p_buffer1[out_samples];        /* Buffer on which we perform
                                                  the FFT (first channel) */
    float *p_buffl = (float*)p_sys->data;  /* Original buffer */

    int16_t  *p_buffs;                         /* int16_t converted buffer */
    int16_t  *p_s16_buff;                      /* int16_t converted buffer */

    if (!nb_samples) {
        LOGE("no samples yet...");
        goto release;
    }

    /* Allocate the buffer only if the number of samples change */
    if (nb_samples != p_sys->i_prev_nb_samples)
    {
        free(p_sys->p_prev_s16_buff);
        p_sys->p_prev_s16_buff = (short *) malloc(nb_samples *
                                        p_sys->i_channels *
                                        sizeof(int16_t));
        if (!p_sys->p_prev_s16_buff)
            goto release;
        p_sys->i_prev_nb_samples = nb_samples;
    }
    p_buffs = p_s16_buff = p_sys->p_prev_s16_buff;

    /* Convert the buffer to int16_t */
    for (i = nb_samples * p_sys->i_channels; i--;)
    {
        union {float f; int32_t i;} u;

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
    if (!p_state)
    {
        LOGE("unable to initialize FFT transform...");
        goto release;
    }
    if (!window_init(out_samples, &p_sys->wind_param, &wind_ctx))
    {
        LOGE("unable to initialize FFT window...");
        goto release;
    }
    p_buffs = p_s16_buff;
    for (i = 0 ; i < out_samples; i++)
    {
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
