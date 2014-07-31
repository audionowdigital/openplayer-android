#include <android/log.h>

extern int debug;

#define LOG_TAG "jniDecoder"
#define LOGD(LOG_TAG, ...) if (debug) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG,__VA_ARGS__)
#define LOGV(LOG_TAG, ...) if (debug) __android_log_print(ANDROID_LOG_VERBOSE, LOG_TAG, __VA_ARGS__)
#define LOGE(LOG_TAG, ...) if (debug) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG,__VA_ARGS__)
#define LOGW(LOG_TAG, ...) if (debug) __android_log_print(ANDROID_LOG_WARN, LOG_TAG,__VA_ARGS__)
#define LOGI(LOG_TAG, ...) if (debug) __android_log_print(ANDROID_LOG_INFO, LOG_TAG,__VA_ARGS__)
