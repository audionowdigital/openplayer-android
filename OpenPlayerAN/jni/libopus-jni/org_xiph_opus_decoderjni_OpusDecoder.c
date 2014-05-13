/* Takes a vorbis bitstream from java callbacks from JNI and writes raw stereo PCM to
the jni callbacks. Decodes simple and chained OggVorbis files from beginning
to end. */

#include "org_xiph_opus_decoderjni_OpusDecoder.h"

/*Define message codes*/
#define INVALID_OGG_BITSTREAM -21
#define ERROR_READING_FIRST_PAGE -22
#define ERROR_READING_INITIAL_HEADER_PACKET -23
#define NOT_OPUS_HEADER -24
#define CORRUPT_SECONDARY_HEADER -25
#define PREMATURE_END_OF_FILE -26
#define SUCCESS 0

#define BUFFER_LENGTH 4096

extern void _VDBG_dump(void);

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

//Stops the vorbis data feed
void onStopDecodeFeed(JNIEnv *env, jobject* encDataFeed, jmethodID* stopMethodId) {
    (*env)->CallVoidMethod(env, (*encDataFeed), (*stopMethodId));
}

//Reads raw vorbis data from the jni callback
int onReadOpusDataFromOpusDataFeed(JNIEnv *env, jobject* encDataFeed, jmethodID* readOpusDataMethodId, char* buffer, jbyteArray* jByteArrayReadBuffer) {
    //Call the read method
	LOGI(LOG_TAG, "onReadOpusDataFromOpusDataFeed call.");
    int readByteCount = (*env)->CallIntMethod(env, (*encDataFeed), (*readOpusDataMethodId), (*jByteArrayReadBuffer), BUFFER_LENGTH);
    LOGI(LOG_TAG, "onReadOpusDataFromOpusDataFeed method called %d", readByteCount);
    //Don't bother copying, just return 0
    if(readByteCount == 0) {
        return 0;
    }

    //Gets the bytes from the java array and copies them to the vorbis buffer
    jbyte* readBytes = (*env)->GetByteArrayElements(env, (*jByteArrayReadBuffer), NULL);
    memcpy(buffer, readBytes, readByteCount);
    
    //Clean up memory and return how much data was read
    (*env)->ReleaseByteArrayElements(env, (*jByteArrayReadBuffer), readBytes, JNI_ABORT);

    //Return the amount actually read
    return readByteCount;
}

//Writes the pcm data to the Java layer
void onWritePCMDataFromOpusDataFeed(JNIEnv *env, jobject* encDataFeed, jmethodID* writePCMDataMethodId, ogg_int16_t* buffer, int bytes, jshortArray* jShortArrayWriteBuffer) {
    
    //No data to read, just exit
    if(bytes == 0) {
        return;
    }
    //LOGI(LOG_TAG, "onWritePCMDataFromOpusDataFeed %d", bytes);

    //Copy the contents of what we're writing to the java short array
    (*env)->SetShortArrayRegion(env, (*jShortArrayWriteBuffer), 0, bytes, (jshort *)buffer);
    
    //Call the write pcm data method
    (*env)->CallVoidMethod(env, (*encDataFeed), (*writePCMDataMethodId), (*jShortArrayWriteBuffer), bytes);
}

//jclass 	//decodeStreamInfoClass,
	//	encDataFeedClass;

//Starts the decode feed with the necessary information about sample rates, channels, etc about the stream
void onStart(JNIEnv *env, jobject *encDataFeed, jmethodID* startMethodId, long sampleRate, long channels, char* vendor) {
    LOGI(LOG_TAG, "Notifying decode feed");

    //Creates a java string for the vendor
    jstring vendorString = (*env)->NewStringUTF(env, vendor);

    //Get decode stream info class and constructor

    //jclass tmp = (*env)->FindClass(env, "org/xiph/vorbis/decoderjni/DecodeStreamInfo");
    //decodeStreamInfoClass = (jclass)(*env)->NewGlobalRef(env, tmp);
    jclass decodeStreamInfoClass = (*env)->FindClass(env, "org/xiph/opus/decoderjni/DecodeStreamInfo");

    jmethodID constructor = (*env)->GetMethodID(env, decodeStreamInfoClass, "<init>", "(JJLjava/lang/String;)V");

    //Create the decode stream info object
    jobject decodeStreamInfo = (*env)->NewObject(env, decodeStreamInfoClass, constructor, (jlong)sampleRate, (jlong)channels, vendorString);

    //Call decode feed onStart
    (*env)->CallVoidMethod(env, (*encDataFeed), (*startMethodId), decodeStreamInfo);

    //Cleanup decode feed object
    (*env)->DeleteLocalRef(env, decodeStreamInfo);

    //Cleanup java vendor string
    (*env)->DeleteLocalRef(env, vendorString);
}

//Starts reading the header information
void onStartReadingHeader(JNIEnv *env, jobject *encDataFeed, jmethodID* startReadingHeaderMethodId) {
    LOGI(LOG_TAG, "Notifying decode feed to onStart reading the header");

    //Call header onStart reading method
    (*env)->CallVoidMethod(env, (*encDataFeed), (*startReadingHeaderMethodId));
}

//Starts reading the header information
void onNewIteration(JNIEnv *env, jobject *encDataFeed, jmethodID* newIterationMethodId) {
    //LOGI(LOG_TAG, "Notifying decode feed to onNewIteration");

    //Call header onStart reading method
    (*env)->CallVoidMethod(env, (*encDataFeed), (*newIterationMethodId));
}

//jobject encDataFeed;
//jmethodID readOpusDataMethodId, writePCMDataMethodId, startMethodId, startReadingHeaderMethodId, stopMethodId, newIterationMethodId;

//onStartReadingHeader(env, &encDataFeed, &startReadingHeaderMethodId);
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
	/*
	#ifdef NONTHREADSAFE_PSEUDOSTACK
	global_stack = user_malloc(OPUS_STACK_SIZE);
	#endif
	*/

	if (opus_header_parse(op->packet, op->bytes, &header) == 0) {
		LOGE(LOG_TAG, "Cannot parse header");
		return NULL;
	} else
		LOGD(LOG_TAG, "header details: ch:%d samplerate:%d", header.channels, header.input_sample_rate);
	*channels = header.channels;

	if (!*rate) *rate = header.input_sample_rate;
	/* validate sample rate */
	/* If the rate is unspecified we decode to 48000 */
	if (*rate == 0) *rate = 48000;
	if (*rate < 8000 || *rate > 192000) {
		LOGE(LOG_TAG, "Invalid input_rate %d, defaulting to 48000 instead.",*rate);
		*rate = 48000;
	}

	*preskip = header.preskip;
	st = opus_decoder_create(48000, header.channels, &err);
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

JNIEXPORT int JNICALL Java_org_xiph_opus_decoderjni_OpusDecoder_readDecodeWriteLoop(JNIEnv *env, jclass cls, jobject encDataFeed) {
	LOGI(LOG_TAG, "startDecoding called, initing buffers ");

    //Create a new java byte array to pass to the vorbis data feed method
    jbyteArray jByteArrayReadBuffer = (*env)->NewByteArray(env, BUFFER_LENGTH);

    //Create our write buffer
    jshortArray jShortArrayWriteBuffer = (*env)->NewShortArray(env, BUFFER_LENGTH*2);

    //-- get Java layer method pointer --//
	jclass encDataFeedClass = (*env)->FindClass(env, "org/xiph/opus/decoderjni/DecodeFeed");


	//Find our java method id's we'll be calling
	jmethodID readOpusDataMethodId = (*env)->GetMethodID(env, encDataFeedClass, "onReadOpusData", "([BI)I");
	jmethodID writePCMDataMethodId = (*env)->GetMethodID(env, encDataFeedClass, "onWritePCMData", "([SI)V");
	jmethodID startMethodId = (*env)->GetMethodID(env, encDataFeedClass, "onStart", "(Lorg/xiph/opus/decoderjni/DecodeStreamInfo;)V");
	jmethodID startReadingHeaderMethodId = (*env)->GetMethodID(env, encDataFeedClass, "onStartReadingHeader", "()V");
	jmethodID stopMethodId = (*env)->GetMethodID(env, encDataFeedClass, "onStop", "()V");
	jmethodID newIterationMethodId = (*env)->GetMethodID(env, encDataFeedClass, "onNewIteration", "()V");
    //--
    ogg_int16_t convbuffer[BUFFER_LENGTH]; /* take 8k out of the data segment, not the stack */
    int convsize=BUFFER_LENGTH;
    
    ogg_sync_state   oy; /* sync and verify incoming physical bitstream */
    ogg_stream_state os; /* take physical pages, weld into a logical stream of packets */
    ogg_page         og; /* one Ogg bitstream page. Opus packets are inside */
    ogg_packet       op; /* one raw packet of data for decode */
    
    // OPUS stuff


    char *buffer;
    int  bytes;

    /********** Decode setup ************/

    

    /* 20ms at 48000, TODO 120ms */
    #define MAX_FRAME_SIZE      960
    #define OPUS_STACK_SIZE     31684 /**/

    /* global data */
    opus_int16 *output;
    int frame_size =0;
    OpusDecoder *st = NULL;
    opus_int64 packet_count;
	int total_links =0;
	int stream_init = 0;
	int quiet = 0;
	ogg_int64_t page_granule;
	ogg_int64_t end_granule;
	ogg_int64_t link_out;
	int eos = 0;
	ogg_int64_t audio_size;
	double last_coded_seconds;
	int channels = 0;
	int rate = 0;
	int preskip = 0;
	int gran_offset = 0;
	int has_opus_stream = 0;
	ogg_int32_t opus_serialno;
	int proccessing_page = 0;
	int seeking;
	opus_int64 maxout;
	void *ogg_buf;
	int ogg_buf_size;
	// decode: metadata
	int channel_count = 0;
	int bitrate;
	//
	int mstime_max, mstime_curr, time_curr;
	//
	char title[40];
	char artist[40];
	char album[40];
	char notes[128];
	char year[5];
	//
	char error_string[128];
	//
	//Notify the decode feed we are starting to initialize
	onStartReadingHeader(env, &encDataFeed, &startReadingHeaderMethodId);
	ogg_sync_init(&oy); /* Now we can read pages */
	//
    while(1) {
    	/* grab some data at the head of the stream. We want the first page
        (which is guaranteed to be small and only contain the Opus
        stream initial header) We need the first page to get the stream
        serialno - similar to Vorbis logic */
    	// READ DATA
		/* submit a 4k block to libopus' Ogg layer */
		LOGI(LOG_TAG, "Submitting 4k block");
		buffer = ogg_sync_buffer(&oy,BUFFER_LENGTH);
		bytes = onReadOpusDataFromOpusDataFeed(env, &encDataFeed, &readOpusDataMethodId, buffer, &jByteArrayReadBuffer);
		ogg_sync_wrote(&oy,bytes);
		/* Get the first page. */
		LOGD(LOG_TAG, "Getting the first page, read (%d) bytes", bytes);

		if (1) {
			if(ogg_sync_pageout(&oy,&og) != 1) {
				/* have we simply run out of data?  If so, we're done. */
				if(bytes<BUFFER_LENGTH) break;
				/* error case.  Must not be Opus data */
				onStopDecodeFeed(env, &encDataFeed, &stopMethodId);
				return INVALID_OGG_BITSTREAM;
			}
			LOGI(LOG_TAG, "Successfully fetched the first page");

			/* Get the serial number and set up the rest of decode. */
			/* serialno first; use it to set up a logical stream */
			ogg_stream_init(&os,ogg_page_serialno(&og));

			if(ogg_stream_pagein(&os,&og) < 0){
				// error; stream version mismatch perhaps
				onStopDecodeFeed(env, &encDataFeed, &stopMethodId);
				return ERROR_READING_FIRST_PAGE;
			}
			stream_init = 1;
		}


		if(ogg_stream_packetout(&os,&op)!=1){
			/* no page? must not be vorbis */
			onStopDecodeFeed(env, &encDataFeed, &stopMethodId);
			return ERROR_READING_INITIAL_HEADER_PACKET;
		}

		LOGD(LOG_TAG, "packets:%d %d", op.bytes, op.packetno);

		if (!proccessing_page) {
			proccessing_page = 1;
			LOGE(LOG_TAG,"checking header");
			/*OggOpus streams are identified by a magic string in the initial  stream header.*/
			if (op.b_o_s && op.bytes >= 8 && !memcmp(op.packet, "OpusHead", 8))
			{
				packet_count = 0;
				eos = 0;
				opus_serialno = os.serialno;
				has_opus_stream = 1;
				LOGD(LOG_TAG, "Header received, stream serial number: %d packetno: %d" , os.serialno, op.packetno);
			}

			/* Discard packets with the wrong serial no */
			if (!has_opus_stream && os.serialno != opus_serialno) {
				onStopDecodeFeed(env, &encDataFeed, &stopMethodId);
				return NOT_OPUS_HEADER;
			}

			st = process_header(&op, &rate, &channels, &preskip, quiet);

			// os, og, op
			//  ogg_stream_state os; /* take physical pages, weld into a logical stream of packets */
			//   ogg_page         og; /* one Ogg bitstream page. Opus packets are inside */
			//   ogg_packet       op; /* one raw packet of data for decode */
			LOGI(LOG_TAG, "Data: %d %d %d %d", rate, channels, preskip, quiet);

			onStart(env, &encDataFeed, &startMethodId, rate, channels, "opus");
		}


    }
    /* OK, clean up the framer */
    ogg_sync_clear(&oy);



    onStopDecodeFeed(env, &encDataFeed, &stopMethodId);

    //Clean up our buffers
    (*env)->DeleteLocalRef(env, jByteArrayReadBuffer);
    (*env)->DeleteLocalRef(env, jShortArrayWriteBuffer);

    return SUCCESS;
}
