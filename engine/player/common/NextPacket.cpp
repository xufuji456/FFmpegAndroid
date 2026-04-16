/**
 * Note: custom of AVPacket
 * Date: 2026/4/16
 * Author: frank
 */

#include "NextPacket.h"

#include "CommonUtil.h"
#include "NalUnitParser.h"


static bool IsIdrPacket(AVPacket *pkt, bool is_hevc) {
    if (pkt && pkt->data && pkt->size >= 5) {
        uint32_t offset = 0;
        while (offset >= 0 && offset + 5 <= pkt->size) {
            auto *nal_start = reinterpret_cast<uint8_t *>(pkt->data + offset);
            if (is_hevc) {
                int type = NALUnitParser::get_hevc_nal_unit_type((const uint8_t *const) nal_start);
                if (NALUnitParser::is_hevc_idr(type)) {
                    return true;
                }
            } else {
                int type = NALUnitParser::get_h264_nal_unit_type((const uint8_t *const) nal_start);
                if (NALUnitParser::is_h264_idr(type)) {
                    return true;
                }
            }
            offset += (CommonUtil::byteToInt(nal_start) + 4);
        }
    }
    return false;
}

NextPacket::NextPacket(AVPacket *pkt) {
    mPkt = av_packet_alloc();
    av_init_packet(mPkt);
    av_packet_ref(mPkt, pkt);
}

NextPacket::NextPacket(AVPacket *pkt, int serial) : NextPacket(pkt) {
    mSerial = serial;
}

NextPacket::NextPacket(PacketOpType type) : mPktType(type) {}

NextPacket::~NextPacket() {
    if (mPkt) {
        av_packet_free(&mPkt);
    }
}

AVPacket *NextPacket::GetPacket() {
    return mPkt;
}

int NextPacket::GetSerial() const {
    return mSerial;
}

bool NextPacket::IsKeyPacket() {
    return mPkt && mPkt->flags & AV_PKT_FLAG_KEY;
}

bool NextPacket::IsFlushPacket() {
    return mPktType == PKT_OP_TYPE_FLUSH;
}

bool NextPacket::IsEofPacket() {
    return mPktType == PKT_OP_TYPE_EOF;
}

bool NextPacket::IsKeyOrIdrPacket(bool is_idr, bool is_hevc) {
    if (is_idr) {
        return IsIdrPacket(mPkt, is_hevc);
    } else {
        return IsKeyPacket();
    }
}
