/* Takes a vorbis bitstream from java callbacks from JNI and writes raw stereo PCM to
the jni callbacks. Decodes simple and chained OggVorbis files from beginning
to end. */

#include "org_xiph_vorbis_decoderjni_VorbisDecoder.h"

/*Define message codes*/
#define NOT_VORBIS_HEADER -1
#define CORRUPT_HEADER -2
#define SUCCESS 0

#define VORBIS_HEADERS 3

#define BUFFER_LENGTH 4096

//extern void _VDBG_dump(void);

int debug = 0;

#define LOG_TAG "jniVorbisDecoder"
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
void onStopDecodeFeed(JNIEnv *env, jobject* vorbisDataFeed, jmethodID* stopMethodId) {
    (*env)->CallVoidMethod(env, (*vorbisDataFeed), (*stopMethodId));
}

//Reads raw vorbis data from the jni callback
int onReadVorbisDataFromVorbisDataFeed(JNIEnv *env, jobject* vorbisDataFeed, jmethodID* readVorbisDataMethodId, char* buffer, jbyteArray* jByteArrayReadBuffer) {
    //Call the read method
    int readByteCount = (*env)->CallIntMethod(env, (*vorbisDataFeed), (*readVorbisDataMethodId), (*jByteArrayReadBuffer), BUFFER_LENGTH);
    
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
void onWritePCMDataFromVorbisDataFeed(JNIEnv *env, jobject* vorbisDataFeed, jmethodID* writePCMDataMethodId, ogg_int16_t* buffer, int bytes, jshortArray* jShortArrayWriteBuffer) {
    
    //No data to read, just exit
    if(bytes == 0) return;

    //LOGI(LOG_TAG, "onWritePCMDataFromVorbisDataFeed %d", bytes);

    //Copy the contents of what we're writing to the java short array
    (*env)->SetShortArrayRegion(env, (*jShortArrayWriteBuffer), 0, bytes, (jshort *)buffer);
    
    //Call the write pcm data method
    (*env)->CallVoidMethod(env, (*vorbisDataFeed), (*writePCMDataMethodId), (*jShortArrayWriteBuffer), bytes);
}

//jclass 	//decodeStreamInfoClass,
	//	vorbisDataFeedClass;

//Starts the decode feed with the necessary information about sample rates, channels, etc about the stream
void onStart(JNIEnv *env, jobject *vorbisDataFeed, jmethodID* startMethodId, long sampleRate, long channels, char* vendor) {
    LOGI(LOG_TAG, "onStart call.");

    //Creates a java string for the vendor
    jstring vendorString = (*env)->NewStringUTF(env, vendor);

    //Get decode stream info class and constructor

    //jclass tmp = (*env)->FindClass(env, "org/xiph/vorbis/decoderjni/DecodeStreamInfo");
    //decodeStreamInfoClass = (jclass)(*env)->NewGlobalRef(env, tmp);
    jclass decodeStreamInfoClass = (*env)->FindClass(env, "org/xiph/vorbis/decoderjni/DecodeStreamInfo");

    jmethodID constructor = (*env)->GetMethodID(env, decodeStreamInfoClass, "<init>", "(JJLjava/lang/String;)V");

    //Create the decode stream info object
    jobject decodeStreamInfo = (*env)->NewObject(env, decodeStreamInfoClass, constructor, (jlong)sampleRate, (jlong)channels, vendorString);

    //Call decode feed onStart
    (*env)->CallVoidMethod(env, (*vorbisDataFeed), (*startMethodId), decodeStreamInfo);

    //Cleanup decode feed object
    (*env)->DeleteLocalRef(env, decodeStreamInfo);

    //Cleanup java vendor string
    (*env)->DeleteLocalRef(env, vendorString);
}

//Starts reading the header information
void onStartReadingHeader(JNIEnv *env, jobject *vorbisDataFeed, jmethodID* startReadingHeaderMethodId) {
    LOGI(LOG_TAG, "onStartReadingHeader call .");

    //Call header onStart reading method
    (*env)->CallVoidMethod(env, (*vorbisDataFeed), (*startReadingHeaderMethodId));
}


//jobject vorbisDataFeed;
//jmethodID readVorbisDataMethodId, writePCMDataMethodId, startMethodId, startReadingHeaderMethodId, stopMethodId, newIterationMethodId;

//onStartReadingHeader(env, &vorbisDataFeed, &startReadingHeaderMethodId);
JNIEXPORT int JNICALL Java_org_xiph_vorbis_decoderjni_VorbisDecoder_initJni(JNIEnv *env, jclass cls, int debug0) {
	debug = debug0;
	LOGI(LOG_TAG, "initJni called, initing methods");
}

JNIEXPORT int JNICALL Java_org_xiph_vorbis_decoderjni_VorbisDecoder_readDecodeWriteLoop(JNIEnv *env, jclass cls, jobject vorbisDataFeed) {
	LOGI(LOG_TAG, "startDecoding called, initing buffers");

	//Create a new java byte array to pass to the vorbis data feed method
	jbyteArray jByteArrayReadBuffer = (*env)->NewByteArray(env, BUFFER_LENGTH);

	//Create our write buffer
	jshortArray jShortArrayWriteBuffer = (*env)->NewShortArray(env, BUFFER_LENGTH*2);

        //-- get Java layer method pointer --//
	jclass vorbisDataFeedClass = (*env)->FindClass(env, "org/xiph/vorbis/decoderjni/DecodeFeed");


	//Find our java method id's we'll be calling
	jmethodID readVorbisDataMethodId = (*env)->GetMethodID(env, vorbisDataFeedClass, "onReadVorbisData", "([BI)I");
	jmethodID writePCMDataMethodId = (*env)->GetMethodID(env, vorbisDataFeedClass, "onWritePCMData", "([SI)V");
	jmethodID startMethodId = (*env)->GetMethodID(env, vorbisDataFeedClass, "onStart", "(Lorg/xiph/vorbis/decoderjni/DecodeStreamInfo;)V");
	jmethodID startReadingHeaderMethodId = (*env)->GetMethodID(env, vorbisDataFeedClass, "onStartReadingHeader", "()V");
	jmethodID stopMethodId = (*env)->GetMethodID(env, vorbisDataFeedClass, "onStop", "()V");
	//--
    ogg_int16_t convbuffer[BUFFER_LENGTH]; /* take 8k out of the data segment, not the stack */
    int convsize=BUFFER_LENGTH;
    
    ogg_sync_state   oy; /* sync and verify incoming physical bitstream */
    ogg_stream_state os; /* take physical pages, weld into a logical stream of packets */
    ogg_page         og; /* one Ogg bitstream page. Vorbis packets are inside */
    ogg_packet       op; /* one raw packet of data for decode */
    
    vorbis_info      vi; /* struct that stores all the static vorbis bitstream settings */
    vorbis_comment   vc; /* struct that stores all the bitstream user comments */
    vorbis_dsp_state vd; /* central working state for the packet->PCM decoder */
    vorbis_block     vb; /* local working space for packet->PCM decode */
    
    char *buffer;
    int  bytes;
    
    /********** Decode setup ************/

    //Notify the decode feed we are starting to initialize
    onStartReadingHeader(env, &vorbisDataFeed, &startReadingHeaderMethodId);
    
    //1
    ogg_sync_init(&oy); /* Now we can read pages */
    
    int inited = 0, header = 3;
    int eos = 0;
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
        bytes = onReadVorbisDataFromVorbisDataFeed(env, &vorbisDataFeed, &readVorbisDataMethodId, buffer, &jByteArrayReadBuffer);
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
				header = VORBIS_HEADERS;

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
					float **pcm;
					int samples;
					// test for success!
					if(vorbis_synthesis(&vb,&op)==0) vorbis_synthesis_blockin(&vd,&vb);
					while((samples = vorbis_synthesis_pcmout(&vd,&pcm)) > 0) {
						//LOGE(LOG_TAG, "start while 8, decoding %d samples: %d convsize:%d", op.bytes,  samples, convsize);
						int j;
						int frame_size = (samples < convsize?samples : convsize);

						// convert floats to 16 bit signed ints (host order) and interleave
						for(i = 0; i < vi.channels; i++){
							ogg_int16_t *ptr = convbuffer + i;
							float  *mono = pcm[i];
							for(j=0;j<frame_size;j++){
								int val = floor(mono[j]*32767.f+.5f);
								// might as well guard against clipping
								if(val>32767) { val=32767; }
								if(val<-32768) { val=-32768; }
								*ptr=val;
								ptr += vi.channels;
							}
						}

						// Call decodefeed to push data to AudioTrack
						onWritePCMDataFromVorbisDataFeed(env, &vorbisDataFeed, &writePCMDataMethodId, convbuffer, frame_size*vi.channels, &jShortArrayWriteBuffer);
						vorbis_synthesis_read(&vd,frame_size); // tell libvorbis how many samples we actually consumed
					}
				} // decoding done

				// do we need the header? that's the first thing to take
				if (header > 0) {
					if (header == VORBIS_HEADERS) {
						// prepare vorbis structures
						vorbis_info_init(&vi);
						vorbis_comment_init(&vc);
					}
					// we need to do this 3 times, for all 3 vorbis headers!
					// add data to header structure
					if(vorbis_synthesis_headerin(&vi,&vc,&op) < 0) {
						// error case; not a vorbis header
						LOGE(LOG_TAG, "Err: not a vorbis header.");
						err = NOT_VORBIS_HEADER;
						break;
					}
					// signal next header
					header--;

					// we got all 3 vorbis headers
					if (header == 0) {
						LOGE(LOG_TAG, "Vorbis header data: ver:%d ch:%d samp:%ld [%s]" ,  vi.version, vi.channels, vi.rate, vc.vendor);
						int i=0;
						for (i=0; i<vc.comments; i++)
							LOGD(LOG_TAG,"Header comment:%d len:%d [%s]", i, vc.comment_lengths[i], vc.user_comments[i]);
						// init vorbis decoder
						if(vorbis_synthesis_init(&vd,&vi) != 0) {
							// corrupt header
							LOGE(LOG_TAG, "Err: corrupt header.");
							err = CORRUPT_HEADER;
							break;
						}
						// central decode state
						vorbis_block_init(&vd,&vb);

						// header ready , call player to pass stream details and init AudioTrack
						onStart(env, &vorbisDataFeed, &startMethodId, vi.rate, vi.channels, vc.vendor);
					}
				} // header decoding

				// while packets

				// check stream end
				if (ogg_page_eos(&og)) {
					LOGE(LOG_TAG, "Stream finished.");
					// clean up this logical bitstream;
					ogg_stream_clear(&os);
					vorbis_comment_clear(&vc);
					vorbis_info_clear(&vi);  // must be called last

					// clear decoding structures
					vorbis_block_clear(&vb);
					vorbis_dsp_clear(&vd);

					// attempt to go for re-initialization until EOF in data source
					err = SUCCESS;

					inited = 0;
					break;
				}
			}
        	// page if
        } // while pages

    }



    // ogg_page and ogg_packet structs always point to storage in libvorbis.  They're never freed or manipulated directly
	vorbis_comment_clear(&vc);
	vorbis_info_clear(&vi);  // must be called last

	vorbis_block_clear(&vb);
	vorbis_dsp_clear(&vd);


    // OK, clean up the framer
    ogg_sync_clear(&oy);

    onStopDecodeFeed(env, &vorbisDataFeed, &stopMethodId);

    //Clean up our buffers
    (*env)->DeleteLocalRef(env, jByteArrayReadBuffer);
    (*env)->DeleteLocalRef(env, jShortArrayWriteBuffer);

    return err;
}
