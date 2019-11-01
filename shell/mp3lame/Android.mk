WORKING_DIR := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_PATH := $(WORKING_DIR)
LOCAL_ARM_MODE  := arm
LOCAL_MODULE    := mp3lame
LOCAL_SRC_FILES := mp3lame/bitstream.c mp3lame/encoder.c \
                   mp3lame/fft.c mp3lame/gain_analysis.c mp3lame/id3tag.c \
                   mp3lame/lame.c mp3lame/newmdct.c mp3lame/quantize.c \
                   mp3lame/presets.c mp3lame/psymodel.c mp3lame/quantize_pvt.c \
                   mp3lame/reservoir.c mp3lame/set_get.c mp3lame/tables.c \
                   mp3lame/takehiro.c mp3lame/util.c mp3lame/vbrquantize.c \
                   mp3lame/VbrTag.c mp3lame/version.c

LOCAL_C_INCLUDES := mp3lame
LOCAL_LDLIBS    := -llog -lz -lm -landroid

include $(BUILD_STATIC_LIBRARY)
#include $(BUILD_SHARED_LIBRARY)

