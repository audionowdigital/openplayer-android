/* Takes a vorbis bitstream from java callbacks from JNI and writes raw stereo PCM to
the jni callbacks. Decodes simple and chained OggVorbis files from beginning
to end. */

#include "org_xiph_vorbis_decoderjni_VorbisDecoder.h"

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
    if(bytes == 0) {
        return;
    }
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
    (*env)->CallVoidMethod(env, (*vorbisDataFeed), (*startMethodId), decodeStreamInfo);

    //Cleanup decode feed object
    (*env)->DeleteLocalRef(env, decodeStreamInfo);

    //Cleanup java vendor string
    (*env)->DeleteLocalRef(env, vendorString);
}

//Starts reading the header information
void onStartReadingHeader(JNIEnv *env, jobject *vorbisDataFeed, jmethodID* startReadingHeaderMethodId) {
    LOGI(LOG_TAG, "Notifying decode feed to onStart reading the header");

    //Call header onStart reading method
    (*env)->CallVoidMethod(env, (*vorbisDataFeed), (*startReadingHeaderMethodId));
}

//Starts reading the header information
void onNewIteration(JNIEnv *env, jobject *vorbisDataFeed, jmethodID* newIterationMethodId) {
    //LOGI(LOG_TAG, "Notifying decode feed to onNewIteration");

    //Call header onStart reading method
    (*env)->CallVoidMethod(env, (*vorbisDataFeed), (*newIterationMethodId));
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
	jmethodID newIterationMethodId = (*env)->GetMethodID(env, vorbisDataFeedClass, "onNewIteration", "()V");
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
    
    ogg_sync_init(&oy); /* Now we can read pages */
    
    while(1) {
    	LOGE(LOG_TAG, "start while 1");
        /* we repeat if the bitstream is chained */
        int eos=0;
        int i;
        

        /* grab some data at the head of the stream. We want the first page
        (which is guaranteed to be small and only contain the Vorbis
        stream initial header) We need the first page to get the stream
        serialno. */
        
        // READ DATA
        /* submit a 4k block to libvorbis' Ogg layer */
        LOGI(LOG_TAG, "Submitting 1 4k block to libvorbis' Ogg layer");
        buffer=ogg_sync_buffer(&oy,BUFFER_LENGTH);
        bytes=onReadVorbisDataFromVorbisDataFeed(env, &vorbisDataFeed, &readVorbisDataMethodId, buffer, &jByteArrayReadBuffer);
        ogg_sync_wrote(&oy,bytes);
        
        /* Get the first page. */
        LOGD(LOG_TAG, "Getting the first page, read (%d) bytes", bytes);
        if(ogg_sync_pageout(&oy,&og)!=1){
        	/* have we simply run out of data?  If so, we're done. */
            if(bytes<BUFFER_LENGTH)break;
            /* error case.  Must not be Vorbis data */
            onStopDecodeFeed(env, &vorbisDataFeed, &stopMethodId);
            return INVALID_OGG_BITSTREAM;
        }

        LOGI(LOG_TAG, "Successfully fetched the first page");

        /* Get the serial number and set up the rest of decode. */
        /* serialno first; use it to set up a logical stream */
        ogg_stream_init(&os,ogg_page_serialno(&og));

        /* extract the initial header from the first page and verify that the
        Ogg bitstream is in fact Vorbis data */

        /* I handle the initial header first instead of just having the code
        read all three Vorbis headers at once because reading the initial
        header is an easy way to identify a Vorbis bitstream and it's
        useful to see that functionality separated out. */

        vorbis_info_init(&vi);
        vorbis_comment_init(&vc);
        if(ogg_stream_pagein(&os,&og)<0){
            /* error; stream version mismatch perhaps */
            onStopDecodeFeed(env, &vorbisDataFeed, &stopMethodId);
            return ERROR_READING_FIRST_PAGE;
        }

        if(ogg_stream_packetout(&os,&op)!=1){
            /* no page? must not be vorbis */
            onStopDecodeFeed(env, &vorbisDataFeed, &stopMethodId);
            return ERROR_READING_INITIAL_HEADER_PACKET;
        }

        if(vorbis_synthesis_headerin(&vi,&vc,&op)<0){
            /* error case; not a vorbis header */
            onStopDecodeFeed(env, &vorbisDataFeed, &stopMethodId);
            return NOT_VORBIS_HEADER;
        }

        /* At this point, we're sure we're Vorbis. We've set up the logical
        (Ogg) bitstream decoder. Get the comment and codebook headers and
        set up the Vorbis decoder */

        /* The next two packets in order are the comment and codebook headers.
        They're likely large and may span multiple pages. Thus we read
        and submit data until we get our two packets, watching that no
        pages are missing. If a page is missing, error out; losing a
        header page is the only place where missing data is fatal. */

        i=0;
        while (i<2) {
        	LOGE(LOG_TAG, "start while 2");
            while (i<2) {
            	LOGE(LOG_TAG, "start while 3");
                int result=ogg_sync_pageout(&oy,&og);
                if (result==0) break; /* Need more data */
                /* Don't complain about missing or corrupt data yet. We'll
                catch it at the packet output phase */
                if(result==1){
                    ogg_stream_pagein(&os,&og); /* we can ignore any errors here
                    as they'll also become apparent
                    at packetout */
                    while (i<2) {
                    	LOGE(LOG_TAG, "start while 4");
                        result=ogg_stream_packetout(&os,&op);
                        if(result==0)break;
                        if(result<0){
                            /* data at some point was corrupted or missing!
                            We can't tolerate that in a header.  Die. */
                            onStopDecodeFeed(env, &vorbisDataFeed, &stopMethodId);
                            return CORRUPT_SECONDARY_HEADER;
                        }
                        result=vorbis_synthesis_headerin(&vi,&vc,&op);
                        if(result<0){
                            onStopDecodeFeed(env, &vorbisDataFeed, &stopMethodId);
                            return CORRUPT_SECONDARY_HEADER;
                        }
                        i++;
                    }
                }
            } // end while 3

            //READ DATA
            /* no harm in not checking before adding more */
            buffer=ogg_sync_buffer(&oy,BUFFER_LENGTH);
            bytes=onReadVorbisDataFromVorbisDataFeed(env, &vorbisDataFeed, &readVorbisDataMethodId, buffer, &jByteArrayReadBuffer);
            if(bytes==0 && i<2){
                onStopDecodeFeed(env, &vorbisDataFeed, &stopMethodId);
                return PREMATURE_END_OF_FILE;
            }
            ogg_sync_wrote(&oy,bytes);
        } // end while 2


        /* Throw the comments plus a few lines about the bitstream we're
        decoding */
        {
            char **ptr=vc.user_comments;
            while(*ptr){
                ++ptr;
            }

            LOGI(LOG_TAG, "Bitstream is %d channel",vi.channels);
            LOGI(LOG_TAG, "Bitstream %d Hz",vi.rate);
            LOGI(LOG_TAG, "Encoded by: %s\n\n",vc.vendor);

            onStart(env, &vorbisDataFeed, &startMethodId, vi.rate, vi.channels, vc.vendor);
        }

        convsize=BUFFER_LENGTH/vi.channels;

        /* OK, got and parsed all three headers. Initialize the Vorbis packet->PCM decoder. */
        LOGD(LOG_TAG, "Headers parsed, go for PCM decoding");
        if(vorbis_synthesis_init(&vd,&vi)==0){
            /* central decode state */
            vorbis_block_init(&vd,&vb);          /* local state for most of the decode
            so multiple block decodes can
            proceed in parallel. We could init
            multiple vorbis_block structures
            for vd here */

            /* The rest is just a straight decode loop until end of stream */
            while(!eos){
            	LOGE(LOG_TAG, "start while 5");

                while(!eos){
                	LOGE(LOG_TAG, "start while 6");
                    int result=ogg_sync_pageout(&oy,&og);
                    if(result==0)break; /* need more data, so go to PREVIOUS loop and read more */
                    if(result<0){
                        /* missing or corrupt data at this page position */
                        LOGW(LOG_TAG, "VorbisDecoder %s", "Corrupt or missing data in bitstream; continuing..");
                    }
                    else{
                        ogg_stream_pagein(&os,&og); /* can safely ignore errors at this point */
                        while(1){
                        	LOGE(LOG_TAG, "start while 7");
                        	/* inform player that we are about to start a new iteration */
                  	        onNewIteration(env, &vorbisDataFeed, &newIterationMethodId);

                            result=ogg_stream_packetout(&os,&op);

                            if(result==0)break; /* need more data so exit and go read data in PREVIOUS loop */
                            if(result<0){
                                /* missing or corrupt data at this page position */
                                /* no reason to complain; already complained above */
                            }
                            else{

                                /* we have a packet.  Decode it */
                                float **pcm;
                                int samples;

                                if(vorbis_synthesis(&vb,&op)==0) /* test for success! */
                                vorbis_synthesis_blockin(&vd,&vb);
                                /*

                                **pcm is a multichannel float vector.  In stereo, for
                                example, pcm[0] is left, and pcm[1] is right.  samples is
                                the size of each channel.  Convert the float values
                                (-1.<=range<=1.) to whatever PCM format and write it out */

                                while((samples=vorbis_synthesis_pcmout(&vd,&pcm))>0){
                                	LOGE(LOG_TAG, "start while 8");
                                    int j;
                                    int clipflag=0;
                                    int bout=(samples<convsize?samples:convsize);

                                    /* convert floats to 16 bit signed ints (host order) and
                                    interleave */
                                    for(i=0;i<vi.channels;i++){
                                        ogg_int16_t *ptr=convbuffer+i;
                                        float  *mono=pcm[i];
                                        for(j=0;j<bout;j++){
                                            int val=floor(mono[j]*32767.f+.5f);
                                            /* might as well guard against clipping */
                                            if(val>32767) { val=32767; clipflag=1; }
                                            if(val<-32768) { val=-32768; clipflag=1; }
                                            *ptr=val;
                                            ptr+=vi.channels;
                                        }
                                    }

                                    if(clipflag) {
                                        LOGI(LOG_TAG, "Clipping in frame %ld\n",(long)(vd.sequence));
                                    }

                                    /* Call decodefeed to push data to AudioTrack */
                                    onWritePCMDataFromVorbisDataFeed(env, &vorbisDataFeed, &writePCMDataMethodId, &convbuffer[0], bout*vi.channels, &jShortArrayWriteBuffer);
                                    vorbis_synthesis_read(&vd,bout); /* tell libvorbis how many samples we actually consumed */
                                }
                            }
                        }
                        if(ogg_page_eos(&og))eos=1;
                    }
                } // WHILE 6

                // content player read
                if(!eos){
                	// READ DATA
                    buffer=ogg_sync_buffer(&oy,BUFFER_LENGTH);
                    bytes=onReadVorbisDataFromVorbisDataFeed(env, &vorbisDataFeed, &readVorbisDataMethodId, buffer, &jByteArrayReadBuffer);
                    ogg_sync_wrote(&oy,bytes);

                    // ending condition if no more data available
                    if(bytes==0) eos=1;
                }
            } // while 5

            /* ogg_page and ogg_packet structs always point to storage in
            libvorbis.  They're never freed or manipulated directly */
            vorbis_block_clear(&vb);
            vorbis_dsp_clear(&vd);

        }
        else{
            LOGW(LOG_TAG, "Error: Corrupt header during playback initialization.");
        }

        /* clean up this logical bitstream; before exit we see if we're
        followed by another [chained] */

        ogg_stream_clear(&os);
        vorbis_comment_clear(&vc);
        vorbis_info_clear(&vi);  /* must be called last */
    }

    /* OK, clean up the framer */
    ogg_sync_clear(&oy);



    onStopDecodeFeed(env, &vorbisDataFeed, &stopMethodId);

    //Clean up our buffers
    (*env)->DeleteLocalRef(env, jByteArrayReadBuffer);
    (*env)->DeleteLocalRef(env, jShortArrayWriteBuffer);

    return SUCCESS;
}
