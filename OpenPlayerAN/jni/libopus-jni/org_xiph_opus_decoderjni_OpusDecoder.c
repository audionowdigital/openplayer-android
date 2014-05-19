/* Takes a opus bitstream from java callbacks from JNI and writes raw stereo PCM to
the jni callbacks. Decodes simple and chained OggOpus files from beginning
to end. */

#include "org_xiph_opus_decoderjni_OpusDecoder.h"

/*Define message codes*/
#define NOT_OPUS_HEADER -1
#define CORRUPT_HEADER -2
#define OPUS_DECODE_ERROR -3
#define SUCCESS 0

#define OPUS_HEADERS 2

#define BUFFER_LENGTH 4096



int debug = 0;

#define LOG_TAG "jniOpusDecoder"
#define LOGD(LOG_TAG, ...) if (debug) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG,__VA_ARGS__)
#define LOGV(LOG_TAG, ...) if (debug) __android_log_print(ANDROID_LOG_VERBOSE, LOG_TAG, __VA_ARGS__)
#define LOGE(LOG_TAG, ...) if (debug) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG,__VA_ARGS__)
#define LOGW(LOG_TAG, ...) if (debug) __android_log_print(ANDROID_LOG_WARN, LOG_TAG,__VA_ARGS__)
#define LOGI(LOG_TAG, ...) if (debug) __android_log_print(ANDROID_LOG_INFO, LOG_TAG,__VA_ARGS__)

JNIEXPORT jint JNI_OnLoad(JavaVM *vm, void *reserved) {
  LOGD(LOG_TAG, "onLoad called.");
	return JNI_VERSION_1_6;
}

//Stops the opus data feed
void onStopDecodeFeed(JNIEnv *env, jobject* opusDataFeed, jmethodID* stopMethodId) {
    (*env)->CallVoidMethod(env, (*opusDataFeed), (*stopMethodId));
}

//Reads raw opus data from the jni callback
int onReadOpusDataFromOpusDataFeed(JNIEnv *env, jobject* opusDataFeed, jmethodID* readOpusDataMethodId, char* buffer, jbyteArray* jByteArrayReadBuffer) {
    //Call the read method
    int readByteCount = (*env)->CallIntMethod(env, (*opusDataFeed), (*readOpusDataMethodId), (*jByteArrayReadBuffer), BUFFER_LENGTH);

    //Don't bother copying, just return 0
    if(readByteCount == 0) return 0;

    //Gets the bytes from the java array and copies them to the opus buffer
    jbyte* readBytes = (*env)->GetByteArrayElements(env, (*jByteArrayReadBuffer), NULL);
    memcpy(buffer, readBytes, readByteCount);
    
    //Clean up memory and return how much data was read
    (*env)->ReleaseByteArrayElements(env, (*jByteArrayReadBuffer), readBytes, JNI_ABORT);

    //Return the amount actually read
    return readByteCount;
}

//Writes the pcm data to the Java layer
void onWritePCMDataFromOpusDataFeed(JNIEnv *env, jobject* opusDataFeed, jmethodID* writePCMDataMethodId, ogg_int16_t* buffer, int bytes, jshortArray* jShortArrayWriteBuffer) {

    //No data to read, just exit
    if(bytes == 0) return;

    //Copy the contents of what we're writing to the java short array
    (*env)->SetShortArrayRegion(env, (*jShortArrayWriteBuffer), 0, bytes, (jshort *)buffer);
    
    //Call the write pcm data method
    (*env)->CallVoidMethod(env, (*opusDataFeed), (*writePCMDataMethodId), (*jShortArrayWriteBuffer), bytes);
}

//Starts the decode feed with the necessary information about sample rates, channels, etc about the stream
void onStart(JNIEnv *env, jobject *opusDataFeed, jmethodID* startMethodId, long sampleRate, long channels, char* vendor) {
    LOGI(LOG_TAG, "Notifying decode feed");

    //Creates a java string for the vendor
    jstring vendorString = (*env)->NewStringUTF(env, vendor);

    //Get decode stream info class and constructor
    jclass decodeStreamInfoClass = (*env)->FindClass(env, "org/xiph/opus/decoderjni/DecodeStreamInfo");

    jmethodID constructor = (*env)->GetMethodID(env, decodeStreamInfoClass, "<init>", "(JJLjava/lang/String;)V");

    //Create the decode stream info object
    jobject decodeStreamInfo = (*env)->NewObject(env, decodeStreamInfoClass, constructor, (jlong)sampleRate, (jlong)channels, vendorString);

    //Call decode feed onStart
    (*env)->CallVoidMethod(env, (*opusDataFeed), (*startMethodId), decodeStreamInfo);

    //Cleanup decode feed object
    (*env)->DeleteLocalRef(env, decodeStreamInfo);

    //Cleanup java vendor string
    (*env)->DeleteLocalRef(env, vendorString);
}

//Starts reading the header information
void onStartReadingHeader(JNIEnv *env, jobject *opusDataFeed, jmethodID* startReadingHeaderMethodId) {
    LOGI(LOG_TAG, "Notifying decode feed to onStart reading the header");

    //Call header onStart reading method
    (*env)->CallVoidMethod(env, (*opusDataFeed), (*startReadingHeaderMethodId));
}


//onStartReadingHeader(env, &opusDataFeed, &startReadingHeaderMethodId);
JNIEXPORT int JNICALL Java_org_xiph_opus_decoderjni_OpusDecoder_initJni(JNIEnv *env, jclass cls, int debug0) {
	debug = debug0;
	LOGI(LOG_TAG, "initJni called, initing methods");
}


/*Process an Opus header and setup the opus decoder based on it.
  It takes several pointers for header values which are needed
  elsewhere in the code.*/
static OpusDecoder *process_header(ogg_packet *op, int *rate, int *channels, int *preskip, int quiet) {
	int err;
	OpusDecoder *st;
	OpusHeader header;

	if (opus_header_parse(op->packet, op->bytes, &header) == 0) {
		LOGE(LOG_TAG, "Cannot parse header");
		return NULL;
	} else
		LOGD(LOG_TAG, "Header parsed: ch:%d samplerate:%d", header.channels, header.input_sample_rate);
	*channels = header.channels;

	// validate sample rate: If the rate is unspecified we decode to 48000
	if (!*rate) *rate = header.input_sample_rate;
	if (*rate == 0) *rate = 48000;
	if (*rate < 8000 || *rate > 192000) {
		LOGE(LOG_TAG, "Invalid input_rate %d, defaulting to 48000 instead.",*rate);
		*rate = 48000;
	}

	*preskip = header.preskip;
	st = opus_decoder_create(*rate, header.channels, &err); // was 48000
	if (err != OPUS_OK)	{
		LOGE(LOG_TAG, "Cannot create decoder: %s", opus_strerror(err));
		return NULL;
	}
	if (!st) {
		LOGE(LOG_TAG, "Decoder initialization failed: %s", opus_strerror(err));
		return NULL;
	}

	if (header.gain != 0) {
		/*Gain API added in a newer libopus version, if we don't have it
		 we apply the gain ourselves. We also add in a user provided
		 manual gain at the same time.*/
		int gainadj = (int) header.gain;
		err = opus_decoder_ctl(st, OPUS_SET_GAIN(gainadj));
		if (err != OPUS_OK) {
		  LOGE(LOG_TAG, "Error setting gain: %s", opus_strerror(err));
		  return NULL;
		}
	}

	if (!quiet) {
		LOGE(LOG_TAG, "Decoding to %d Hz (%d channels)", *rate, *channels);
		if (header.version != 1) LOGE(LOG_TAG, "Header v%d",header.version);

		if (header.gain != 0) {
			LOGE(LOG_TAG, "Playback gain: %f dB\n", header.gain / 256.);
		}
	}

	return st;
}

JNIEXPORT int JNICALL Java_org_xiph_opus_decoderjni_OpusDecoder_readDecodeWriteLoop(JNIEnv *env, jclass cls, jobject opusDataFeed) {
	LOGI(LOG_TAG, "startDecoding called, initing buffers");

	//Create a new java byte array to pass to the opus data feed method
	jbyteArray jByteArrayReadBuffer = (*env)->NewByteArray(env, BUFFER_LENGTH);

	//Create our write buffer
	jshortArray jShortArrayWriteBuffer = (*env)->NewShortArray(env, BUFFER_LENGTH*2);

        //-- get Java layer method pointer --//
	jclass opusDataFeedClass = (*env)->FindClass(env, "org/xiph/opus/decoderjni/DecodeFeed");


	//Find our java method id's we'll be calling
	jmethodID readOpusDataMethodId = (*env)->GetMethodID(env, opusDataFeedClass, "onReadOpusData", "([BI)I");
	jmethodID writePCMDataMethodId = (*env)->GetMethodID(env, opusDataFeedClass, "onWritePCMData", "([SI)V");
	jmethodID startMethodId = (*env)->GetMethodID(env, opusDataFeedClass, "onStart", "(Lorg/xiph/opus/decoderjni/DecodeStreamInfo;)V");
	jmethodID startReadingHeaderMethodId = (*env)->GetMethodID(env, opusDataFeedClass, "onStartReadingHeader", "()V");
	jmethodID stopMethodId = (*env)->GetMethodID(env, opusDataFeedClass, "onStop", "()V");
	//--
    ogg_int16_t convbuffer[BUFFER_LENGTH]; /* take 8k out of the data segment, not the stack */
    int convsize=BUFFER_LENGTH;
    
    ogg_sync_state   oy; /* sync and verify incoming physical bitstream */
    ogg_stream_state os; /* take physical pages, weld into a logical stream of packets */
    ogg_page         og; /* one Ogg bitstream page. Opus packets are inside */
    ogg_packet       op; /* one raw packet of data for decode */
    

    char *buffer;
    int  bytes;

    /********** Decode setup ************/
	/* 20ms at 48000, TODO 120ms */
	#define MAX_FRAME_SIZE      960
	#define OPUS_STACK_SIZE     31684 /**/

	/* global data */

	int frame_size =0;
	OpusDecoder *st = NULL;
	opus_int64 packet_count;
	int stream_init = 0;
	int eos = 0;
	int channels = 0;
	int rate = 0;
	int preskip = 0;
	int gran_offset = 0;
	int has_opus_stream = 0;
	ogg_int32_t opus_serialno = 0;
	int proccessing_page = 0;
	//
	char title[40];
	char artist[40];
	char album[40];
	char notes[128];
	char year[5];
	//
	char error_string[128];
    //Notify the decode feed we are starting to initialize
    onStartReadingHeader(env, &opusDataFeed, &startReadingHeaderMethodId);

    //1
    ogg_sync_init(&oy); /* Now we can read pages */

    int inited = 0, header = OPUS_HEADERS;

    int err = SUCCESS;
    int i;

    // start source reading / decoding loop
    while (1) {
    	if (err != SUCCESS) {
    		LOGE(LOG_TAG, "Global loop closing for error: %d", err);
    		break;
    	}

        // READ DATA : submit a 4k block to Ogg layer
        buffer = ogg_sync_buffer(&oy,BUFFER_LENGTH);
        bytes = onReadOpusDataFromOpusDataFeed(env, &opusDataFeed, &readOpusDataMethodId, buffer, &jByteArrayReadBuffer);
        ogg_sync_wrote(&oy,bytes);

        // Check available data
        if (bytes == 0) {
        	LOGW(LOG_TAG, "Data source finished.");
        	err = SUCCESS;
        	break;
        }

        // loop pages
        while (1) {
        	// exit loop on error
        	if (err != SUCCESS) break;
        	// sync the stream and get a page
        	int result = ogg_sync_pageout(&oy,&og);
        	// need more data, so go to PREVIOUS loop and read more */
        	if (result == 0) break;
           	// missing or corrupt data at this page position
           	if (result < 0) {
        		LOGW(LOG_TAG, "Corrupt or missing data in bitstream; continuing..");
        		continue;
           	}
           	// we finally have a valid page
			if (!inited) {
				ogg_stream_init(&os, ogg_page_serialno(&og));
				LOGE(LOG_TAG, "inited stream, serial no: %ld", os.serialno);
				inited = 1;
				// reinit header flag here
				header = OPUS_HEADERS;

			}
			//  add page to bitstream: can safely ignore errors at this point
			if (ogg_stream_pagein(&os, &og) < 0) LOGE(LOG_TAG, "error 5 pagein");

			// consume all , break for error
			while (1) {
				result = ogg_stream_packetout(&os,&op);

				if(result == 0) break; // need more data so exit and go read data in PREVIOUS loop
				if(result < 0) continue; // missing or corrupt data at this page position , drop here or tolerate error?


				// decode available data
				if (header == 0) {
					int ret = opus_decode(st, (unsigned char*) op.packet, op.bytes, convbuffer, MAX_FRAME_SIZE, 0);

					/*If the decoder returned less than zero, we have an error.*/
					if (ret < 0) {
						LOGE(LOG_TAG,"Decoding error: %s", opus_strerror(ret));
						err = OPUS_DECODE_ERROR;
						break;
					}
					frame_size = (ret < convsize?ret : convsize);
					onWritePCMDataFromOpusDataFeed(env, &opusDataFeed, &writePCMDataMethodId, convbuffer, channels*frame_size, &jShortArrayWriteBuffer);


				} // decoding done

				// do we need the header? that's the first thing to take
				if (header > 0) {
					if (header == OPUS_HEADERS) { // first header
						//if (op.b_o_s && op.bytes >= 8 && !memcmp(op.packet, "OpusHead", 8)) {
						if (op.bytes < 8 || memcmp(op.packet, "OpusHead", 8) != 0) {
							err = NOT_OPUS_HEADER;
							break;
						}
						// prepare opus structures
						st = process_header(&op, &rate, &channels, &preskip, 0);
					}
					if (header == OPUS_HEADERS -1) { // second and last header, read comments

					}
					// we need to do this 2 times, for all 2 opus headers! add data to header structure

					// signal next header
					header--;

					// we got all opus headers
					if (header == 0) {
						//  header ready , call player to pass stream details and init AudioTrack
						onStart(env, &opusDataFeed, &startMethodId, rate, channels, "");// vi.rate, vi.channels, vc.vendor);
					}
				} // header decoding

				// while packets

				// check stream end
				if (ogg_page_eos(&og)) {
					LOGE(LOG_TAG, "Stream finished.");
					// clean up this logical bitstream;
					ogg_stream_clear(&os);

					// attempt to go for re-initialization until EOF in data source
					err = SUCCESS;

					inited = 0;
					break;
				}
			}
        	// page if
        } // while pages

    }



    // ogg_page and ogg_packet structs always point to storage in libopus.  They're never freed or manipulated directly


    // OK, clean up the framer
    ogg_sync_clear(&oy);

    onStopDecodeFeed(env, &opusDataFeed, &stopMethodId);

    //Clean up our buffers
    (*env)->DeleteLocalRef(env, jByteArrayReadBuffer);
    (*env)->DeleteLocalRef(env, jShortArrayWriteBuffer);

    return err;
}
