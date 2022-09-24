
#include "FFMessageQueue.h"

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

void FFMessageQueue::init() {
    memset(q, 0, sizeof(MessageQueue));
    q->abort_request = 1;
}

void FFMessageQueue::start() {
    std::lock_guard<std::mutex> lock(q->mutex);
    q->abort_request = 0;

    AVMessage msg;
    msg_init_msg(&msg);
    msg.what = FFP_MSG_FLUSH;
    msg_queue_put_private(q, &msg);
}

void FFMessageQueue::flush() {
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

void FFMessageQueue::sendMessage1(int what) {
    AVMessage msg;
    msg_init_msg(&msg);
    msg.what = what;
    msg_queue_put(q, &msg);
}

void FFMessageQueue::sendMessage2(int what, int arg1) {
    AVMessage msg;
    msg_init_msg(&msg);
    msg.what = what;
    msg.arg1 = arg1;
    msg_queue_put(q, &msg);
}

void FFMessageQueue::sendMessage3(int what, int arg1, int arg2) {
    AVMessage msg;
    msg_init_msg(&msg);
    msg.what = what;
    msg.arg1 = arg1;
    msg.arg2 = arg2;
    msg_queue_put(q, &msg);
}

int FFMessageQueue::get(AVMessage *msg, int block) {
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

void FFMessageQueue::remove(int what) {
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

void FFMessageQueue::abort() {
    std::unique_lock<std::mutex> lock(q->mutex);
    q->abort_request = 1;
    q->cond.notify_all();
}

void FFMessageQueue::destroy() {
    flush();
    std::lock_guard<std::mutex> lock(q->mutex);

    while(q->recycle_msg) {
        AVMessage *msg = q->recycle_msg;
        if (msg)
            q->recycle_msg = msg->next;
        msg_free(msg);
        av_freep(&msg);
    }
}