
#ifndef FFMSG_QUEUE_H
#define FFMSG_QUEUE_H

#include <mutex>
#include <libavutil/mem.h>

/**************** msg  define begin ***************/

#define FFP_MSG_FLUSH                       0
#define FFP_MSG_ERROR                       100
#define FFP_MSG_PREPARED                    200
#define FFP_MSG_COMPLETED                   300
#define FFP_MSG_VIDEO_SIZE_CHANGED          400
#define FFP_MSG_SAR_CHANGED                 401
#define FFP_MSG_VIDEO_RENDERING_START       402
#define FFP_MSG_AUDIO_RENDERING_START       403
#define FFP_MSG_VIDEO_ROTATION_CHANGED      404
#define FFP_MSG_AUDIO_DECODED_START         405
#define FFP_MSG_VIDEO_DECODED_START         406
#define FFP_MSG_OPEN_INPUT                  407
#define FFP_MSG_FIND_STREAM_INFO            408
#define FFP_MSG_COMPONENT_OPEN              409
#define FFP_MSG_VIDEO_SEEK_RENDERING_START  410
#define FFP_MSG_AUDIO_SEEK_RENDERING_START  411

#define FFP_MSG_BUFFERING_START             500
#define FFP_MSG_BUFFERING_END               501
#define FFP_MSG_BUFFERING_UPDATE            502
#define FFP_MSG_BUFFERING_BYTES_UPDATE      503
#define FFP_MSG_BUFFERING_TIME_UPDATE       504
#define FFP_MSG_SEEK_COMPLETE               600
#define FFP_MSG_PLAYBACK_STATE_CHANGED      700
#define FFP_MSG_TIMED_TEXT                  800
#define FFP_MSG_VIDEO_DECODER_OPEN          900

#define FFP_REQ_START                       1001
#define FFP_REQ_PAUSE                       1002
#define FFP_REQ_SEEK                        1003

/**************** msg  define end *****************/

typedef struct AVMessage {
    int what;
    int arg1;
    int arg2;
    void *obj;
    void (*free_l)(void *obj);
    struct AVMessage *next;
} AVMessage;

typedef struct MessageQueue {
    AVMessage *first_msg, *last_msg;
    int nb_messages;
    int abort_request;
    std::mutex mutex;
    std::condition_variable cond;

    AVMessage *recycle_msg;
    int recycle_count;
    int alloc_count;
} MessageQueue;

inline static void msg_free(AVMessage *msg) {
    if (!msg || !msg->obj)
        return;
    assert(msg->free_l);
    msg->free_l(msg->obj);
    msg->obj = nullptr;
}

inline static int msg_queue_put_private(MessageQueue *q, AVMessage *msg) {
    AVMessage *msg1;

    if (q->abort_request)
        return -1;

    msg1 = q->recycle_msg;
    if (msg1) {
        q->recycle_msg = msg1->next;
        q->recycle_count++;
    } else {
        q->alloc_count++;
        msg1 = static_cast<AVMessage *>(av_malloc(sizeof(AVMessage)));
    }

    if (!msg1)
        return -1;

    *msg1 = *msg;
    msg1->next = nullptr;

    if (!q->last_msg)
        q->first_msg = msg1;
    else
        q->last_msg->next = msg1;
    q->last_msg = msg1;
    q->nb_messages++;
    q->cond.notify_all();
    return 0;
}

inline static int msg_queue_put(MessageQueue *q, AVMessage *msg) {
    int ret;

    std::lock_guard<std::mutex> lock(q->mutex);
    ret = msg_queue_put_private(q, msg);

    return ret;
}

inline static void msg_init_msg(AVMessage *msg) {
    memset(msg, 0, sizeof(AVMessage));
}

inline static void msg_queue_put_notify1(MessageQueue *q, int what) {
    AVMessage msg;
    msg_init_msg(&msg);
    msg.what = what;
    msg_queue_put(q, &msg);
}

inline static void msg_queue_put_notify2(MessageQueue *q, int what, int arg1) {
    AVMessage msg;
    msg_init_msg(&msg);
    msg.what = what;
    msg.arg1 = arg1;
    msg_queue_put(q, &msg);
}

inline static void msg_queue_put_notify3(MessageQueue *q, int what, int arg1, int arg2) {
    AVMessage msg;
    msg_init_msg(&msg);
    msg.what = what;
    msg.arg1 = arg1;
    msg.arg2 = arg2;
    msg_queue_put(q, &msg);
}

inline static void msg_queue_init(MessageQueue *q) {
    memset(q, 0, sizeof(MessageQueue));
    q->abort_request = 1;
}

inline static void msg_queue_flush(MessageQueue *q) {
    AVMessage *msg, *msg1;
    std::lock_guard<std::mutex> lock(q->mutex);

    for (msg = q->first_msg; msg != NULL; msg = msg1) {
        msg1 = msg->next;
        msg->next = q->recycle_msg;
        q->recycle_msg = msg;
    }
    q->last_msg = nullptr;
    q->first_msg = nullptr;
    q->nb_messages = 0;
}

inline static void msg_queue_abort(MessageQueue *q) {
    std::unique_lock<std::mutex> lock(q->mutex);
    q->abort_request = 1;
    q->cond.notify_all();
}

inline static void msg_queue_start(MessageQueue *q) {
    std::lock_guard<std::mutex> lock(q->mutex);
    q->abort_request = 0;

    AVMessage msg;
    msg_init_msg(&msg);
    msg.what = FFP_MSG_FLUSH;
    msg_queue_put_private(q, &msg);
}

inline static int msg_queue_get(MessageQueue *q, AVMessage *msg, int block) {
    AVMessage *msg1;
    int ret;

    std::unique_lock<std::mutex> lock(q->mutex);

    for (;;) {
        if (q->abort_request) {
            ret = -1;
            break;
        }

        msg1 = q->first_msg;
        if (msg1) {
            q->first_msg = msg1->next;
            if (!q->first_msg)
                q->last_msg = nullptr;
            q->nb_messages--;
            *msg = *msg1;
            msg1->obj = nullptr;
            msg1->next = q->recycle_msg;
            q->recycle_msg = msg1;
            ret = 1;
            break;
        } else if (!block) {
            ret = 0;
            break;
        } else {
            q->cond.wait(lock);
        }
    }
    lock.unlock();
    return ret;
}

inline static void msg_queue_remove(MessageQueue *q, int what) {
    AVMessage **p_msg, *msg, *last_msg;
    std::lock_guard<std::mutex> lock(q->mutex);

    last_msg = q->first_msg;

    if (!q->abort_request && q->first_msg) {
        p_msg = &q->first_msg;
        while (*p_msg) {
            msg = *p_msg;

            if (msg->what == what) {
                *p_msg = msg->next;
                msg_free(msg);
                msg->next = q->recycle_msg;
                q->recycle_msg = msg;
                q->nb_messages--;
            } else {
                last_msg = msg;
                p_msg = &msg->next;
            }
        }

        if (q->first_msg) {
            q->last_msg = last_msg;
        } else {
            q->last_msg = nullptr;
        }
    }
}

inline static void msg_queue_destroy(MessageQueue *q) {
    msg_queue_flush(q);
    std::lock_guard<std::mutex> lock(q->mutex);

    while(q->recycle_msg) {
        AVMessage *msg = q->recycle_msg;
        if (msg)
            q->recycle_msg = msg->next;
        msg_free(msg);
        av_freep(&msg);
    }
}

#endif // FFMSG_QUEUE_H
