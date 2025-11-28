/**
 * Note: Common Util
 * Date: 2025/11/28
 * Author: frank
 */

#ifndef COMMON_UTIL_H
#define COMMON_UTIL_H

#include <chrono>
#include <cstdint>

// 字节数组转int类型(大端)
static inline uint32_t ByteToInt(const uint8_t *src) {
    return (uint32_t) ((src[0] & 0xFF) << 24 |
                       (src[1] & 0xFF) << 16 |
                       (src[2] & 0xFF) << 8 |
                       (src[3] & 0xFF));
}

static inline int64_t CurrentTimeUs() {
    return std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
}

static inline int64_t CurrentTimeMs() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
}

#endif //COMMON_UTIL_H
