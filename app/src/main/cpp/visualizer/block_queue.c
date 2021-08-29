//
// Created by frank on 2021/08/08.
//
#include "block_queue.h"
#include <stdlib.h>

vlc_queue_t *vlc_queue_init(int size) {
    vlc_queue_t *queue = (vlc_queue_t *)(malloc(sizeof(vlc_queue_t)));
    queue->size = size;
    queue->next_to_read = 0;
    queue->next_to_write = 0;
    int i;
    queue->packets = (void **)(malloc(sizeof(*queue->packets) * size));
    for (i = 0; i < size; i++) {
        block_t *packet = malloc(sizeof(block_t));
        packet->i_nb_samples = 0;
        queue->packets[i] = packet;
    }
    pthread_mutex_init(&queue->mutex, NULL);
    pthread_cond_init(&queue->cond, NULL);
    return queue;
}

void vlc_queue_free(vlc_queue_t *queue) {
    int i;
    for (i = 0; i < queue->size; i++) {
        free(queue->packets[i]);
    }
    free(queue->packets);
    free(queue);
    pthread_cond_destroy(&queue->cond);
    pthread_mutex_destroy(&queue->mutex);
}

int vlc_queue_next(vlc_queue_t *queue, int current) {
    return (current + 1) % queue->size;
}

void *vlc_queue_push(vlc_queue_t *queue, void *data) {
    pthread_mutex_lock(&queue->mutex);//lock
    int current = queue->next_to_write;
    int next_to_write;
    for (;;) {
        next_to_write = vlc_queue_next(queue, current);
        if (next_to_write != queue->next_to_read) {
            break;
        }
        pthread_cond_wait(&queue->cond, &queue->mutex);
    }
    queue->next_to_write = next_to_write;
    queue->packets[current] = data;//TODO
    pthread_cond_broadcast(&queue->cond);
    pthread_mutex_unlock(&queue->mutex);//unlock
    return queue->packets[current];
}

void *vlc_queue_pop(vlc_queue_t *queue) {
    pthread_mutex_lock(&queue->mutex);
    int current = queue->next_to_read;
    for (;;) {
        if (queue->next_to_write != queue->next_to_read) {
            break;
        }
        pthread_cond_wait(&queue->cond, &queue->mutex);
    }
    queue->next_to_read = vlc_queue_next(queue, current);
    pthread_cond_broadcast(&queue->cond);
    pthread_mutex_unlock(&queue->mutex);
    return queue->packets[current];
}