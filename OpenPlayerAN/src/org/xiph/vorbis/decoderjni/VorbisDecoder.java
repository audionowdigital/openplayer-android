package org.xiph.vorbis.decoderjni;

/**
 * The native vorbis decoder to be used in conjunction with JNI
 */
public class VorbisDecoder {

    /**
     * Load our vorbis-jni library and other dependent libraries
     */
    static {
        System.loadLibrary("ogg");
        System.loadLibrary("vorbis");
        System.loadLibrary("vorbis-jni");
    }

    /**
     * Start decoding the data by way of a jni call
     *
     * @param decodeFeed the custom decode feed
     * @return the result code
     */
    public static native int initJni(DecodeFeed decodeFeed);
    
    public static native int readDecodeWriteLoop();
}
