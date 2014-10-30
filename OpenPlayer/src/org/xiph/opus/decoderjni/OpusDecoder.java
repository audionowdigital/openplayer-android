/*
 * OpusDecoder.java - The native opus decoder to be used in conjunction with JNI
 *
 * (C) 2014 Radu Motisan, radu.motisan@gmail.com
 *
 * Part of the OpenPlayer implementation for Alpine Audio Now Digital LLC
 */

package org.xiph.opus.decoderjni;

import com.audionowdigital.android.openplayer.DecodeFeed;

/**
 * Created by radhoo on /14.
 */

public class OpusDecoder {

    /**
     * Load our -jni library and other dependent libraries
     */
    static {
        System.loadLibrary("ogg");
        System.loadLibrary("opus");
        System.loadLibrary("opus-jni");
    }
    
    /**
     * Init the JNI layer
     * @param debug set to true to enable debug
     * @return currently not used
     */
    public static native int initJni(int debug);
    
    /**
     * Start decoding the data by way of a jni call
     *
     * @param decodeFeed the custom decode feed
     * @return the result code
     */
    public static native int readDecodeWriteLoop(DecodeFeed decodeFeed);
}
