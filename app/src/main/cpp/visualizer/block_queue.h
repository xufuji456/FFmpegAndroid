//
// Created by frank on 2021/08/08.
//

#ifndef BLOCK_QUEUE_H
#define BLOCK_QUEUE_H

#include <pthread.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef int64_t vlc_tick_t;

typedef struct FFT_callback {
    void (*callback)(void*);
} FFT_callback;

typedef struct block_t {
    uint8_t    *p_buffer; /** Payload start */

    unsigned    i_nb_samples; /** Used for audio */

    vlc_tick_t  i_pts;
    vlc_tick_t  i_length;

    FFT_callback fft_callback;
} block_t;

typedef struct vlc_queue {
    //the size of queue
    int size;
    //packet array
    void **packets;
    //the packet next to write
    int next_to_write;
    //the packet next to read
    int next_to_read;

    pthread_mutex_t mutex;
    pthread_cond_t cond;
} vlc_queue_t;

vlc_queue_t *vlc_queue_init(int size);

void vlc_queue_free(vlc_queue_t *queue);

void *vlc_queue_push(vlc_queue_t *queue, void *data);

void *vlc_queue_pop(vlc_queue_t *queue);

#endif //BLOCK_QUEUE_H

#ifdef __cplusplus
}
#endif