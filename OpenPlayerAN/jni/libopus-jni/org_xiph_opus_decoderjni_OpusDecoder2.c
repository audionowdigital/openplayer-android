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
	//LOGI(LOG_TAG, "onReadOpusDataFromOpusDataFeed call.");
    int readByteCount = (*env)->CallIntMethod(env, (*encDataFeed), (*readOpusDataMethodId), (*jByteArrayReadBuffer), BUFFER_LENGTH);
    //LOGI(LOG_TAG, "onReadOpusDataFromOpusDataFeed method called %d", readByteCount);
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
	//
    //
	//Notify the decode feed we are starting to initialize
	onStartReadingHeader(env, &encDataFeed, &startReadingHeaderMethodId);
	ogg_sync_init(&oy); /* Now we can read pages */
	while(1) {
    	/* grab some data at the head of the stream. We want the first page
        (which is guaranteed to be small and only contain the Opus
        stream initial header) We need the first page to get the stream
        serialno - similar to Vorbis logic */

    	// READ DATA
		/* submit a 4k block to libopus' Ogg layer */
		//LOGI(LOG_TAG, "Submitting 1 4k block");
		buffer = ogg_sync_buffer(&oy,BUFFER_LENGTH);
		bytes = onReadOpusDataFromOpusDataFeed(env, &encDataFeed, &readOpusDataMethodId, buffer, &jByteArrayReadBuffer);
		ogg_sync_wrote(&oy,bytes);
		if (bytes > 0)
				LOGE(LOG_TAG, "%d bytes, frames:%d processingp:%d packetcount:%d", bytes, frame_size, proccessing_page);
		else { //we are done
			 onStopDecodeFeed(env, &encDataFeed, &stopMethodId);
			 return PREMATURE_END_OF_FILE;
		}

		if (!frame_size) /* already have decoded frame */
		{
			if (proccessing_page) {
				if (ogg_stream_packetout(&os, &op) == 1) {

					LOGD(LOG_TAG, "ogg stream: bytes=%d bos=%d hasopusstream=%d",
							op.bytes, op.b_o_s, has_opus_stream);

					/*OggOpus streams are identified by a magic string in the initial
					 stream header.*/
					if (op.b_o_s && op.bytes >= 8 && !memcmp(op.packet, "OpusHead", 8)) {
						if (!has_opus_stream) {
							opus_serialno = os.serialno;
							has_opus_stream = 1;
							packet_count = 0;
							eos = 0;
							LOGE(LOG_TAG, "header found:%ld" , opus_serialno);
						} else {
							//LOGE(LOG_TAG, "Warning: ignoring opus stream %ld", os->serialno);
							LOGE(LOG_TAG, "ignoring opus stream");
						}
					}
					if (!has_opus_stream || os.serialno != opus_serialno) {
						LOGE(LOG_TAG, "not has_opus_stream OR  os->serialno not opus_serialno");
						//onStopDecodeFeed(env, &encDataFeed, &stopMethodId);
						//return NOT_OPUS_HEADER; // TODO: check error
					}

					/*If first packet in a logical stream, process the Opus header*/
					if (packet_count == 0) {
						LOGD(LOG_TAG, "prepare to process header");

						st = process_header(&op, &rate, &channels, &preskip, 0);
						if (!st) {
							LOGE(LOG_TAG, "invalid header - unable to process what we got");

							onStopDecodeFeed(env, &encDataFeed, &stopMethodId);
							return NOT_OPUS_HEADER;
						}
						// prepare to decode, no comments yet
						onStart(env, &encDataFeed, &startMethodId, rate, channels, "");// vi.rate, vi.channels, vc.vendor);


						if (ogg_stream_packetout(&os, &op) != 0 || og.header[og.header_len - 1] == 255) {
							/*The format specifies that the initial header and tags packets are on their
							 own pages. To aid implementors in discovering that their files are wrong
							 we reject them explicitly here. In some player designs files like this would
							 fail even without an explicit test.*/
							LOGE(LOG_TAG, "Extra packets on initial header page. Invalid stream.");
							onStopDecodeFeed(env, &encDataFeed, &stopMethodId);
							return NOT_OPUS_HEADER;
						}

						//output = user_malloc(sizeof(opus_int16) * MAX_FRAME_SIZE * channels);
						output = (opus_int16 *)malloc(sizeof(opus_int16) * MAX_FRAME_SIZE * channels);
						if (!output) {
							// fatal error, can't allocate memory for decoding
							LOGE(LOG_TAG, "fatal error no memory for decoding");
							onStopDecodeFeed(env, &encDataFeed, &stopMethodId);
							return NOT_OPUS_HEADER;
						}
					} else if (packet_count == 1) {
						LOGE(LOG_TAG, "read comments here - additional data from header");
						// read additional data from header
						// if (!quiet) print_comments(psDecoderContext, (char*) op.packet, op.bytes);
					} else {
						int ret;
						LOGE(LOG_TAG, "go for decode:");
						/*End of stream condition*/
						if (op.e_o_s && os.serialno == opus_serialno)
							eos = 1; /* don't care for anything except opus eos */

						ret = opus_decode(st, (unsigned char*) op.packet, op.bytes, output, MAX_FRAME_SIZE, 0);

						/*If the decoder returned less than zero, we have an error.*/
						if (ret < 0) {
							LOGE(LOG_TAG,"Decoding error: %s", opus_strerror(ret));
							onStopDecodeFeed(env, &encDataFeed, &stopMethodId);
							return PREMATURE_END_OF_FILE;
						}
						LOGE(LOG_TAG, "decoded:%d from %d", ret, op.bytes);
						frame_size = ret;

					}
					packet_count++;
				} // stream packet out 1
				else {
					/* End of packets in page */
					proccessing_page = 0;
				}
			} //if processing page

			/*We're done*/
			if (eos) {
				has_opus_stream = 0;
			} else {
				LOGD(LOG_TAG, "NOT EOS");

				if (!proccessing_page && ogg_sync_pageout(&oy, &og) == 1) {
					if (stream_init == 0) {
						LOGD(LOG_TAG, "stream_init");
						ogg_stream_init(&os, ogg_page_serialno(&og));
						stream_init = 1;
					}
					if (ogg_page_serialno(&og) != os.serialno) {
						/* so all streams are read. */
						LOGD(LOG_TAG, "reset serial no");

						ogg_stream_reset_serialno(&os, ogg_page_serialno(&og));
					}
					/*Add page to the bitstream*/
					ogg_stream_pagein(&os, &og);

					proccessing_page = 1;
				} else {
					LOGD(LOG_TAG, "NEED MORE DATA");

				}
			} // check we're done
		}


		/* already have decoded frame, go ahead and play */
		if (frame_size)
		{
			LOGE(LOG_TAG, "play:%d", frame_size);
			int bout=(frame_size<convsize?frame_size:convsize);
			onWritePCMDataFromOpusDataFeed(env, &encDataFeed, &writePCMDataMethodId, output, channels*bout, &jShortArrayWriteBuffer);

			frame_size = 0; /* we have consumed that last decoded frame */
		}
	} // big while loop
	// TODO: where do we break??

    /* OK, clean up the framer */
    ogg_sync_clear(&oy);



    onStopDecodeFeed(env, &encDataFeed, &stopMethodId);

    //Clean up our buffers
    (*env)->DeleteLocalRef(env, jByteArrayReadBuffer);
    (*env)->DeleteLocalRef(env, jShortArrayWriteBuffer);

    return SUCCESS;
}
