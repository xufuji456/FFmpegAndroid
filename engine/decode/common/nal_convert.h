#ifndef NAL_CONVERT_H
#define NAL_CONVERT_H

#include <stdint.h>

typedef struct H2645ConvertState {
    uint32_t nal_len;
    uint32_t nal_pos;
}H2645ConvertState;

int convert_sps_pps(const uint8_t *p_in_buf, size_t in_buf_size,
                           uint8_t *p_out_buf, size_t out_buf_size,
                           size_t *sps_pps_size, size_t *nal_size);

int convert_hevc_nal_units(const uint8_t *p_in_buf, size_t in_buf_size,
                                  uint8_t *p_out_buf, size_t out_buf_size,
                                  size_t *sps_pps_size, size_t *nal_size);

void convert_h2645_to_annexb(uint8_t *p_buf, size_t len,
                                    size_t nal_size,
                                    H2645ConvertState *state);

void convert_avcc_to_annexb(uint8_t *data, size_t size);

#endif
