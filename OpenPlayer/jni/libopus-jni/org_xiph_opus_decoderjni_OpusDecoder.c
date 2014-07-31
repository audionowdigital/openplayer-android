/* Takes a opus bitstream from java callbacks from JNI and writes raw stereo PCM to
the jni callbacks. Decodes simple and chained OggOpus files from beginning
to end. */

#include "org_xiph_opus_decoderjni_OpusDecoder.h"
#include "OpusHeader.h"
#include "../decodefeed/DecodeFeed.h"

#define OPUS_HEADERS 2


#define COMMENT_MAX_LEN 40

int debug = 0;

JNIEXPORT jint JNI_OnLoad(JavaVM *vm, void *reserved) {
  LOGD(LOG_TAG, "onLoad called.");
	return JNI_VERSION_1_6;
}


//onStartReadingHeader(env, &javaDecodeFeedObj, &startReadingHeaderMethodId);
JNIEXPORT int JNICALL Java_org_xiph_opus_decoderjni_OpusDecoder_initJni(JNIEnv *env, jclass cls, int debug0) {
	debug = debug0;
	LOGI(LOG_TAG, "initJni called, initing methods (opus)");
}




JNIEXPORT int JNICALL Java_org_xiph_opus_decoderjni_OpusDecoder_readDecodeWriteLoop(JNIEnv *env, jclass cls, jobject javaDecodeFeedObj) {
	LOGI(LOG_TAG, "startDecoding called, initing buffers (opus)");

	//Create a new java byte array to pass to the opus data feed method
	jbyteArray jByteArrayReadBuffer = (*env)->NewByteArray(env, BUFFER_LENGTH);

	//Create our write buffer
	jshortArray jShortArrayWriteBuffer = (*env)->NewShortArray(env, BUFFER_LENGTH*2);

    //-- get Java layer method pointer --//
	jclass javaDecodeFeedObjClass = (*env)->FindClass(env, "com/audionowdigital/android/openplayer/DecodeFeed");


	//Find our java method id's we'll be calling
	jmethodID readOpusDataMethodId = (*env)->GetMethodID(env, javaDecodeFeedObjClass, "onReadEncodedData", "([BI)I");
	jmethodID writePCMDataMethodId = (*env)->GetMethodID(env, javaDecodeFeedObjClass, "onWritePCMData", "([SI)V");
	jmethodID startMethodId = (*env)->GetMethodID(env, javaDecodeFeedObjClass, "onStart", "(JJLjava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)V");
	jmethodID startReadingHeaderMethodId = (*env)->GetMethodID(env, javaDecodeFeedObjClass, "onStartReadingHeader", "()V");
	jmethodID stopMethodId = (*env)->GetMethodID(env, javaDecodeFeedObjClass, "onStop", "()V");
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

	char vendor[COMMENT_MAX_LEN] = {0};
	char title[COMMENT_MAX_LEN] = {0};
	char artist[COMMENT_MAX_LEN] = {0};
	char album[COMMENT_MAX_LEN] = {0};
	char date[COMMENT_MAX_LEN] = {0};
	char track[COMMENT_MAX_LEN] = {0};

	//Notify the decode feed we are starting to initialize
    onStartReadingHeader(env, &javaDecodeFeedObj, &startReadingHeaderMethodId);

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
        bytes = onReadEncodedData(env, &javaDecodeFeedObj, &readOpusDataMethodId, buffer, &jByteArrayReadBuffer);
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
						err = DECODE_ERROR;
						break;
					}
					frame_size = (ret < convsize?ret : convsize);
					onWritePCMData(env, &javaDecodeFeedObj, &writePCMDataMethodId, convbuffer, channels*frame_size, &jShortArrayWriteBuffer);


				} // decoding done

				// do we need the header? that's the first thing to take
				if (header > 0) {
					if (header == OPUS_HEADERS) { // first header
						//if (op.b_o_s && op.bytes >= 8 && !memcmp(op.packet, "OpusHead", 8)) {
						if (op.bytes < 8 || memcmp(op.packet, "OpusHead", 8) != 0) {
							err = INVALID_HEADER;
							break;
						}
						// prepare opus structures
						st = process_header(&op, &rate, &channels, &preskip, 0);
					}
					if (header == OPUS_HEADERS -1) { // second and last header, read comments
						// err = we ignore comment errors
						process_comments((char *)op.packet, op.bytes, vendor, title, artist, album, date, track, COMMENT_MAX_LEN);

					}
					// we need to do this 2 times, for all 2 opus headers! add data to header structure

					// signal next header
					header--;

					// we got all opus headers
					if (header == 0) {
						//  header ready , call player to pass stream details and init AudioTrack
						onStart(env, &javaDecodeFeedObj, &startMethodId, rate, channels, vendor,
								title, artist, album, date, track);
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

    onStop(env, &javaDecodeFeedObj, &stopMethodId);

    //Clean up our buffers
    (*env)->DeleteLocalRef(env, jByteArrayReadBuffer);
    (*env)->DeleteLocalRef(env, jShortArrayWriteBuffer);

    return err;
}
