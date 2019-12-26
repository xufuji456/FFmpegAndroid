LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := faac
LOCAL_CFLAGS := 
LOCAL_SRC_FILES := libfaac/aacquant.c libfaac/backpred.c libfaac/bitstream.c \
	    libfaac/channels.c libfaac/fft.c libfaac/filtbank.c libfaac/frame.c \
        libfaac/huffman.c libfaac/ltp.c libfaac/midside.c libfaac/psychkni.c \
        libfaac/tns.c libfaac/util.c

LOCAL_C_INCLUDES := $(LOCAL_PATH)/libfaac
LOCAL_LDLIBS := -llog
LOCAL_LDLIBS += -landroid

include $(BUILD_STATIC_LIBRARY)

