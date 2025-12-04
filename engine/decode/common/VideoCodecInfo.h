
#ifndef VIDEO_CODEC_INFO_H
#define VIDEO_CODEC_INFO_H

#if defined(__ANDROID__)
#include <android/native_window.h>
#endif

#if defined(__HARMONY__)
#include <multimedia/player_framework/native_avcodec_base.h>
#endif

enum DecodeFlag {
    DECODE_FLAG_NO_OUT_FRAME = 0x1000
};

struct HardWareContext {
    virtual ~HardWareContext() = default;
};

enum class VideoCodecType {
    DECODE_TYPE_UNKNOWN  = 0,
    DECODE_TYPE_SOFTWARE = 1,
    DECODE_TYPE_ANDROID  = 2,
    DECODE_TYPE_IOS      = 3,
    DECODE_TYPE_HARMONY  = 4
};

#if defined(__ANDROID__)
struct AndroidHardWareContext : public HardWareContext {
    explicit AndroidHardWareContext(ANativeWindow *window) : native_window(window) {

    }

    ANativeWindow *native_window = nullptr;
};
#endif

#if defined(__HARMONY__)
struct HarmonyHardWareContext : public HardWareContext {
  explicit HarmonyHardWareContext(OHNativeWindow *window) : native_window(window) {

  }
  OHNativeWindow *native_window = nullptr;
};
#endif

#endif
