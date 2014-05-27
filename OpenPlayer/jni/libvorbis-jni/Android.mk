LOCAL_PATH := $(call my-dir)
PATH_ABS := $(abspath $(call my-dir)/../../../../libs_common/)

include $(CLEAR_VARS)

LOCAL_MODULE := vorbis-jni
LOCAL_CFLAGS += -I$(PATH_ABS)/libvorbis/include -I$(PATH_ABS)/libogg/include -fsigned-char
ifeq ($(TARGET_ARCH),arm)
	LOCAL_CFLAGS += -march=armv6 -marm -mfloat-abi=softfp -mfpu=vfp
endif

LOCAL_SHARED_LIBRARIES := libogg libvorbis

LOCAL_LDLIBS := -L$(SYSROOT)/usr/lib -llog

LOCAL_SRC_FILES := org_xiph_vorbis_decoderjni_VorbisDecoder.c

include $(BUILD_SHARED_LIBRARY)
