LOCAL_PATH := $(call my-dir)
#can't use the same name for this variable as in the .mk scripts invoked, as it will get overriden.

include $(addprefix $(LOCAL_PATH)/, $(addsuffix /Android.mk, libogg libvorbis libvorbis-jni libopus libopus-jni ))
