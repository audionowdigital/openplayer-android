/*
 * OpusDecoder.java - The native opus decoder to be used in conjunction with JNI
 *
 * (C) 2014 Radu Motisan, radu.motisan@gmail.com
 *
 * Part of the OpenPlayer implementation for Alpine Audio Now Digital LLC
 */

package org.xiph.opus.decoderjni;

import android.util.Log;

import com.audionowdigital.android.openplayer.DecodeFeed;

/**
 * Created by radhoo on /14.
 */

public class OpusDecoder {

    /**
     * Load our -jni library and other dependent libraries
     */
    static {
    	//Log.e("test", "1");
        System.loadLibrary("ogg");
        //Log.e("test", "2");
        System.loadLibrary("opus");
        //Log.e("test", "3");
        System.loadLibrary("opus-jni"); //dlopen failed: cannot locate symbol "opus_header_parse" referenced by "libopus-jni.so"...
        //Log.e("test", "4");
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
