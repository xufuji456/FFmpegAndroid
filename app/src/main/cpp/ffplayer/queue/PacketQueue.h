
#ifndef FFMPEGANDROID_PACKETQUEUE_H
#define FFMPEGANDROID_PACKETQUEUE_H


#include <queue>
#include <mutex>

extern "C" {
#include <libavcodec/avcodec.h>
}

typedef struct PacketList {
    AVPacket pkt;
    struct PacketList *next;
} PacketList;

class PacketQueue {

private:
    std::mutex m_mutex;
    std::condition_variable m_cond;

    int m_size;
    int m_nb_packets;
    int64_t m_duration;
    int m_abort_request;
    PacketList *first_pkt_list;
    PacketList *last_pkt_list;

    int put(AVPacket *pkt);

public:

    PacketQueue();

    int pushPacket(AVPacket *pkt);

    int pushEmptyPacket(int stream_idx);

    int popPacket(AVPacket *pkt);

    void flush();

    void start();

    void abort();

    int getSize();

    int getPacketSize();

    int64_t getDuration();

    int isAbortRequest();

    virtual ~PacketQueue();

};


#endif //FFMPEGANDROID_PACKETQUEUE_H
