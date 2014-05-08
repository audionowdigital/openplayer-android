/* Takes a vorbis bitstream from java callbacks from JNI and writes raw stereo PCM to
the jni callbacks. Decodes simple and chained OggVorbis files from beginning
to end. */

#include "org_xiph_opus_decoderjni_OpusDecoder.h"

/*Define message codes*/
#define INVALID_OGG_BITSTREAM -21
#define ERROR_READING_FIRST_PAGE -22
#define ERROR_READING_INITIAL_HEADER_PACKET -23
#define NOT_VORBIS_HEADER -24
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
    int readByteCount = (*env)->CallIntMethod(env, (*encDataFeed), (*readOpusDataMethodId), (*jByteArrayReadBuffer), BUFFER_LENGTH);
    
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
    jclass decodeStreamInfoClass = (*env)->FindClass(env, "org/xiph/vorbis/decoderjni/DecodeStreamInfo");

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
/*
	//Find our java classes we'll be calling
	jclass tmp = (*env)->FindClass(env, "org/xiph/vorbis/decoderjni/DecodeFeed");
	encDataFeedClass = (jclass)(*env)->NewGlobalRef(env, tmp);

	//Find our java classes we'll be calling
	//jclass encDataFeedClass = (*env)->FindClass(env, "org/xiph/vorbis/decoderjni/DecodeFeed");

	encDataFeed = encDataFeed0;

	//Find our java method id's we'll be calling
	readOpusDataMethodId = (*env)->GetMethodID(env, encDataFeedClass, "onReadOpusData", "([BI)I");
	writePCMDataMethodId = (*env)->GetMethodID(env, encDataFeedClass, "onWritePCMData", "([SI)V");
	startMethodId = (*env)->GetMethodID(env, encDataFeedClass, "onStart", "(Lorg/xiph/vorbis/decoderjni/DecodeStreamInfo;)V");
	startReadingHeaderMethodId = (*env)->GetMethodID(env, encDataFeedClass, "onStartReadingHeader", "()V");
	stopMethodId = (*env)->GetMethodID(env, encDataFeedClass, "onStop", "()V");
	newIterationMethodId = (*env)->GetMethodID(env, encDataFeedClass, "onNewIteration", "()V");
	*/
}

JNIEXPORT int JNICALL Java_org_xiph_opus_decoderjni_OpusDecoder_readDecodeWriteLoop(JNIEnv *env, jclass cls, jobject encDataFeed) {
	LOGI(LOG_TAG, "startDecoding called, initing buffers");

    //Create a new java byte array to pass to the vorbis data feed method
    jbyteArray jByteArrayReadBuffer = (*env)->NewByteArray(env, BUFFER_LENGTH);

    //Create our write buffer
    jshortArray jShortArrayWriteBuffer = (*env)->NewShortArray(env, BUFFER_LENGTH*2);

    //-- get Java layer method pointer --//
	jclass encDataFeedClass = (*env)->FindClass(env, "org/xiph/vorbis/decoderjni/DecodeFeed");


	//Find our java method id's we'll be calling
	jmethodID readOpusDataMethodId = (*env)->GetMethodID(env, encDataFeedClass, "onReadOpusData", "([BI)I");
	jmethodID writePCMDataMethodId = (*env)->GetMethodID(env, encDataFeedClass, "onWritePCMData", "([SI)V");
	jmethodID startMethodId = (*env)->GetMethodID(env, encDataFeedClass, "onStart", "(Lorg/xiph/vorbis/decoderjni/DecodeStreamInfo;)V");
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
    
   // vorbis_info      vi; /* struct that stores all the static vorbis bitstream settings */
   // vorbis_comment   vc; /* struct that stores all the bitstream user comments */
   // vorbis_dsp_state vd; /* central working state for the packet->PCM decoder */
   // vorbis_block     vb; /* local working space for packet->PCM decode */
    
    char *buffer;
    int  bytes;
    
    /********** Decode setup ************/

    //Notify the decode feed we are starting to initialize
    onStartReadingHeader(env, &encDataFeed, &startReadingHeaderMethodId);
    
    ogg_sync_init(&oy); /* Now we can read pages */
    
    while(1) {
    	LOGE(LOG_TAG, "start while 1");
        /* we repeat if the bitstream is chained */
        int eos=0;
        int i;
        

        /* grab some data at the head of the stream. We want the first page
        (which is guaranteed to be small and only contain the Opus
        stream initial header) We need the first page to get the stream
        serialno. */
        
        // READ DATA
        /* submit a 4k block to libvorbis' Ogg layer */
        LOGI(LOG_TAG, "Submitting 4k block to libvorbis' Ogg layer");
        buffer = ogg_sync_buffer(&oy,BUFFER_LENGTH);
        bytes = onReadOpusDataFromOpusDataFeed(env, &encDataFeed, &readOpusDataMethodId, buffer, &jByteArrayReadBuffer);
        ogg_sync_wrote(&oy,bytes);
        
        /* Get the first page. */
        LOGD(LOG_TAG, "Getting the first page, read (%d) bytes", bytes);
        if(ogg_sync_pageout(&oy,&og)!=1){
        	/* have we simply run out of data?  If so, we're done. */

            if(bytes<BUFFER_LENGTH)break;
            /* error case.  Must not be Opus data */
            onStopDecodeFeed(env, &encDataFeed, &stopMethodId);
            return INVALID_OGG_BITSTREAM;
        }

        LOGI(LOG_TAG, "Successfully fetched the first page");

        /* Get the serial number and set up the rest of decode. */
        /* serialno first; use it to set up a logical stream */
        ogg_stream_init(&os,ogg_page_serialno(&og));

        /* extract the initial header from the first page and verify that the
        Ogg bitstream is in fact Opus data */

        /* I handle the initial header first instead of just having the code
        read all three Opus headers at once because reading the initial
        header is an easy way to identify a Opus bitstream and it's
        useful to see that functionality separated out. */

        /*vorbis_info_init(&vi);
        vorbis_comment_init(&vc);
        */
        if(ogg_stream_pagein(&os,&og)<0){
            // error; stream version mismatch perhaps
            onStopDecodeFeed(env, &encDataFeed, &stopMethodId);
            return ERROR_READING_FIRST_PAGE;
        }


        if(ogg_stream_packetout(&os,&op)!=1){
            /* no page? must not be vorbis */
            onStopDecodeFeed(env, &encDataFeed, &stopMethodId);
            return ERROR_READING_INITIAL_HEADER_PACKET;
        }

        LOGI(LOG_TAG, "Init Header 1");

        /*if(vorbis_synthesis_headerin(&vi,&vc,&op)<0){
            // error case; not a vorbis header
            onStopDecodeFeed(env, &encDataFeed, &stopMethodId);
            return NOT_VORBIS_HEADER;
        }*/


        /* At this point, we're sure we're Opus. We've set up the logical
        (Ogg) bitstream decoder. Get the comment and codebook headers and
        set up the Opus decoder */

        /* The next two packets in order are the comment and codebook headers.
        They're likely large and may span multiple pages. Thus we read
        and submit data until we get our two packets, watching that no
        pages are missing. If a page is missing, error out; losing a
        header page is the only place where missing data is fatal. */

        i=0;
        while(i<2){
        	LOGE(LOG_TAG, "start while 2");
            while(i<2){
            	LOGE(LOG_TAG, "start while 3");
                int result=ogg_sync_pageout(&oy,&og);
                if(result==0)break; /* Need more data */
                /* Don't complain about missing or corrupt data yet. We'll
                catch it at the packet output phase */
                if(result==1){
                    ogg_stream_pagein(&os,&og); /* we can ignore any errors here
                    as they'll also become apparent
                    at packetout */
                    while(i<2){
                    	LOGE(LOG_TAG, "start while 4");
                        result=ogg_stream_packetout(&os,&op);
                        if(result==0)break;
                        if(result<0){
                            /* data at some point was corrupted or missing!
                            We can't tolerate that in a header.  Die. */
                            onStopDecodeFeed(env, &encDataFeed, &stopMethodId);
                            return CORRUPT_SECONDARY_HEADER;
                        }
                        LOGI(LOG_TAG, "Init header 2");
                        /*result=vorbis_synthesis_headerin(&vi,&vc,&op);
                        if(result<0){
                            onStopDecodeFeed(env, &encDataFeed, &stopMethodId);
                            return CORRUPT_SECONDARY_HEADER;
                        }*/

                        i++;
                    }
                }
            }

            //READ DATA
            /* no harm in not checking before adding more */
            buffer=ogg_sync_buffer(&oy,BUFFER_LENGTH);
            bytes=onReadOpusDataFromOpusDataFeed(env, &encDataFeed, &readOpusDataMethodId, buffer, &jByteArrayReadBuffer);
            if(bytes==0 && i<2){
                onStopDecodeFeed(env, &encDataFeed, &stopMethodId);
                return PREMATURE_END_OF_FILE;
            }
            ogg_sync_wrote(&oy,bytes);
        }


        /* Throw the comments plus a few lines about the bitstream we're
        decoding */
        {
           /* char **ptr=vc.user_comments;
            while(*ptr){
                ++ptr;
            }

            LOGI(LOG_TAG, "Bitstream is %d channel",vi.channels);
            LOGI(LOG_TAG, "Bitstream %d Hz",vi.rate);
            LOGI(LOG_TAG, "Encoded by: %s\n\n",vc.vendor);*/
            // report stream parameters
            onStart(env, &encDataFeed, &startMethodId, 0,0,"");// vi.rate, vi.channels, vc.vendor);
        }

        //convsize=BUFFER_LENGTH/vi.channels;

        /* OK, got and parsed all three headers. Initialize the Opus
        packet->PCM decoder. */
        LOGD(LOG_TAG, "Headers parsed, go for PCM decoding");


        ogg_stream_clear(&os);

    }
    /* OK, clean up the framer */
    ogg_sync_clear(&oy);



    onStopDecodeFeed(env, &encDataFeed, &stopMethodId);

    //Clean up our buffers
    (*env)->DeleteLocalRef(env, jByteArrayReadBuffer);
    (*env)->DeleteLocalRef(env, jShortArrayWriteBuffer);

    return SUCCESS;
}
