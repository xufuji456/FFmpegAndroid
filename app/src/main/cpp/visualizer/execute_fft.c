//
// Created by frank on 2021/8/16.
//

#include "execute_fft.h"

#include <android/log.h>
#define LOG_TAG "execute_fft"
#define LOGE(...) ((void)__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, \
                   __VA_ARGS__))

#define SPECTRUM_WIDTH 4.f
#define NB_BANDS 20
#define ROTATION_INCREMENT .1f
#define BAR_DECREMENT .075f
#define ROTATION_MAX 20

static inline void block_CopyProperties(block_t *dst, const block_t *src)
{
    dst->i_dts        = src->i_dts;
    dst->i_pts        = src->i_pts;
    dst->i_flags      = src->i_flags;
    dst->i_length     = src->i_length;
    dst->i_nb_samples = src->i_nb_samples;
}

static inline block_t *block_Duplicate(const block_t *p_block)
{
//    block_t *p_dup = block_Alloc(p_block->i_buffer);//TODO
    block_t *p_dup = (block_t*) malloc(sizeof(block_t));
    if(p_dup == NULL) return NULL;
    p_dup->i_buffer = p_block->i_buffer;
    p_dup->p_buffer = (uint8_t*) malloc(p_block->i_buffer);//TODO

    block_CopyProperties(p_dup, p_block);
    memcpy(p_dup->p_buffer, p_block->p_buffer, p_block->i_buffer);

    return p_dup;
}

/*static*/ int Open(filter_sys_t *p_sys)
{
    if (p_sys == NULL)
        return VLC_ENOMEM;

    /* Create the object for the thread */
    p_sys->i_channels = 2;//TODO
    p_sys->i_prev_nb_samples = 0;
    p_sys->p_prev_s16_buff = NULL;

    p_sys->f_rotationAngle = 0;
    p_sys->f_rotationIncrement = ROTATION_INCREMENT;

    window_param *w_param = (window_param*) malloc(sizeof(window_param));
    p_sys->wind_param = *w_param;//TODO

    /* Fetch the FFT window parameters */
    window_get_param(&p_sys->wind_param);

    /* Create the FIFO for the audio data. */
    vlc_queue_Init(&p_sys->queue, offsetof (block_t, p_next));
    p_sys->dead = false;

    /* Create the thread */
//    if (vlc_clone(&p_sys->thread, Thread, p_filter,
//                  VLC_THREAD_PRIORITY_VIDEO)) {
//        return VLC_ENOMEM;
//    }

    pthread_create (&p_sys->thread, NULL, Thread, p_sys);

    return VLC_SUCCESS;
}

/*static*/ block_t *filter_audio(filter_sys_t *p_sys, block_t *p_in_buf)
{
    vlc_queue_Enqueue(&p_sys->queue, block_Duplicate(p_in_buf));
    return p_in_buf;
}

/*static*/ void Close(filter_sys_t *p_filter)
{
    filter_sys_t *p_sys = p_filter;

    /* Terminate the thread. */
    vlc_queue_Kill(&p_sys->queue, &p_sys->dead);
//    vlc_join(p_sys->thread, NULL);
    pthread_join(p_sys->thread, NULL);

    free(p_sys->p_prev_s16_buff);
}

static void *Thread(void *p_data)
{
    filter_sys_t *p_sys = (filter_sys_t*)p_data;
    block_t *block;

    float height[NB_BANDS] = {0};
    LOGE("start FFT thread...");
    while ((block = vlc_queue_DequeueKillable(&p_sys->queue, &p_sys->dead)))
    {
        LOGE("running FFT transform...");
        unsigned win_width, win_height;

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
            p_sys->p_prev_s16_buff = malloc(block->i_nb_samples *
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

        /* Determine the camera rotation angle. */
        p_sys->f_rotationAngle += p_sys->f_rotationIncrement;
        if (p_sys->f_rotationAngle <= -ROTATION_MAX)
            p_sys->f_rotationIncrement = ROTATION_INCREMENT;
        else if (p_sys->f_rotationAngle >= ROTATION_MAX)
            p_sys->f_rotationIncrement = -ROTATION_INCREMENT;

        /* Render the frame. */
//        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
//        glPushMatrix();
//        glRotatef(p_sys->f_rotationAngle, 0, 1, 0);
//        drawBars(height);
//        glPopMatrix();

        /* Wait to swapp the frame on time. */
//        vlc_tick_wait(block->i_pts + (block->i_length / 2));
//        vlc_gl_Swap(gl);
        usleep(block->i_pts + (block->i_length / 2));
        LOGE("height[0]=%f, height[1]=%f, height=[2]=%f", height[0], height[1], height[2]);

release:
        window_close(&wind_ctx);
        fft_close(p_state);
//        block_Release(block);//TODO
        if (block->p_buffer) free(block->p_buffer);
        free(block);
    }

    return NULL;
}
