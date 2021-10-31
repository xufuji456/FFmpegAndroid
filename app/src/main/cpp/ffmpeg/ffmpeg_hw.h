//
// Created by frank on 2021/10/31.
//

#ifndef FFMPEGANDROID_FFMPEG_HW_H
#define FFMPEGANDROID_FFMPEG_HW_H

#include "ffmpeg_common.h"

#ifdef __cplusplus
extern "C" {
#endif
HWDevice *hw_device_get_by_name(const char *name);

int hw_device_init_from_string(const char *arg, HWDevice **dev_out);

void hw_device_free_all(void);

int hw_device_setup_for_decode(InputStream *ist);

int hw_device_setup_for_encode(OutputStream *ost);

int hwaccel_decode_init(AVCodecContext *avctx);
#ifdef __cplusplus
}
#endif

#endif //FFMPEGANDROID_FFMPEG_HW_H
