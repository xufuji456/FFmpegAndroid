/**
 * Note: define of struct
 * Date: 2025/11/29
 * Author: frank
 */

#ifndef NEXT_STRUCT_DEFINE_H
#define NEXT_STRUCT_DEFINE_H

#include <functional>
#include <vector>

#define sp std::shared_ptr

using LogCallback    = std::function<void(int, const char*, const char*)>;
using NotifyCallback = std::function<void(int, int, int, void*, int)>;
using InjectCallback = std::function<int(void*, int, void*)>;

struct TrackInfo {
    int codec_id      = 0;
    int stream_type   = -1;
    int stream_index  = -1;
    int codec_profile = 0;
    int64_t bit_rate  = 0;

    // Video
    int width     = 0;
    int height    = 0;
    int sar_num   = 0;
    int sar_den   = 1;
    int fps_num   = 0;
    int fps_den   = 1;
    int tbr_num   = 0;
    int tbr_den   = 1;
    int rotation  = 0;
    int pixel_fmt = -1;
    uint8_t color_space     = 0;
    uint8_t color_range     = 0;
    uint8_t color_transfer  = 0;
    uint8_t color_primaries = 0;

    // Audio
    int channels            = 0;
    int sample_fmt          = 0;
    int sample_rate         = 0;
    int time_base_num       = 0;
    int time_base_den       = 1;
    int extra_data_size     = 0;
    uint8_t *extra_data     = nullptr;
};

struct MetaData {
    ~MetaData() {
        for (auto & it : track_info) {
            if (it.extra_data) {
                delete[] it.extra_data;
                it.extra_data = nullptr;
            }
        }
    }

    int64_t duration   = 0;
    int64_t bit_rate   = 0;
    int64_t start_time = 0;
    int audio_index    = -1;
    int video_index    = -1;
    int subtitle_index = -1;

    std::vector<TrackInfo> track_info;
};

#endif //NEXT_STRUCT_DEFINE_H
