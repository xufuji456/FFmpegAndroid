//
// Created by xu fulong on 2022/7/9.
//

#include "yuv_converter.h"
#include <cstring>

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

static void rgba_to_yuv420p(int *argb, int8_t *yuv, int width, int height) {
    int frameSize = width * height;
    int index = 0;
    int yIndex = 0;
    int uIndex = frameSize;
    int vIndex = frameSize * 5 / 4;
    int R, G, B, Y, U, V;

    for (int j = 0; j < height; j++) {
        for (int i = 0; i < width; i++) {
            R = (argb[index] & 0xff0000) >> 16;
            G = (argb[index] & 0xff00) >> 8;
            B = (argb[index] & 0xff);

            // RGB to YUV algorithm
            Y = ((66 * R + 129 * G + 25 * B + 128) >> 8) + 16;

            // I420(YUV420p) -> YYYYYYYY UU VV
            yuv[yIndex++] = (int8_t) Y;
            if (j % 2 == 0 && i % 2 == 0) {
                U = ((-38 * R - 74 * G + 112 * B + 128) >> 8) + 128;
                V = ((112 * R - 94 * G - 18 * B + 128) >> 8) + 128;
                yuv[uIndex++] = (int8_t) U;
                yuv[vIndex++] = (int8_t) V;
            }
            index++;
        }
    }
}

static int yuv2argb(int y, int u, int v) {
#define max(a, b) ((a) > (b) ? (a) : (b))

    int r, g, b;

    r = y + (int) (1.402f * u);
    g = y - (int) (0.344f * v + 0.714f * u);
    b = y + (int) (1.772f * v);
    r = r > 255 ? 255 : max(r, 0);
    g = g > 255 ? 255 : max(g, 0);
    b = b > 255 ? 255 : max(b, 0);
    return 0xff000000 | (r << 16) | (g << 8) | b;
}

static void yuv420p_to_argb(int8_t *yuv, int *argb, int width, int height) {
    int size = width * height;
    int offset = size;
    int u, v, y1, y2, y3, y4;

    for (int i = 0, k = 0; i < size; i += 2, k += 2) {
        y1 = yuv[i] & 0xff;
        y2 = yuv[i + 1] & 0xff;
        y3 = yuv[width + i] & 0xff;
        y4 = yuv[width + i + 1] & 0xff;

        u = yuv[offset + k] & 0xff;
        v = yuv[offset + k + 1] & 0xff;
        u = u - 128;
        v = v - 128;

        argb[i] = yuv2argb(y1, u, v);
        argb[i + 1] = yuv2argb(y2, u, v);
        argb[width + i] = yuv2argb(y3, u, v);
        argb[width + i + 1] = yuv2argb(y4, u, v);

        if (i != 0 && (i + 2) % width == 0)
            i += width;
    }
}

static void nv21_to_yuv420p(int8_t *dst, int8_t *src, int len) {
    memcpy(dst, src, len); // y
    for (int i = 0; i < len / 4; ++i) {
        *(dst + len + i) = *(src + len + i * 2 + 1);  // u
        *(dst + len * 5 / 4 + i) = *(src + len + i * 2); // v
    }
}

static void yuv420p_rotate90(int8_t *dst, int8_t *src, int width, int height) {
    int n = 0;
    int wh = width * height;
    int half_width = width / 2;
    int half_height = height / 2;
    // y
    for (int j = 0; j < width; j++) {
        for (int i = height - 1; i >= 0; i--) {
            dst[n++] = src[width * i + j];
        }
    }
    // u
    for (int i = 0; i < half_width; i++) {
        for (int j = 1; j <= half_height; j++) {
            dst[n++] = src[wh + ((half_height - j) * half_width + i)];
        }
    }
    // v
    for (int i = 0; i < half_width; i++) {
        for (int j = 1; j <= half_height; j++) {
            dst[n++] = src[wh + wh / 4 + ((half_height - j) * half_width + i)];
        }
    }
}

static void yuv420p_rotate180(int8_t *dst, int8_t *src, int width, int height) {
    int n = 0;
    int half_width = width / 2;
    int half_height = height / 2;
    // y
    for (int j = height - 1; j >= 0; j--) {
        for (int i = width; i > 0; i--) {
            dst[n++] = src[width * j + i - 1];
        }
    }
    // u
    int offset = width * height;
    for (int j = half_height - 1; j >= 0; j--) {
        for (int i = half_width; i > 0; i--) {
            dst[n++] = src[offset + half_width * j + i - 1];
        }
    }
    // v
    offset += half_width * half_height;
    for (int j = half_height - 1; j >= 0; j--) {
        for (int i = half_width; i > 0; i--) {
            dst[n++] = src[offset + half_width * j + i - 1];
        }
    }
}

static void yuv420p_rotate270(int8_t *dst, int8_t *src, int width, int height) {

    for (int j = 0; j < width; j++) {
        for (int i = 1; i <= height; i++) {
            *dest++ = *(src + i * width - j);
        }
    }

    char *src_u = src + width * height;
    for (int j = 0; j < width / 2; j++) {
        for (int i = 1; i <= height / 2; i++) {
            *dest++ = *(src_u + i * width / 2 - j);

        }
    }

    char *src_v = src + width * height * 5 / 4;
    for (int j = 0; j < width / 2; j++) {
        for (int i = 1; i <= height / 2; i++) {
            *dest++ = *(src_v + i * width / 2 - j);
        }
    }
}

static void yuv420p_rotate(int8_t *dst, int8_t *src, int width, int height, int degree) {
    switch(degree) {
        case 0:
            memcpy(dst, src, width * height * 3 / 2);
            break;
        case 90:
            yuv420p_rotate90(dst, src, width, height);
            break;
        case 180:
            yuv420p_rotate180(dst, src, width, height);
            break;
        case 270:
            yuv420p_rotate270(dst, src, width, height);
            break;
        default:
            break;
    }
}