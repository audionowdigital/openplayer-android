/*
 * DecodeFeed.h
 *
 * (C) 2014 Radu Motisan, radu.motisan@gmail.com
 *
 * Part of the OpenPlayer implementation for Alpine Audio Now Digital LLC
 */


#include <jni.h>
#include <stdio.h>
#include <stdlib.h>
#include <android/log.h>
#include <ogg/ogg.h>
#include "Log.h"

#define BUFFER_LENGTH 4096


//Stops the vorbis data feed
void onStop(JNIEnv *env, jobject* javaDecodeFeedObj, jmethodID* stopMethodId);

//Reads raw vorbis data from the jni callback
int onReadEncodedData(JNIEnv *env, jobject* javaDecodeFeedObj, jmethodID* readVorbisDataMethodId, char* buffer, jbyteArray* jByteArrayReadBuffer);

//Writes the pcm data to the Java layer
void onWritePCMData(JNIEnv *env, jobject* javaDecodeFeedObj, jmethodID* writePCMDataMethodId, ogg_int16_t* buffer, int bytes, jshortArray* jShortArrayWriteBuffer);

//Starts the decode feed with the necessary information about sample rates, channels, etc about the stream
void onStart(JNIEnv *env, jobject *javaDecodeFeedObj, jmethodID* startMethodId, long sampleRate, long channels, char* vendor,
		char *title, char *artist, char *album, char *date, char *track);

//Starts reading the header information
void onStartReadingHeader(JNIEnv *env, jobject *javaDecodeFeedObj, jmethodID* startReadingHeaderMethodId);
