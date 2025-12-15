/**
 * Note: the rotation of texture
 * Date: 2025/12/15
 * Author: frank
 */

#ifndef ROTATION_HELPER_H
#define ROTATION_HELPER_H

// 0°
static const float TextureRotationNo[8] = {
        0.0f,
        1.0f,
        1.0f,
        1.0f,
        0.0f,
        0.0f,
        1.0f,
        0.0f
};

// rotate 90°
static const float TextureRotation90[8] = {
        1.0f,
        1.0f,
        1.0f,
        0.0f,
        0.0f,
        1.0f,
        0.0f,
        0.0f
};

// rotate 180°
static const float TextureRotation180[8] = {
        1.0f,
        0.0f,
        0.0f,
        0.0f,
        1.0f,
        1.0f,
        0.0f,
        1.0f
};

// rotate 270°
static const float TextureRotation270[8] = {
        0.0f,
        0.0f,
        0.0f,
        1.0f,
        1.0f,
        0.0f,
        1.0f,
        1.0f
};

// vertical flip
static const float TextureRotationFlipV[8] = {
        0.0f,
        0.0f,
        1.0f,
        0.0f,
        0.0f,
        1.0f,
        1.0f,
        1.0f
};

// horizontal flip
static const float TextureRotationFlipH[8] = {
        1.0f,
        1.0f,
        0.0f,
        1.0f,
        1.0f,
        0.0f,
        0.0f,
        0.0f
};


#endif //ROTATION_HELPER_H
