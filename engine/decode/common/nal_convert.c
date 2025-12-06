/**
 * Note: converter of NAL unit
 * Date: 2025/12/6
 * Author: frank
 */

#include "nal_convert.h"

#include <limits.h>
#include <string.h>

int convert_sps_pps(const uint8_t *p_in_buf, size_t in_buf_size,
                    uint8_t *p_out_buf, size_t out_buf_size,
                    size_t *sps_pps_size, size_t *nal_size)
{
    uint32_t m_data_size = in_buf_size;
    uint32_t m_nal_size = 0;
    uint32_t m_sps_pps_size = 0;
    uint32_t m_len = 0;

    if (m_data_size < 7) {
        return -1;
    }

    if (nal_size)
        *nal_size = (p_in_buf[4] & 0x03) + 1;
    p_in_buf += 5;
    m_data_size -= 5;

    for (unsigned int j = 0; j < 2; j++) {
        if (m_data_size < 1) {
            return -1;
        }
        m_len = p_in_buf[0] & (j == 0 ? 0x1f : 0xff);
        p_in_buf++;
        m_data_size--;

        for (unsigned int i = 0; i < m_len; i++) {
            if (m_data_size < 2) {
                return -1;
            }

            m_nal_size = (p_in_buf[0] << 8) | p_in_buf[1];
            p_in_buf += 2;
            m_data_size -= 2;

            if (m_data_size < m_nal_size) {
                return -1;
            }
            if (m_sps_pps_size + 4 + m_nal_size > out_buf_size) {
                return -1;
            }

            p_out_buf[m_sps_pps_size++] = 0;
            p_out_buf[m_sps_pps_size++] = 0;
            p_out_buf[m_sps_pps_size++] = 0;
            p_out_buf[m_sps_pps_size++] = 1;

            memcpy(p_out_buf + m_sps_pps_size, p_in_buf, m_nal_size);
            m_sps_pps_size += m_nal_size;

            p_in_buf += m_nal_size;
            m_data_size -= m_nal_size;
        }
    }

    *sps_pps_size = m_sps_pps_size;

    return 0;
}

int convert_hevc_nal_units(const uint8_t *p_in_buf, size_t in_buf_size,
                           uint8_t *p_out_buf, size_t out_buf_size,
                           size_t *sps_pps_size, size_t *nal_size)
{
    uint32_t m_sps_pps_size = 0;
    const uint8_t *p_end = p_in_buf + in_buf_size;

    if (in_buf_size <= 3 || (!p_in_buf[0] && !p_in_buf[1] && p_in_buf[2] <= 1))
        return -1;

    if (p_end - p_in_buf < 23) {
        return -1;
    }

    p_in_buf += 21;

    if (nal_size)
        *nal_size = (*p_in_buf & 0x03) + 1;
    p_in_buf++;

    int buf_size = *p_in_buf++;

    for (int i = 0; i < buf_size; i++) {
        int type, cnt, j;

        if (p_end - p_in_buf < 3) {
            return -1;
        }
        type = *(p_in_buf++) & 0x3f;
        (void) (type);

        cnt = p_in_buf[0] << 8 | p_in_buf[1];
        p_in_buf += 2;

        for (j = 0; j < cnt; j++) {
            int i_nal_size;

            if (p_end - p_in_buf < 2) {
                return -1;
            }

            i_nal_size = p_in_buf[0] << 8 | p_in_buf[1];
            p_in_buf += 2;

            if (i_nal_size < 0 || p_end - p_in_buf < i_nal_size) {
                return -1;
            }

            if (m_sps_pps_size + 4 + i_nal_size > out_buf_size) {
                return -1;
            }

            p_out_buf[m_sps_pps_size++] = 0;
            p_out_buf[m_sps_pps_size++] = 0;
            p_out_buf[m_sps_pps_size++] = 0;
            p_out_buf[m_sps_pps_size++] = 1;

            memcpy(p_out_buf + m_sps_pps_size, p_in_buf, i_nal_size);
            p_in_buf += i_nal_size;

            m_sps_pps_size += i_nal_size;
        }
    }

    *sps_pps_size = m_sps_pps_size;

    return 0;
}

void convert_h2645_to_annexb(uint8_t *p_buf, size_t len,
                             size_t nal_size, H2645ConvertState *state)
{
    if (nal_size < 3 || nal_size > 4)
        return;

    while (len > 0) {
        if (state->nal_pos < nal_size) {
            unsigned int i;
            for (i = 0; state->nal_pos < nal_size && i < len;
                 i++, state->nal_pos++) {
                state->nal_len = (state->nal_len << 8) | p_buf[i];
                p_buf[i] = 0;
            }
            if (state->nal_pos < nal_size)
                return;
            p_buf[i - 1] = 1;
            p_buf += i;
            len -= i;
        }
        if (state->nal_len > INT_MAX)
            return;
        if (state->nal_len > len) {
            state->nal_len -= len;
            return;
        } else {
            p_buf += state->nal_len;
            len -= state->nal_len;
            state->nal_len = 0;
            state->nal_pos = 0;
        }
    }
}

void convert_avcc_to_annexb(uint8_t *data, size_t size)
{
    static uint8_t start_code[4] = {0x00, 0x00, 0x00, 0x01};
    uint8_t *head = data;
    while (data < head + size) {
        int nal_len = (data[0] << 24) | (data[1] << 16) | (data[2] << 8) | data[3];
        if (nal_len > 0) {
            memcpy(data, start_code, 4);
        }
        data = data + 4 + nal_len;
    }
}
