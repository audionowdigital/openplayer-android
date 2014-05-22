LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := libvorbis
LOCAL_CFLAGS += -I$(LOCAL_PATH)/include -I$(LOCAL_PATH)/lib -I$(LOCAL_PATH)/../libogg/include -ffast-math -fsigned-char

ifeq ($(TARGET_ARCH),arm)
	LOCAL_CFLAGS += -march=armv6 -marm -mfloat-abi=softfp -mfpu=vfp
endif

LOCAL_SHARED_LIBRARIES := libogg

LOCAL_SRC_FILES := \
	lib/mdct.c		lib/smallft.c	\
	lib/block.c		lib/envelope.c	\
	lib/window.c	        lib/lsp.c	\
	lib/lpc.c		lib/analysis.c	\
	lib/synthesis.c	        lib/psy.c       \
	lib/info.c		lib/floor1.c	\
	lib/floor0.c	        lib/res0.c	\
	lib/mapping0.c	        lib/registry.c	\
	lib/codebook.c	        lib/sharedbook.c\
	lib/lookup.c	        lib/bitrate.c	\
	lib/vorbisfile.c	lib/vorbisenc.c

include $(BUILD_SHARED_LIBRARY)
