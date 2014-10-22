/*
 * OpenMXPlayer - Freeware audio player library for Android
 *
 * Copyright (C) 2009 - 2014 Radu Motisan, radu.motisan@gmail.com
 *
 * This file is a part of "OpenMXPlayer" open source library. Licensed under LGPL
 *
 * http://www.pocketmagic.net/2014/06/android-audio-player-using-mediacodec-mediaextractor/
 */

package net.pocketmagic.android.openmxplayer;

import android.media.MediaCodec;
import android.media.MediaExtractor;
import android.media.MediaFormat;
import android.util.Log;

import com.audionowdigital.android.openplayer.DecodeFeed;

import java.nio.ByteBuffer;
import java.nio.ByteOrder;

/**
 * Created by radhoo on /14.
 */

public class MXDecoder {
	private static final String TAG = "MXDecoder";
	
	private static final int
		BUFFER_LENGTH = 4096,
		INVALID_HEADER = -1,
		SUCCESS = 0, 
		MX_HEADERS =3; // TODO: check this

	private static MediaExtractor extractor;
	private static MediaCodec codec;
	private static boolean stop = false;
		
	// currently unused
	public static int init(int param) {
		return 0;
	}
	public static void stop() {
		stop = true;
	}
	// the main loop to read data, decode, write
	public static int readDecodeWriteLoop(DecodeFeed decodeFeed) {
		
		String mime = null;
	    int sampleRate = 0, channels = 0, bitrate = 0;
	    long presentationTimeUs = 0, duration = 0;

	    stop = false;
	    
		// start right away
		decodeFeed.onStartReadingHeader();
		
		int err = SUCCESS;
		
		//InputStream is = decodeFeed.getDataSource().getInputStream();
		String srcPath = decodeFeed.getDataSource().getPath();
		Log.d(TAG, "readDecodeWriteLoop call for src:" + srcPath);
		
		// open source and read type / header
		// extractor gets information about the stream
        extractor = new MediaExtractor();
        
        // try to set the source, this might fail
        try {
			extractor.setDataSource(srcPath);
		} catch (Exception e) {
			Log.e(TAG, "exception: "+e.getMessage());
			decodeFeed.onStop();
			return INVALID_HEADER;
		}
		
        // Read track header
        MediaFormat format = null; 
        try {
        	format = extractor.getTrackFormat(0);
	        mime = format.getString(MediaFormat.KEY_MIME);
	        sampleRate = format.getInteger(MediaFormat.KEY_SAMPLE_RATE);
			channels = format.getInteger(MediaFormat.KEY_CHANNEL_COUNT);
			// if duration is 0, we are probably playing a live stream
			duration = format.getLong(MediaFormat.KEY_DURATION);
			bitrate = format.getInteger(MediaFormat.KEY_BIT_RATE);
        } catch (Exception e) {
			Log.e(TAG, "Reading format parameters exception:"+e.getMessage());
			// don't exit, tolerate this error, we'll fail later if this is critical
		}
        Log.d(TAG, "Track info: mime:" + mime + " sampleRate:" + sampleRate + " channels:" + channels + " bitrate:" + bitrate + " duration:" + duration);
       
        // check we have audio content we know
        if (format == null || !mime.startsWith("audio/")) {
        	Log.e(TAG, "Invalid mime:" + mime);
        	decodeFeed.onStop();
			return INVALID_HEADER;
        }
        
        // create the actual decoder, using the mime to select
        codec = MediaCodec.createDecoderByType(mime);
        // check we have a valid codec instance
        if (codec == null) {
        	Log.e(TAG, "Can't start codec for mime:" + mime);
        	decodeFeed.onStop();
			return INVALID_HEADER;
        }
        
        // finally we're starting!!!
        decodeFeed.onStart(sampleRate, channels, "", "", "", "", "", "");

        codec.configure(format, null, null, 0);
        codec.start();
        ByteBuffer[] codecInputBuffers  = codec.getInputBuffers();
        ByteBuffer[] codecOutputBuffers = codec.getOutputBuffers();
        
		// start playing, we will feed the AudioTrack later
        extractor.selectTrack(0);
        
        // start decoding
        
        final long kTimeOutUs = 10;

        MediaCodec.BufferInfo info = new MediaCodec.BufferInfo();
        
        boolean sawInputEOS = false;
        boolean sawOutputEOS = false;
        int noOutputCounter = 0;
        int noOutputCounterLimit = 10;
        
        //state.set(PlayerStates.PLAYING);
        while (!sawOutputEOS && noOutputCounter < noOutputCounterLimit && !stop) {
        	
        	// pause implementation
        	//waitPlay();
        	decodeFeed.onReadEncodedData(null,  0); //we read nothing, but we use this to block
        	
        	
        	noOutputCounter++;
        	// read a buffer before feeding it to the decoder
            if (!sawInputEOS) {
            	int inputBufIndex = codec.dequeueInputBuffer(kTimeOutUs);
                if (inputBufIndex >= 0) {
                    ByteBuffer dstBuf = codecInputBuffers[inputBufIndex];
                    int sampleSize = extractor.readSampleData(dstBuf, 0);
                    if (sampleSize < 0) {
                        Log.d(TAG, "saw input EOS. Stopping playback");
                        sawInputEOS = true;
                        sampleSize = 0;
                    } else {
                        presentationTimeUs = extractor.getSampleTime();
                        final int percent =  (duration == 0)? 0 : (int) (100 * presentationTimeUs / duration);
                    }
   
                	codec.queueInputBuffer(inputBufIndex, 0, sampleSize, presentationTimeUs, sawInputEOS ? MediaCodec.BUFFER_FLAG_END_OF_STREAM : 0);
                	
                    if (!sawInputEOS) extractor.advance();
                    
                } else {
                	Log.e(TAG, "inputBufIndex " +inputBufIndex);
                }
            } // !sawInputEOS

            // decode to PCM 
            int res = codec.dequeueOutputBuffer(info, kTimeOutUs);
            // push PCM to the AudioTrack player
            if (res >= 0) {
                if (info.size > 0)  noOutputCounter = 0;
                
                int outputBufIndex = res;
                ByteBuffer buf = codecOutputBuffers[outputBufIndex];
                // check byte[] buffer
                final byte[] chunk = new byte[info.size];
                buf.get(chunk);
                buf.clear();
                // convert it to short[]
                short[] shorts = new short[chunk.length/2];
                // to turn bytes to shorts as either big endian or little endian. 
                ByteBuffer.wrap(chunk).order(ByteOrder.LITTLE_ENDIAN).asShortBuffer().get(shorts);
                // feed it to audiotrack
                if (shorts.length > 0) {
                	// we have PCM data and we also know exact position in the source: only for MX
                	decodeFeed.onWritePCMData(shorts, shorts.length, (int) (extractor.getSampleTime() / 1000000));
                }

                codec.releaseOutputBuffer(outputBufIndex, false);

                if ((info.flags & MediaCodec.BUFFER_FLAG_END_OF_STREAM) != 0) {
                    Log.d(TAG, "saw output EOS.");
                    sawOutputEOS = true;
                }
            } else if (res == MediaCodec.INFO_OUTPUT_BUFFERS_CHANGED) {
                codecOutputBuffers = codec.getOutputBuffers();
                Log.d(TAG, "output buffers have changed.");
            } else if (res == MediaCodec.INFO_OUTPUT_FORMAT_CHANGED) {
                MediaFormat oformat = codec.getOutputFormat();
                Log.d(TAG, "output format has changed to " + oformat);
            } else {
                Log.d(TAG, "dequeueOutputBuffer returned " + res);
            }
        }
        
        Log.d(TAG, "stopping...");

        //codec.
        if(codec != null) {
        	Log.d(TAG, "Release codec");
        	
			codec.stop();
			codec.release();
			codec = null;
		}


        extractor.release();


	    stop = true;

        // duration 0 for live stream

        // for recording daemon's dance: duration:30834100 presentationTimeUs:30798367
        Log.e(TAG, "duration:" + duration + " presentationTimeUs:" + presentationTimeUs );

        // 1 sec tolerance
        if(presentationTimeUs + 1000000 < duration) {
            Log.d(TAG, "readDecodeWriteLoop stopped with error.");
            err = DecodeFeed.DECODE_ERROR;
            //decodeFeed.onError();
        } else {
            Log.d(TAG, "readDecodeWriteLoop stopped.");
        }


        // clear source and the other globals
        duration = 0;
        mime = null;
        sampleRate = 0; channels = 0; bitrate = 0;
        presentationTimeUs = 0; duration = 0;


        decodeFeed.onStop();

		return err;
	}
	
	/**
	 * Set the track play position to the given value
	 * @param pos the new position in seconds
	 */
	public static void setPositionSec(int pos) {
		extractor.seekTo(pos * 1000000, MediaExtractor.SEEK_TO_CLOSEST_SYNC);		
	}

}
