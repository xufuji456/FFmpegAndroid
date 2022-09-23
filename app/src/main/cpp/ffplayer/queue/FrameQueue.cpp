
#include "FrameQueue.h"

FrameQueue::FrameQueue(int max_size, int keep_last):m_size(0),
                                                    m_show_idx(0),
                                                    m_read_idx(0),
                                                    m_write_idx(0),
                                                    m_keep_last(keep_last),
                                                    m_abort_request(0) {
    m_max_size  = FFMIN(max_size, FRAME_QUEUE_SIZE);
    memset(m_queue, 0, sizeof(Frame) * FRAME_QUEUE_SIZE);

    for (int i = 0; i < this->m_max_size; ++i) {
        m_queue[i].frame = av_frame_alloc();
    }
}

void FrameQueue::start() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_abort_request = 0;
}

void FrameQueue::abort() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_abort_request = 1;
}

Frame *FrameQueue::currentFrame() {
    return &m_queue[(m_read_idx + m_show_idx) % m_max_size];
}

Frame *FrameQueue::nextFrame() {
    return &m_queue[(m_read_idx + m_show_idx + 1) % m_max_size];
}

Frame *FrameQueue::lastFrame() {
    return &m_queue[m_read_idx];
}

Frame *FrameQueue::peekWritable() {
    if (m_abort_request) {
        return nullptr;
    }

    return &m_queue[m_write_idx];
}

void FrameQueue::pushFrame() {
    std::unique_lock<std::mutex> lock(m_mutex);
    if (++m_write_idx == m_max_size) {
        m_write_idx = 0;
    }
    m_size++;
}

void FrameQueue::popFrame() {
    std::unique_lock<std::mutex> lock(m_mutex);
    if (m_keep_last && !m_show_idx) {
        m_show_idx = 1;
        return;
    }
    unrefFrame(&m_queue[m_read_idx]);
    if (++m_read_idx == m_max_size) {
        m_read_idx = 0;
    }
    m_size--;
}

void FrameQueue::flush() {
    while (getFrameSize() > 0) {
        popFrame();
    }
}

int FrameQueue::getFrameSize() {
    return m_size - m_show_idx;
}

int FrameQueue::getShowIndex() const {
    return m_show_idx;
}

void FrameQueue::unrefFrame(Frame *vp) {
    av_frame_unref(vp->frame);
}

FrameQueue::~FrameQueue() {
    for (int i = 0; i < m_max_size; ++i) {
        Frame *vp = &m_queue[i];
        unrefFrame(vp);
        av_frame_free(&vp->frame);
    }
}