//
// Created by frank on 2018/2/3.
//

#ifndef VIDEOPLAYER_AVPACKET_QUEUE_H
#define VIDEOPLAYER_AVPACKET_QUEUE_H
#include <pthread.h>

typedef struct AVPacketQueue{
    //队列大小
    int size;
    //指针数组
    void ** packets;
    //下一个写入的packet
    int next_to_write;
    //下一个读取的packet
    int next_to_read;
}AVPacketQueue;

AVPacketQueue* queue_init(int size);
void queue_free(AVPacketQueue *queue);
void* queue_push(AVPacketQueue* queue, pthread_mutex_t* mutex, pthread_cond_t* cond);
void* queue_pop(AVPacketQueue* queue, pthread_mutex_t* mutex, pthread_cond_t* cond);

#endif //VIDEOPLAYER_AVPACKET_QUEUE_H