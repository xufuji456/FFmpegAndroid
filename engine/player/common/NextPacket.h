#ifndef NEXT_PACKET_H
#define NEXT_PACKET_H

#ifdef __cplusplus
extern "C" {
#endif
#include "libavcodec/packet.h"
#ifdef __cplusplus
}
#endif

enum PacketOpType {
    PKT_OP_TYPE_DEFAULT = 0,
    PKT_OP_TYPE_FLUSH   = 1,
    PKT_OP_TYPE_EOF     = 2
};

class NextPacket {
public:
    explicit NextPacket(AVPacket *pkt);

    NextPacket(AVPacket *pkt, int serial);

    explicit NextPacket(PacketOpType type);

    ~NextPacket();

    AVPacket *GetPacket();

    int GetSerial() const;

    bool IsKeyPacket();

    bool IsFlushPacket();

    bool IsEofPacket();

    bool IsKeyOrIdrPacket(bool is_idr, bool is_hevc);

    NextPacket(const NextPacket &) = delete;

    NextPacket(NextPacket &&) = delete;

    NextPacket &operator=(const NextPacket &) = delete;

    NextPacket &operator=(NextPacket &&) = delete;

private:
    int mSerial           = 0;
    AVPacket *mPkt        = nullptr;
    PacketOpType mPktType = PKT_OP_TYPE_DEFAULT;
};

#endif //NEXT_PACKET_H
