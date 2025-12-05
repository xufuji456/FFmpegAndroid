/**
 * Note: parser of H26x series
 * Date: 2025/12/5
 * Author: frank
 */

#include "h26x_parser.h"

#include <stdlib.h>

#define AV_RB16(x)                                                             \
  ((((const uint8_t *)(x))[0] << 8) | ((const uint8_t *)(x))[1])

#define AV_RB24(x)                                                             \
  ((((const uint8_t *)(x))[0] << 16) | (((const uint8_t *)(x))[1] << 8) |      \
   ((const uint8_t *)(x))[2])

typedef struct {
    int           head;
    uint64_t      cache;
    const uint8_t *data;
    const uint8_t *end;
} nal_bitstream;

typedef struct {
    uint64_t sps_id;
    uint64_t level_idc;
    uint64_t profile_idc;
    uint64_t chroma_format_idc;
    uint64_t max_num_ref_frames;
    uint64_t bit_depth_luma_minus8;
    uint64_t bit_depth_chroma_minus8;
    uint64_t pic_order_cnt_type;
    uint64_t pic_width_in_mbs_minus1;
    uint64_t pic_height_in_map_units_minus1;
    uint64_t log2_max_frame_num_minus4;
    uint64_t log2_max_pic_order_cnt_lsb_minus4;

    uint64_t frame_mbs_only_flag;
    uint64_t direct_8x8_inference_flag;
    uint64_t separate_colour_plane_flag;
    uint64_t mb_adaptive_frame_field_flag;
    uint64_t seq_scaling_matrix_present_flag;
    uint64_t qpprime_y_zero_transform_bypass_flag;
    uint64_t gaps_in_frame_num_value_allowed_flag;

    uint64_t frame_cropping_flag;
    uint64_t frame_crop_top_offset;
    uint64_t frame_crop_left_offset;
    uint64_t frame_crop_right_offset;
    uint64_t frame_crop_bottom_offset;
} sps_info_struct;

static void nal_bitstream_init(nal_bitstream *bs, const uint8_t *data, size_t size)
{
    bs->data  = data;
    bs->end   = data + size;
    bs->head  = 0;
    bs->cache = 0xffffffff;
}

static uint64_t nal_bitstream_read(nal_bitstream *bs, int n)
{
    uint64_t res = 0;
    int shift;

    if (n == 0)
        return res;

    while (bs->head < n) {
        uint8_t a_byte;
        int check_three_byte = 1;

    next_byte:
        if (bs->data >= bs->end) {
            n = bs->head;
            break;
        }
        a_byte = *bs->data++;
        if (check_three_byte && a_byte == 0x03 && ((bs->cache & 0xffff) == 0)) {
            check_three_byte = 0;
            goto next_byte;
        }
        bs->cache = (bs->cache << 8) | a_byte;
        bs->head += 8;
    }

    if ((shift = bs->head - n) > 0)
        res = bs->cache >> shift;
    else
        res = bs->cache;

    if (n < 32)
        res &= (1 << n) - 1;

    bs->head = shift;

    return res;
}

static int nal_bitstream_eos(nal_bitstream *bs)
{
    return (bs->data >= bs->end) && (bs->head == 0);
}

static int64_t nal_bitstream_read_ue(nal_bitstream *bs)
{
    int i = 0;

    while (nal_bitstream_read(bs, 1) == 0 && !nal_bitstream_eos(bs) && i < 32)
        i++;

    return ((1 << i) - 1 + nal_bitstream_read(bs, i));
}

void parse_h264_sps(uint8_t *sps, uint32_t sps_size, int *profile, int *level,
                    int *interlaced, int32_t *max_ref_frames)
{
    nal_bitstream bs;
    sps_info_struct sps_info = {0};

    nal_bitstream_init(&bs, sps, sps_size);

    sps_info.profile_idc = nal_bitstream_read(&bs, 8);
    nal_bitstream_read(&bs, 1); // constraint_set0_flag
    nal_bitstream_read(&bs, 1); // constraint_set1_flag
    nal_bitstream_read(&bs, 1); // constraint_set2_flag
    nal_bitstream_read(&bs, 1); // constraint_set3_flag
    nal_bitstream_read(&bs, 4); // reserved
    sps_info.level_idc = nal_bitstream_read(&bs, 8);
    sps_info.sps_id = nal_bitstream_read_ue(&bs);

    if (sps_info.profile_idc == 100 || sps_info.profile_idc == 110 ||
        sps_info.profile_idc == 122 || sps_info.profile_idc == 244 ||
        sps_info.profile_idc == 44 || sps_info.profile_idc == 83 ||
        sps_info.profile_idc == 86) {
        sps_info.chroma_format_idc = nal_bitstream_read_ue(&bs);
        if (sps_info.chroma_format_idc == 3)
            sps_info.separate_colour_plane_flag = nal_bitstream_read(&bs, 1);
        sps_info.bit_depth_luma_minus8 = nal_bitstream_read_ue(&bs);
        sps_info.bit_depth_chroma_minus8 = nal_bitstream_read_ue(&bs);
        sps_info.qpprime_y_zero_transform_bypass_flag = nal_bitstream_read(&bs, 1);

        sps_info.seq_scaling_matrix_present_flag = nal_bitstream_read(&bs, 1);
    }
    sps_info.log2_max_frame_num_minus4 = nal_bitstream_read_ue(&bs);
    sps_info.pic_order_cnt_type = nal_bitstream_read_ue(&bs);
    if (sps_info.pic_order_cnt_type == 0) {
        sps_info.log2_max_pic_order_cnt_lsb_minus4 = nal_bitstream_read_ue(&bs);
    }

    sps_info.max_num_ref_frames = nal_bitstream_read_ue(&bs);
    sps_info.gaps_in_frame_num_value_allowed_flag = nal_bitstream_read(&bs, 1);
    sps_info.pic_width_in_mbs_minus1 = nal_bitstream_read_ue(&bs);
    sps_info.pic_height_in_map_units_minus1 = nal_bitstream_read_ue(&bs);

    sps_info.frame_mbs_only_flag = nal_bitstream_read(&bs, 1);
    if (!sps_info.frame_mbs_only_flag)
        sps_info.mb_adaptive_frame_field_flag = nal_bitstream_read(&bs, 1);

    sps_info.direct_8x8_inference_flag = nal_bitstream_read(&bs, 1);

    sps_info.frame_cropping_flag = nal_bitstream_read(&bs, 1);
    if (sps_info.frame_cropping_flag) {
        sps_info.frame_crop_left_offset = nal_bitstream_read_ue(&bs);
        sps_info.frame_crop_right_offset = nal_bitstream_read_ue(&bs);
        sps_info.frame_crop_top_offset = nal_bitstream_read_ue(&bs);
        sps_info.frame_crop_bottom_offset = nal_bitstream_read_ue(&bs);
    }

    *level          = (int)(sps_info.level_idc);
    *profile        = (int)(sps_info.profile_idc);
    *interlaced     = (int)(!sps_info.frame_mbs_only_flag);
    *max_ref_frames = (int)(sps_info.max_num_ref_frames);
}

static H26xInfo* parse_h264_info(uint8_t *extra_data, uint32_t extra_size)
{
    uint8_t *spc = extra_data + 6;
    uint32_t sps_size = AV_RB16(spc);
    H26xInfo *info = (H26xInfo*) malloc(sizeof(H26xInfo));
    if (sps_size)
        parse_h264_sps(spc + 3, sps_size - 1, &info->profile, &info->level,
                       &info->is_interlaced, &info->max_ref_frames);
    return info;
}
