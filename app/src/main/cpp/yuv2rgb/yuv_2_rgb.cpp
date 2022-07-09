//
// Created by xu fulong on 2022/7/8.
//

#include "yuv_to_rgb.h"

// https://chromium.googlesource.com/libyuv/libyuv
// https://mymusing.co/bt601-yuv-to-rgb-conversion-color/
// https://www.color.org/chardata/rgb/rgb_registry.xalter

// BT.601 YUV to RGB
// R = 1.164 * (Y - 16) + 1.596 * (V - 128)
// G = 1.164 * (Y - 16) - 0.392 * (U - 128) - 0.812 * (V - 128)
// B = 1.164 * (Y - 16) + 2.016 * (U - 128)

// Y = 0.257 * R + 0.504 * G + 0.098 * B + 16
// U = -0.148 * R - 0.291 * G + 0.439 * B + 128
// V = 0.439 * R - 0.368 * G - 0.072 * B + 128

// BT.709 YUV to RGB
// R = 1.1644 * (Y - 16) + 1.7928 * (V - 128)
// G = 1.1644 * (Y - 16) - 0.2133 * (U - 128) - 0.533 * (V - 128)
// B = 1.1644 * (Y - 16) + 2.1124 * (U - 128)

// Y = 0.1826 * R + 0.6142 * G + 0.0620 * B + 16
// U = -0.1006 * R - 0.3386 * G + 0.4392 * B + 128
// V = 0.4392 * B - 0.3989 * G - 0.0403 * B + 128

// BT.2020 YUV to RGB
// R = Y - 16 + 1.4746 * (V - 128)
// G = Y - 16 - 0.1645 * (U - 128) - 0.5713 * (V - 128)
// B = Y - 16 + 1.881 * (U - 128)

// Y = 0.2627 * R + 0.6780 * G + 0.0593 * B + 16
// U = -0.1396 * R - 0.3604 * G + 0.5 * B + 128
// V = 0.5 * R - 0.4598 * G - 0.0402 * B + 128

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