#include "DecodeFeed.h"


//Stops the vorbis data feed
void onStop(JNIEnv *env, jobject* javaDecodeFeedObj, jmethodID* stopMethodId) {
    (*env)->CallVoidMethod(env, (*javaDecodeFeedObj), (*stopMethodId));
}

//Reads raw vorbis data from the jni callback
int onReadEncodedData(JNIEnv *env, jobject* javaDecodeFeedObj, jmethodID* readVorbisDataMethodId, char* buffer, jbyteArray* jByteArrayReadBuffer) {
    //Call the read method
    int readByteCount = (*env)->CallIntMethod(env, (*javaDecodeFeedObj), (*readVorbisDataMethodId), (*jByteArrayReadBuffer), BUFFER_LENGTH);

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
void onWritePCMData(JNIEnv *env, jobject* javaDecodeFeedObj, jmethodID* writePCMDataMethodId, ogg_int16_t* buffer, int bytes, jshortArray* jShortArrayWriteBuffer) {

    //No data to read, just exit
    if(bytes == 0) return;

    //LOGI(LOG_TAG, "onWritePCMData %d", bytes);

    //Copy the contents of what we're writing to the java short array
    (*env)->SetShortArrayRegion(env, (*jShortArrayWriteBuffer), 0, bytes, (jshort *)buffer);

    //Call the write pcm data method
    (*env)->CallVoidMethod(env, (*javaDecodeFeedObj), (*writePCMDataMethodId), (*jShortArrayWriteBuffer), bytes, -1);
}

//Starts the decode feed with the necessary information about sample rates, channels, etc about the stream
void onStart(JNIEnv *env, jobject *javaDecodeFeedObj, jmethodID* startMethodId, long sampleRate, long channels, char* vendor,
		char *title, char *artist, char *album, char *date, char *track) {
    LOGI(LOG_TAG, "Notifying decode feed");

    //Creates a java string for the vendor
    jstring vendorString = (*env)->NewStringUTF(env, vendor);
    jstring titleString = (*env)->NewStringUTF(env, title);
    jstring artistString = (*env)->NewStringUTF(env, artist);
    jstring albumString = (*env)->NewStringUTF(env, album);
    jstring dateString = (*env)->NewStringUTF(env, date);
    jstring trackString = (*env)->NewStringUTF(env, track);

    //Call decode feed onStart
    (*env)->CallVoidMethod(env, (*javaDecodeFeedObj), (*startMethodId), (jlong)sampleRate, (jlong)channels, vendorString, titleString, artistString,albumString,dateString,trackString);

    //Cleanup java vendor string
    (*env)->DeleteLocalRef(env, vendorString);
    (*env)->DeleteLocalRef(env, titleString);
    (*env)->DeleteLocalRef(env, artistString);
    (*env)->DeleteLocalRef(env, albumString);
    (*env)->DeleteLocalRef(env, dateString);
    (*env)->DeleteLocalRef(env, trackString);
}

//Starts reading the header information
void onStartReadingHeader(JNIEnv *env, jobject *javaDecodeFeedObj, jmethodID* startReadingHeaderMethodId) {
    LOGI(LOG_TAG, "onStartReadingHeader call .");

    //Call header onStart reading method
    (*env)->CallVoidMethod(env, (*javaDecodeFeedObj), (*startReadingHeaderMethodId));
}
