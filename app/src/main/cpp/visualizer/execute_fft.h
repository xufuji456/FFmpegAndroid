//
// Created by frank on 2021/8/16.
//

#ifndef PLAYER_CORE_RUN_FFT_H
#define PLAYER_CORE_RUN_FFT_H

#include "block_queue.h"
#include "fft.h"
#include "window.h"

#include <math.h>
#include <unistd.h>

/** No error */
#define VLC_SUCCESS        (-0)
/** Unspecified error */
#define VLC_EGENERIC       (-1)
/** Not enough memory */
#define VLC_ENOMEM         (-2)

typedef struct
{
    pthread_t thread;

    /* Audio data */
    vlc_queue_t queue;
    bool dead;
    unsigned i_channels;
    unsigned i_prev_nb_samples;
    int16_t *p_prev_s16_buff;

    float f_rotationAngle;
    float f_rotationIncrement;

    /* FFT window parameters */
    window_param wind_param;
} filter_sys_t;

static void *fft_thread(void *);

int open_visualizer(filter_sys_t *p_sys);

block_t *filter_audio(filter_sys_t *p_sys, void *p_in_buf);

void close_visualizer(filter_sys_t *p_filter);

void fft_once(void *p_data, block_t *block, int16_t *output);

int init_visualizer(filter_sys_t *p_sys);

void release_visualizer(filter_sys_t *p_sys);

#endif //PLAYER_CORE_RUN_FFT_H
