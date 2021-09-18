//
// Created by frank on 2021/8/16.
//

#ifndef EXECUTE_FFT_H
#define EXECUTE_FFT_H

#include "block_queue.h"
#include "fft.h"
#include "window.h"

#include <math.h>
#include <unistd.h>

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

    uint8_t *data;
    int nb_samples;
    int16_t * output;
    int out_samples;
} filter_sys_t;

static void *fft_thread(void *);

int open_visualizer(filter_sys_t *p_sys);

block_t *filter_audio(filter_sys_t *p_sys, void *p_in_buf);

void close_visualizer(filter_sys_t *p_filter);

void fft_once(filter_sys_t *p_sys);

int init_visualizer(filter_sys_t *p_sys);

void release_visualizer(filter_sys_t *p_sys);

#endif //EXECUTE_FFT_H
