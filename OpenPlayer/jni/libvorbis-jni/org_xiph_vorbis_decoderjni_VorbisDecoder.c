/*
 * org_xiph_vorbis_decoderjni_VorbisDecoder.c
 * Takes a vorbis bitstream from java callbacks from JNI and writes raw stereo PCM to the jni callbacks. Decodes simple and chained OggOpus files from beginning to end.
 *
 * (C) 2014 Radu Motisan, radu.motisan@gmail.com
 *
 * Part of the OpenPlayer implementation for Alpine Audio Now Digital LLC
 */

#include "org_xiph_vorbis_decoderjni_VorbisDecoder.h"
#include "../decodefeed/Log.h"
#include "../decodefeed/DecodeFeed.h"

/*Define message codes*/
#define INVALID_HEADER -1
#define SUCCESS 0

#define VORBIS_HEADERS 3


#define COMMENT_MAX_LEN 40

//extern void _VDBG_dump(void);

int debug = 0;


#define max(a,b) ({ __typeof__ (a) _a = (a); __typeof__ (b) _b = (b); _a > _b ? _a : _b; })
#define min(a,b) ({ __typeof__ (a) _a = (a); __typeof__ (b) _b = (b); _a < _b ? _a : _b; })


JNIEXPORT jint JNI_OnLoad(JavaVM *vm, void *reserved) {
  LOGD(LOG_TAG, "onLoad called.");
	return JNI_VERSION_1_6;
}



//onStartReadingHeader(env, &javaDecodeFeedObj, &startReadingHeaderMethodId);
JNIEXPORT int JNICALL Java_org_xiph_vorbis_decoderjni_VorbisDecoder_initJni(JNIEnv *env, jclass cls, int debug0) {
	debug = debug0;
	LOGI(LOG_TAG, "initJni called, initing methods (vorbis)");
}

JNIEXPORT int JNICALL Java_org_xiph_vorbis_decoderjni_VorbisDecoder_readDecodeWriteLoop(JNIEnv *env, jclass cls, jobject javaDecodeFeedObj) {
	LOGI(LOG_TAG, "startDecoding called, initing buffers (vorbis)");

	//Create a new java byte array to pass to the vorbis data feed method
	jbyteArray jByteArrayReadBuffer = (*env)->NewByteArray(env, BUFFER_LENGTH);

	//Create our write buffer
	jshortArray jShortArrayWriteBuffer = (*env)->NewShortArray(env, BUFFER_LENGTH*2);

        //-- get Java layer method pointer --//
//	jclass javaDecodeFeedObjClass = (*env)->FindClass(env, "org/xiph/vorbis/decoderjni/DecodeFeed");
	jclass javaDecodeFeedObjClass = (*env)->FindClass(env, "com/audionowdigital/android/openplayer/DecodeFeed");

	// type signatures are presented here: http://docs.oracle.com/javase/7/docs/technotes/guides/jni/spec/types.html
	//Find our java method id's we'll be calling
	jmethodID readDataMethodId = (*env)->GetMethodID(env, javaDecodeFeedObjClass, "onReadEncodedData", "([BI)I");
	jmethodID writePCMDataMethodId = (*env)->GetMethodID(env, javaDecodeFeedObjClass, "onWritePCMData", "([SII)V");
	jmethodID startMethodId = (*env)->GetMethodID(env, javaDecodeFeedObjClass, "onStart", "(JJLjava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)V");
	jmethodID startReadingHeaderMethodId = (*env)->GetMethodID(env, javaDecodeFeedObjClass, "onStartReadingHeader", "()V");
	jmethodID stopMethodId = (*env)->GetMethodID(env, javaDecodeFeedObjClass, "onStop", "()V");
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
    

	char vendor[COMMENT_MAX_LEN] = {0};
	char title[COMMENT_MAX_LEN] = {0};
	char artist[COMMENT_MAX_LEN] = {0};
	char album[COMMENT_MAX_LEN] = {0};
	char date[COMMENT_MAX_LEN] = {0};
	char track[COMMENT_MAX_LEN] = {0};

    /********** Decode setup ************/

    //Notify the decode feed we are starting to initialize
    onStartReadingHeader(env, &javaDecodeFeedObj, &startReadingHeaderMethodId);
    
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
        bytes = onReadEncodedData(env, &javaDecodeFeedObj, &readDataMethodId, buffer, &jByteArrayReadBuffer);
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
						onWritePCMData(env, &javaDecodeFeedObj, &writePCMDataMethodId, convbuffer, frame_size*vi.channels, &jShortArrayWriteBuffer);
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
						err = INVALID_HEADER;
						break;
					}
					// signal next header
					header--;

					// we got all 3 vorbis headers
					if (header == 0) {
						LOGE(LOG_TAG, "Vorbis header data: ver:%d ch:%d samp:%ld [%s]" ,  vi.version, vi.channels, vi.rate, vc.vendor);
						int i=0;
						for (i=0; i<vc.comments; i++) {
							LOGD(LOG_TAG,"Header comment:%d len:%d [%s]", i, vc.comment_lengths[i], vc.user_comments[i]);
							char *c = vc.user_comments[i];
							int len = vc.comment_lengths[i];
							 // keys we are looking for in the comments, careful if size if bigger than 10
							char keys[5][10] = { "title=", "artist=", "album=", "date=", "track=" };
							char *values[5] = { title, artist, album, date, track }; // put the values in these pointers
							int j = 0;
							for (j = 0; j < 5; j++) { // iterate all keys
								int keylen = strlen(keys[j]);
								if (!strncasecmp(c, keys[j], keylen )) strncpy(values[j], c + keylen, min(len - keylen , COMMENT_MAX_LEN));
							}
						}
						// init vorbis decoder
						if(vorbis_synthesis_init(&vd,&vi) != 0) {
							// corrupt header
							LOGE(LOG_TAG, "Err: corrupt header.");
							err = INVALID_HEADER;
							break;
						}
						// central decode state
						vorbis_block_init(&vd,&vb);

						// header ready , call player to pass stream details and init AudioTrack
						onStart(env, &javaDecodeFeedObj, &startMethodId, vi.rate, vi.channels, vc.vendor,
								title, artist, album, date, track);
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

    onStop(env, &javaDecodeFeedObj, &stopMethodId);

    //Clean up our buffers
    (*env)->DeleteLocalRef(env, jByteArrayReadBuffer);
    (*env)->DeleteLocalRef(env, jShortArrayWriteBuffer);

    return err;
}
