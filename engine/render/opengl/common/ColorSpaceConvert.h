/**
 * Note: color-space conversion: BT601、BT709、BT2020
 * Date: 2025/12/15
 * Author: frank
 */

#ifndef COLORSPACE_CONVERT_H
#define COLORSPACE_CONVERT_H

/**
 * BT601: YUV to RGB
 * color range：standard color，video(limit):16~235, pc(full):0~255
 */
static const float Bt601LimitRangeYUV2RGBMatrix[] = {
        1.164384, 1.164384, 1.164384,  0.0, -0.391762,
        2.017232, 1.596027, -0.812968, 0.0
};

static const float Bt601FullRangeYUV2RGBMatrix[] = {
        1.0, 1.0, 1.0, 0.0, -0.344136,
        1.772, 1.402, -0.714136, 0.0
};

/**
 * BT709: YUV to RGB
 * color range：standard color，video(limit):16~235, pc(full):0~255
 */
static const float Bt709LimitRangeYUV2RGBMatrix[] = {
        1.164384, 1.164384, 1.164384,  0.0, -0.213249,
        2.112402, 1.792741, -0.532909, 0.0
};

static const float Bt709FullRangeYUV2RGBMatrix[] = {
        1.0, 1.0, 1.0, 0.0, -0.187324,
        1.8556, 1.5748, -0.468124, 0.0
};

/**
 * BT2020: YUV to RGB
 * color range: wide color gamut, 10bit、12bit
 */
static const float Bt2020FullRangeYUV2RGBMatrix[] = {
        1.0, 1.0, 1.0, 0.0, -0.164553,
        1.8814, 1.4746, -0.571353, 0.0
};

static const float Bt2020LimitRangeYUV2RGBMatrix[] = {
        1.164384, 1.164384, 1.164384,  0.0, -0.187326,
        2.141772, 1.678674, -0.650424, 0.0
};

static const float ModelViewProjectionMatrix[] = {
        1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f
};

#endif //COLORSPACE_CONVERT_H
