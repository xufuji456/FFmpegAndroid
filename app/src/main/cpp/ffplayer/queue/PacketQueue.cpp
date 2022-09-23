
#include "PacketQueue.h"

PacketQueue::PacketQueue():m_size(0),
                           m_duration(0),
                           m_nb_packets(0),
                           m_abort_request(0),
                           last_pkt_list(nullptr),
                           first_pkt_list(nullptr) {
}

int PacketQueue::put(AVPacket *pkt) {
    PacketList *pkt1;

    if (m_abort_request) {
        return -1;
    }

    pkt1 = (PacketList *) av_malloc(sizeof(PacketList));
    if (!pkt1) {
        return -1;
    }
    pkt1->pkt  = *pkt;
    pkt1->next = nullptr;

    if (!last_pkt_list) {
        first_pkt_list = pkt1;
    } else {
        last_pkt_list->next = pkt1;
    }

    m_nb_packets++;
    last_pkt_list = pkt1;
    m_size += pkt1->pkt.size + sizeof(*pkt1);
    m_duration += pkt1->pkt.duration;
    return 0;
}

int PacketQueue::pushPacket(AVPacket *pkt) {
    std::unique_lock<std::mutex> lock(m_mutex);
    int ret = put(pkt);
    if (ret < 0) {
        av_packet_unref(pkt);
    }
    m_cond.notify_all();

    return ret;
}

int PacketQueue::pushEmptyPacket(int stream_idx) {
    AVPacket pkt1;
    AVPacket*pkt = &pkt1;
    av_init_packet(pkt);

    pkt->size         = 0;
    pkt->data         = nullptr;
    pkt->stream_index = stream_idx;

    return pushPacket(pkt);
}

int PacketQueue::popPacket(AVPacket *pkt) {
    std::unique_lock<std::mutex> lock(m_mutex);
    int ret;
    PacketList *pkt1;

    for (;;) {
        if (m_abort_request) {
            ret = -1;
            break;
        }

        pkt1 = first_pkt_list;
        if (pkt1) {
            first_pkt_list = pkt1->next;
            if (!first_pkt_list) {
                last_pkt_list = nullptr;
            }
            m_nb_packets--;
            m_size -= pkt1->pkt.size + sizeof(*pkt1);
            m_duration -= pkt1->pkt.duration;
            *pkt = pkt1->pkt;
            av_free(pkt1);
            ret = 1;
            break;
        } else {
            m_cond.wait(lock);
        }
    }
    return ret;
}

void PacketQueue::flush() {
    std::unique_lock<std::mutex> lock(m_mutex);
    PacketList *pkt, *pkt1;

    for (pkt = first_pkt_list; pkt; pkt = pkt1) {
        pkt1 = pkt->next;
        av_packet_unref(&pkt->pkt);
        av_freep(&pkt);
    }

    m_size = 0;
    m_duration = 0;
    m_nb_packets = 0;
    last_pkt_list = nullptr;
    first_pkt_list = nullptr;
    m_cond.notify_all();
}

void PacketQueue::start() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_abort_request = 0;
}

void PacketQueue::abort() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_abort_request = 1;
}

int PacketQueue::getSize() {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_size;
}

int PacketQueue::getPacketSize() {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_nb_packets;
}

int64_t PacketQueue::getDuration() {
    return m_duration;
}

int PacketQueue::isAbortRequest() {
    return m_abort_request;
}

PacketQueue::~PacketQueue() {
    abort();
    flush();
}