WORKING_DIR := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_PATH := $(WORKING_DIR)
LOCAL_ARM_MODE  := arm
LOCAL_MODULE    := shine
LOCAL_SRC_FILES := shine/bitstream.c shine/huffman.c \
                   shine/l3bitstream.c shine/l3loop.c shine/l3mdct.c \
                   shine/l3subband.c shine/layer3.c shine/reservoir.c \
                   shine/tables.c
LOCAL_LDLIBS    := -llog -lz -lm -landroid

LOCAL_C_INCLUDES := shine
LOCAL_LDLIBS    := -llog -lz -lm -landroid

include $(BUILD_STATIC_LIBRARY)
#include $(BUILD_SHARED_LIBRARY)

