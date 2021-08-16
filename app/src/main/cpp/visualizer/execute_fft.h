//
// Created by frank on 2021/8/16.
//

#ifndef PLAYER_CORE_RUN_FFT_H
#define PLAYER_CORE_RUN_FFT_H

#include "vlc_queue.h"
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

typedef int64_t vlc_tick_t;

typedef struct block_t block_t;

struct vlc_block_callbacks
{
    void (*free)(block_t *);
};

struct block_t
{
    block_t    *p_next;

    uint8_t    *p_buffer; /**< Payload start */
    size_t      i_buffer; /**< Payload length */
    uint8_t    *p_start; /**< Buffer start */
    size_t      i_size; /**< Buffer total size */

    uint32_t    i_flags;
    unsigned    i_nb_samples; /* Used for audio */

    vlc_tick_t  i_pts;
    vlc_tick_t  i_dts;
    vlc_tick_t  i_length;

    const struct vlc_block_callbacks *cbs;
};

typedef struct
{
//    vlc_thread_t thread;
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

static void *Thread(void *);

/*static*/ int Open(filter_sys_t *p_sys);

/*static*/ block_t *filter_audio(filter_sys_t *p_sys, block_t *p_in_buf);

/*static*/ void Close(filter_sys_t *p_filter);

#endif //PLAYER_CORE_RUN_FFT_H
