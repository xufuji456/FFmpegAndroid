/**
 * Note: Parser of NALUnit
 * Date: 2025/11/28
 * Author: frank
 */

#ifndef NAL_UNIT_PARSER_H
#define NAL_UNIT_PARSER_H

#include "NextDefine.h"

/** H264 NAL结构
---------------------------------
|0|1|2|3|4|5|6|7|
---------------------------------
|F|Idc|  Type   |
*/

/** HEVC NAL结构
---------------------------------
|0|1|2|3|4|5|6|7|0|1|2|3|4|5|6|7|
---------------------------------
|F|    Type   |  LayerId  | Tid |
*/

class NALUnitParser {
public:
    // 获取h264的nal类型
    static inline int get_h264_nal_unit_type(const uint8_t *data) {
        return data[4] & 0x1F;
    }

    // 获取hevc的nal类型
    static inline int get_hevc_nal_unit_type(const uint8_t *data) {
        return (data[4] & 0x7E) >> 1;
    }

    // 判断h264是否idr帧
    static inline int is_h264_idr(int type) {
        return type == NAL_IDR_SLICE;
    }

    // 判断hevc是否idr帧
    static inline int is_hevc_idr(int type) {
        return type == HEVC_NAL_BLA_W_LP ||
               type == HEVC_NAL_BLA_W_RADL ||
               type == HEVC_NAL_BLA_N_LP ||
               type == HEVC_NAL_IDR_W_RADL ||
               type == HEVC_NAL_IDR_N_LP ||
               type == HEVC_NAL_CRA_NUT;
    }

    // 判断hevc是否为非参考帧
    static inline bool is_hevc_no_ref(int type) {
        return type == HEVC_NAL_TRAIL_N ||
               type == HEVC_NAL_TSA_N ||
               type == HEVC_NAL_STSA_N ||
               type == HEVC_NAL_RADL_N ||
               type == HEVC_NAL_RASL_N ||
               type == HEVC_NAL_VCL_N10 ||
               type == HEVC_NAL_VCL_N12 ||
               type == HEVC_NAL_VCL_N14;
    }

    // 获取h264的ref_idc
    static inline int get_h264_ref_idc(const uint8_t *data) {
        return (data[4] >> 5) & 0x03;
    }

    // 获取hevc的编码层级nuh_layer_id
    static inline int get_hevc_nuh_layer_id(const uint8_t *data) {
        return (data[4] & 0x1 << 5) + (data[5] >> 3) & 0x1F;
    }

};

#endif //NAL_UNIT_PARSER_H
