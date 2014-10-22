/*
 * org_xiph_opus_decoderjni_OpusDecoder.h
 * Takes a opus bitstream from java callbacks from JNI and writes raw stereo PCM to the jni callbacks. Decodes simple and chained OggOpus files from beginning to end.
 *
 * (C) 2014 Radu Motisan, radu.motisan@gmail.com
 *
 * Part of the OpenPlayer implementation for Alpine Audio Now Digital LLC
 */

#include <jni.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <ogg/ogg.h>
#include <opus.h>
#include <opus_header.h>
#include <android/log.h>

#ifndef _Included_org_xiph_opus_OpusDecoder
	#define _Included_org_xiph_opus_OpusDecoder
	#ifdef __cplusplus
	extern "C" {
	#endif

	// called to do the initialization
	JNIEXPORT int JNICALL Java_org_xiph_opus_decoderjni_OpusDecoder_initJni(JNIEnv *env, jclass cls, int debug0);

	//Starts the decoding from a vorbis bitstream to pcm
	JNIEXPORT int JNICALL Java_org_xiph_opus_decoderjni_OpusDecoder_readDecodeWriteLoop(JNIEnv *env, jclass cls,  jobject OpusDataFeed);

	//Stops the Opus data feed
	void onStopDecodeFeed(JNIEnv *env, jobject* OpusDataFeed, jmethodID* stopMethodId);

	//Reads raw Opus data from the jni callback
	int onReadOpusDataFromOpusDataFeed(JNIEnv *env, jobject* OpusDataFeed, jmethodID* readOpusDataMethodId, char* buffer, jbyteArray* jByteArrayReadBuffer);

	//Writes the pcm data to the Java layer
	void onWritePCMDataFromOpusDataFeed(JNIEnv *env, jobject* OpusDataFeed, jmethodID* writePCMDataMethodId, ogg_int16_t* buffer, int bytes, jshortArray* jShortArrayWriteBuffer);

	//Starts the decode feed with the necessary information about sample rates, channels, etc about the stream
	void onStart(JNIEnv *env, jobject *opusDataFeed, jmethodID* startMethodId, long sampleRate, long channels, char* vendor,
			char *title, char *artist, char *album, char *date, char *track);
	//Starts reading the header information
	void onStartReadingHeader(JNIEnv *env, jobject *OpusDataFeed, jmethodID* startReadingHeaderMethodId);

	//Inform player we are about to start a new iteration
	void onNewIteration(JNIEnv *env, jobject *OpusDataFeed, jmethodID* newIterationMethodId);

	#ifdef __cplusplus
	}
	#endif
#endif
