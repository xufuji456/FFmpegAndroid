#ifndef H26X_PARSER_H
#define H26X_PARSER_H

#include <stdint.h>

typedef struct H26xInfo {
    int level;
    int profile;
    int is_interlaced;
    int max_ref_frames;
}H26xInfo;

static H26xInfo* parse_h264_info(uint8_t *extra_data, uint32_t extra_size);

#endif
