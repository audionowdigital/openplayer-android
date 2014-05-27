PATH_ABS := $(abspath $(call my-dir)/../../../libs_common/)
PATH_LOC := $(call my-dir)
#can't use the same name for this variable as in the .mk scripts invoked, as it will get overriden.

#include $(addprefix $(LOCAL_PATH)/, $(addsuffix /Android.mk, libogg libvorbis libvorbis-jni libopus libopus-jni ))

include $(PATH_ABS)/libogg/Android.mk
include $(PATH_ABS)/libvorbis/Android.mk
include $(PATH_ABS)/libopus/Android.mk

include $(PATH_LOC)/libvorbis-jni/Android.mk
include $(PATH_LOC)/libopus-jni/Android.mk
