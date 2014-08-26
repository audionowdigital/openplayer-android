package net.pocketmagic.android.openmxplayer;

/*
** OpenMXPlayer - Freeware audio player library for Android
** Copyright (C) 2009 - 2014 Radu Motisan, radu.motisan@gmail.com
**
** This file is a part of "OpenMXPlayer" open source library.
**/
import java.io.FileDescriptor;
import java.io.InputStream;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;

import com.audionowdigital.android.openplayer.DecodeFeed;

import android.content.Context;
import android.content.res.AssetFileDescriptor;
import android.media.AudioFormat;
import android.media.AudioManager;
import android.media.AudioTrack;
import android.media.MediaCodec;
import android.media.MediaCodecInfo;
import android.media.MediaCodecList;
import android.media.MediaExtractor;
import android.media.MediaFormat;
import android.os.Handler;
import android.util.Log;

public class MXDecoder {
	private static final String TAG = "MXDecoder";
	
	private static final int
		BUFFER_LENGTH = 4096,
		INVALID_HEADER = -1,
		SUCCESS = 0, 
		MX_HEADERS =3; // TODO: check this

	private static MediaExtractor extractor;
	private static MediaCodec codec;
		
	// currently unused
	public static int init(int param) {
		return 0;
	}
	
	// the main loop to read data, decode, write
	public static int readDecodeWriteLoop(DecodeFeed decodeFeed) {
		
		String mime = null;
	    int sampleRate = 0, channels = 0, bitrate = 0;
	    long presentationTimeUs = 0, duration = 0;

	    boolean stop = false;
	    
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
        final long kTimeOutUs = 1000;
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
                        //if (events != null) handler.post(new Runnable() { @Override public void run() { events.onPlayUpdate(percent, presentationTimeUs / 1000, duration / 1000);  } });
                    }
   
                	codec.queueInputBuffer(inputBufIndex, 0, sampleSize, presentationTimeUs, sawInputEOS ? MediaCodec.BUFFER_FLAG_END_OF_STREAM : 0);
                	
                    if (!sawInputEOS) extractor.advance();
                    
                } else {
                	Log.e(TAG, "inputBufIndex " +inputBufIndex);
                }
            } // !sawInputEOS

            // decode to PCM and push it to the AudioTrack player
            int res = codec.dequeueOutputBuffer(info, kTimeOutUs);

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
                	decodeFeed.onWritePCMData(shorts, shorts.length);
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
        
        if(codec != null) {
			codec.stop();
			codec.release();
			codec = null;
		}
	    
	    // clear source and the other globals
	    duration = 0;
	    mime = null;
	    sampleRate = 0; channels = 0; bitrate = 0;
	    presentationTimeUs = 0; duration = 0;
		
	    
        stop = true;
        
    	/*if(noOutputCounter >= noOutputCounterLimit) {
    		if (events != null) handler.post(new Runnable() { @Override public void run() { events.onError();  } }); 
	    } else {
	    	if (events != null) handler.post(new Runnable() { @Override public void run() { events.onStop();  } }); 
        }*/
    	
        //---------------------------
		decodeFeed.onStop();
		return err;
	}

}
