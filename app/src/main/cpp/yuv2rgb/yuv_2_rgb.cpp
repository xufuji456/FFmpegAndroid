//
// Created by xu fulong on 2022/7/8.
//

#include "yuv_to_rgb.h"

static int range(int val) {
    if (val < 0) {
        return 0;
    } else if (val > 255) {
        return 255;
    } else {
        return val;
    }
}

static void YCbCr_to_RGB(uint8_t *dst, const uint8_t *src, int length) {
    if (length < 1) return;
    int R, G, B;
    int Y, Cb, Cr;
    int i, offset;
    for (i = 0; i < length; i++) {
        offset = (i << 1) + i;
        Y = src[offset];
        Cb = src[offset + 1] - 128;
        Cr = src[offset + 2] - 128;
        R = Y + ((RGBRCrI * Cr + half_shift) >> shift);
        G = Y + ((RGBGCbI * Cb + RGBGCrI * Cr + half_shift) >> shift);
        B = Y + ((RGBBCbI * Cb + half_shift) >> shift);
        R = range(R);
        G = range(G);
        B = range(B);
        offset = i << 2;
        dst[offset] = (uint8_t) B;
        dst[offset + 1] = (uint8_t) G;
        dst[offset + 2] = (uint8_t) R;
        dst[offset + 3] = 0xff;
    }
}

static void RGB_to_YCbCr(uint8_t *dst, const uint8_t *src, int length) {
    if (length < 1) return;
    int R, G, B;
    int i, offset;
    for (i = 0; i < length; i++) {
        offset = i << 2;
        B = src[offset];
        G = src[offset + 1];
        R = src[offset + 2];
        offset = (i << 1) + i;
        dst[offset] = (uint8_t) ((YCbCrYRI * R + YCbCrYGI * G + YCbCrYBI * B + half_shift)
                >> shift);
        dst[offset + 1] = (uint8_t) (128 +
                                     ((YCbCrCbRI * R + YCbCrCbGI * G + YCbCrCbBI * B + half_shift)
                                             >> shift));
        dst[offset + 2] = (uint8_t) (128 +
                                     ((YCbCrCrRI * R + YCbCrCrGI * G + YCbCrCrBI * B + half_shift)
                                             >> shift));
    }
}