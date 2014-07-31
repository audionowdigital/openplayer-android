LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := libvorbis
LOCAL_CFLAGS += -I$(LOCAL_PATH)/include -I$(LOCAL_PATH)/lib -I$(LOCAL_PATH)/../libogg/include -ffast-math -fsigned-char

ifeq ($(TARGET_ARCH),arm)
	LOCAL_CFLAGS += -march=armv6 -marm -mfloat-abi=softfp -mfpu=vfp
endif

LOCAL_SHARED_LIBRARIES := libogg

LOCAL_SRC_FILES := \
	lib/analysis.c 				 lib/bitrate.c \
  lib/block.c             lib/codebook.c	\
	lib/envelope.c					lib/floor0.c	\
	lib/floor1.c						lib/info.c	\
	lib/lookup.c						lib/lpc.c	\
	lib/lsp.c							 lib/mapping0.c \
	lib/mdct.c 						 lib/psy.c	\
	lib/registry.c 				 lib/res0.c	\
	lib/sharedbook.c 			 lib/smallft.c	\
	lib/synthesis.c 				lib/vorbisenc.c	\
	lib/vorbisfile.c 			 lib/window.c
include $(BUILD_SHARED_LIBRARY)
