//
// Created by frank on 2021/8/16.
//

#ifndef EXECUTE_FFT_H
#define EXECUTE_FFT_H

#include <math.h>
#include <unistd.h>
#include "fft.h"
#include "window.h"
#include "fixed_fft.h"

#ifdef __cplusplus
extern "C" {
#endif

#include "block_queue.h"

#define MIN_FFT_SIZE 128

typedef struct
{
    pthread_t thread;
    vlc_queue_t queue;
    bool dead;
    int i_channels;
    int i_prev_nb_samples;
    int16_t *p_prev_s16_buff;

    /* FFT window parameters */
    window_param wind_param;

    uint8_t *data;
    int data_size;
    int nb_samples;
    int8_t *output;
    int out_samples;
} filter_sys_t;

static void *fft_thread(void *);

int open_visualizer(filter_sys_t *p_sys);

block_t *filter_audio(filter_sys_t *p_sys, void *p_in_buf);

void close_visualizer(filter_sys_t *p_filter);

void fft_once(filter_sys_t *p_sys);

int init_visualizer(filter_sys_t *p_sys);

void release_visualizer(filter_sys_t *p_sys);

int ensure_memory(filter_sys_t *fft_filter, int nb_samples);

void fft_fixed(filter_sys_t *p_sys);

#ifdef __cplusplus
}
#endif

#endif //EXECUTE_FFT_H
