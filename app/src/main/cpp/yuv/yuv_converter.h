//
// Created by xu fulong on 2022/7/9.
//

#ifndef FFMPEGANDROID_YUV_CONVERTER_H
#define FFMPEGANDROID_YUV_CONVERTER_H

#include <cstdint>

static void rgba_to_yuv420p(int *argb, int8_t *yuv, int width, int height);

static void yuv420p_to_argb(int8_t *yuv, int *argb, int width, int height);

static void yuv420p_rotate(int8_t *dst, int8_t *src, int width, int height, int degree);

/**
 * convert NV21 to YUV420P
 * @param dst data of yuv420p
 * @param src data of nv21
 * @param len width*height
 */
static void nv21_to_yuv420p(int8_t *dst, int8_t *src, int len);

/**
 * convert NV12 to YUV420P
 * @param dst data of yuv420p
 * @param src data of nv12
 * @param len width*height
 */
static void nv12_to_yuv420p(int8_t *dst, int8_t *src, int len);

#endif //FFMPEGANDROID_YUV_CONVERTER_H
