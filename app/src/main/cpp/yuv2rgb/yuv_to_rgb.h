//
// Created by xu fulong on 2022/7/8.
//

#ifndef FFMPEGANDROID_YUV_TO_RGB_H
#define FFMPEGANDROID_YUV_TO_RGB_H

#include <cstdint>

const float YCbCrYRF = 0.299F;
const float YCbCrYGF = 0.587F;
const float YCbCrYBF = 0.114F;
const float YCbCrCbRF = -0.168736F;
const float YCbCrCbGF = -0.331264F;
const float YCbCrCbBF = 0.500000F;
const float YCbCrCrRF = 0.500000F;
const float YCbCrCrGF = -0.418688F;
const float YCbCrCrBF = -0.081312F;

const float RGBRCrF = 1.40200F;
const float RGBGCbF = -0.34414F;
const float RGBGCrF = -0.71414F;
const float RGBBCbF = 1.77200F;

const int shift = 20;
const int half_shift = 1 << (shift - 1);

const int YCbCrYRI = (int)(YCbCrYRF * (1 << shift) + 0.5);
const int YCbCrYGI = (int)(YCbCrYGF * (1 << shift) + 0.5);
const int YCbCrYBI = (int)(YCbCrYBF * (1 << shift) + 0.5);
const int YCbCrCbRI = (int)(YCbCrCbRF * (1 << shift) + 0.5);
const int YCbCrCbGI = (int)(YCbCrCbGF * (1 << shift) + 0.5);
const int YCbCrCbBI = (int)(YCbCrCbBF * (1 << shift) + 0.5);
const int YCbCrCrRI = (int)(YCbCrCrRF * (1 << shift) + 0.5);
const int YCbCrCrGI = (int)(YCbCrCrGF * (1 << shift) + 0.5);
const int YCbCrCrBI = (int)(YCbCrCrBF * (1 << shift) + 0.5);

const int RGBRCrI = (int)(RGBRCrF * (1 << shift) + 0.5);
const int RGBGCbI = (int)(RGBGCbF * (1 << shift) + 0.5);
const int RGBGCrI = (int)(RGBGCrF * (1 << shift) + 0.5);
const int RGBBCbI = (int)(RGBBCbF * (1 << shift) + 0.5);

static void YCbCr_to_RGB(uint8_t* dst, const uint8_t* src, int length);

static void RGB_to_YCbCr(uint8_t* dst, const uint8_t* src, int length);

#endif //FFMPEGANDROID_YUV_TO_RGB_H
