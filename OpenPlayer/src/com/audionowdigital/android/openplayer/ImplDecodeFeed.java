package com.audionowdigital.android.openplayer;

import android.media.AudioFormat;
import android.media.AudioManager;
import android.media.AudioTrack;
import android.util.Log;

/**
 * Custom class to easily buffer and decode from a stream and write to an {@link AudioTrack}
 */
public class ImplDecodeFeed implements DecodeFeed {
	/**
	 * The debug tag
	 */
	private String TAG = "ImplDecodeFeed";
	/**
	 * Hold the player state object to know about any changes
	 */
	protected PlayerStates playerState;
	/**
	 * Hold the player events used to inform client
	 */
	protected PlayerEvents events;
    /**
     * The audio track to write the raw pcm bytes to
     */
    protected AudioTrack audioTrack;

    /**
     * The input stream to decode from
     */
    protected DataSource data;
    protected long streamSecondsLength;
    

    /**
     * The amount of written pcm data to the audio track
     */
    protected long writtenPCMData = 0;
    
    
    /**
     * Track seconds or for how many seconds have we been playing
     */
    protected long writtenMiliSeconds = 0;
    


    /**
     * Stream info as reported in the header 
     */
    DecodeStreamInfo streamInfo;
    
    /**
     * Creates a decode feed that reads from a file and writes to an {@link AudioTrack}
     *
     */
    
    public ImplDecodeFeed(PlayerStates playerState, PlayerEvents events) {
    	this.playerState = playerState;
        this.events = events;
	}

    /**
     * Polls the current stream playing position in seconds
     * @return the second where the current play position is in the stream
     */
    public int getCurrentPosition() {
    	return (int) (writtenMiliSeconds / 1000);
    }

    /**
     * init the player data source using a string containing either a file absolute location or an url
     * @param path the file path or the url
     * @param streamSecondsLength the total size in seconds of this stream or -1 if not available (live streams)
     */
    public void setData(String path, long streamSecondsLength) {
    	if (path == null) {
            throw new IllegalArgumentException("Path to decode must not be null.");
        }
        this.streamSecondsLength = streamSecondsLength;
        

        // Make sure that for podcasts, the stream is a BufferedInputStream
       /* if (!(streamToDecode instanceof BufferedInputStream) && streamSize > 0) {
            this.inputStream = new BufferedInputStream(streamToDecode);
        } else {*/
        Log.d(TAG, "Creating a new data source obj");
        data = new DataSource(path);

        /*}
          if (streamSize > 0) {
            this.inputStream.markSupported();
            this.inputStream.mark((int)streamSize);
        }*/
    }
    
    /**
     * A pause mechanism that would block current thread when pause flag is set (READY_TO_PLAY)
     */
    public synchronized void waitPlay(){
        while(playerState.get() == PlayerStates.READY_TO_PLAY) {
            if (streamSecondsLength == -1){
                writtenMiliSeconds = 0;
            }
            try {
                wait();
            } catch (InterruptedException e) {
                e.printStackTrace();
            }
        }
    }
    
    /**
     * Call notify to control the PAUSE (waiting) state, when the state is changed
     */
    public synchronized void syncNotify() {
    	notify();
    }
    
    /**
     * Triggered from the native that is requesting to read the next bit of opus data
     *
     * @param buffer        the buffer to write to
     * @param amountToWrite the amount of opus data to write (from inputstream to our buffer)
     * @return the amount actually written
     */
    @Override public int onReadEncodedData(byte[] buffer, int amountToWrite) {
    	if (!data.isSourceValid()) {
    		return 0;
    	}
    	//Log.d(TAG, "onReadOpusData call: " + amountToWrite);
        //If the player is not playing or reading the header, return 0 to end the native decode method
        if (playerState.get() == PlayerStates.STOPPED) {
            return 0;
        }
        
        waitPlay();

        //Otherwise read from the file
        try {
            int read = data.read(buffer, 0, amountToWrite);
            
            return read == -1 ? 0 : read;
        } catch (Exception e) {
            //There was a problem reading from the file
            Log.e(TAG, "Failed to read opus data from file.  Aborting.", e);
            return 0;
        }
    }

    /**
     * Called to change the current read position for the InputStream.
     * @throws java.lang.IllegalStateException for live streams.
     * @param percent - percentage where to seek
     */
    @Override
    public void setPosition(int percent) {
        if (streamSecondsLength < 0) {
            throw new IllegalStateException("Stream length must be a positive number");
        }
        long seekPosition = percent * data.getSourceLength() / 100;
        if (data.isSourceValid()) {
            try {
                audioTrack.flush();
                //inputStream.reset();
                //data.seekTo()
                data.skip(seekPosition);
                writtenMiliSeconds = percent * streamSecondsLength * 10; // that is /100 * 1000 - save in millis for now.

                Log.d(TAG,"SKIP_POS  percent:" + percent + " new_offset:" + seekPosition + " orig_source_len:" +  data.getSourceLength()+
                        "new_sec:"+ writtenMiliSeconds + " Min:"+ (writtenMiliSeconds / 60000) + ":"+((writtenMiliSeconds/1000)%60) );

            } catch (OutOfMemoryError e) {
            	e.printStackTrace();
            }
        }
    }


    /**
     * Triggered from the native that is requesting to write the next bit of raw PCM data
     *
     * @param pcmData      the raw pcm data
     * @param amountToRead the amount available to read in the buffer and dump it to our PCM buffers
     */
    @Override
    public synchronized void onWritePCMData(short[] pcmData, int amountToRead) {
		waitPlay();
			
        //If we received data and are playing, write to the audio track
        if (pcmData != null && amountToRead > 0 && audioTrack != null && playerState.isPlaying()) {
            audioTrack.write(pcmData, 0, amountToRead);
            //Log.d("DataSource", "audio track write");
            // count data
            writtenPCMData += amountToRead;
            writtenMiliSeconds += convertBytesToMs(amountToRead);


            /*
             * The idea here is we are loosing some seconds when the packages can't be decoded (on seek, when jumping in the middle of a package), so the overall time count is behind the real position
             * So when we know the time size of a stream, we can simply keep count of the bytes read form the source, and compute the time position proportionally
             */
            if (streamSecondsLength > 0 && data.getSourceLength() > 0) {            	
            	writtenMiliSeconds = (data.getReadOffset() * streamSecondsLength * 1000) / data.getSourceLength();
            }

            // send a notification of progress
            events.sendEvent(PlayerEvents.PLAY_UPDATE, (int) (writtenMiliSeconds / 1000));
            
            // at this point we know all stream parameters, including the sampleRate, use it to compute current time.
            //Log.e(TAG, "sample rate: " + streamInfo.getSampleRate() + " " + streamInfo.getChannels() + " " + streamInfo.getVendor() +  " time:" + writtenMiliSeconds + " bytes:" + writtenPCMData);
        } else {
            Log.e("DataSource", "audio track error");
        }
    }

    public void stopAudioTrack() {
        //Stop the audio track
        if (audioTrack != null) {
            Log.d(TAG, "Audiotrack flush");
            audioTrack.flush();
            try {
                audioTrack.stop();
            } catch (Exception ex) {
                Log.e(TAG, "Audiotrack stop ex:"+ex.getMessage());
            }
            audioTrack = null;

        }
    }
    /**
     * Called when decoding has completed and we consumed all input data
     */
    @Override
    public synchronized void onStop() {
        if (!playerState.isStopped()) {
        	writtenPCMData = 0;
            writtenMiliSeconds = 0;
            //Closes the file input stream
            if (data.isSourceValid())
            	data.release();

            Log.d("Player_Status", "decoding complete");
            stopAudioTrack();
        }
        //Set our state to stopped
        playerState.set(PlayerStates.STOPPED);
    }

    /**
     * Called when reading header is complete and we are ready to play the stream. decoding has started
     * @param sampleRate
     * @param channels
     * @param vendor
     * @param title
     * @param artist
     * @param album
     * @param date
     * @param track
     */
    @Override
    public void onStart(long sampleRate, long channels, String vendor, String title, String artist, String album, String date, String track)    
 {
    	DecodeStreamInfo decodeStreamInfo = new DecodeStreamInfo(sampleRate, channels, vendor, title, artist, album, date, track);
        Log.e(TAG, "onStart state:" + playerState.get());
        
        Log.e(TAG, "len tests:" + data.getSourceLength() + " " + sampleRate + " " + channels + " dura:" + ((data.getSourceLength() * 8) / (sampleRate * channels)) );

    	if (playerState.get() != PlayerStates.READING_HEADER &&
        		playerState.get() != PlayerStates.PLAYING) {
            //throw new IllegalStateException("Must read header first!");
            return;
        }
        if (decodeStreamInfo.getChannels() != 1 && decodeStreamInfo.getChannels() != 2) {
            throw new IllegalArgumentException("Channels can only be one or two");
        }
        if (decodeStreamInfo.getSampleRate() <= 0) {
            throw new IllegalArgumentException("Invalid sample rate, must be above 0");
        }
        // TODO: finish initing
        Log.d(TAG, "onStart call ok (Vendor:" + decodeStreamInfo.getVendor() + ") Track parameters: Title:"+ decodeStreamInfo.getTitle() + " Artist:"+decodeStreamInfo.getArtist() + 
        		" Album:" + decodeStreamInfo.getAlbum() + " Date:" + decodeStreamInfo.getDate() + " Track:" + decodeStreamInfo.getTrack());

        streamInfo = decodeStreamInfo;
        
        // we are already playing but track changed
        if (playerState.get() != PlayerStates.STOPPED) {
            Log.d("Player_Status", "change track");
            stopAudioTrack();
        }
        
        //Create the audio track
        int channelConfiguration = decodeStreamInfo.getChannels() == 1 ? AudioFormat.CHANNEL_OUT_MONO : AudioFormat.CHANNEL_OUT_STEREO;
        int minSize = AudioTrack.getMinBufferSize((int) decodeStreamInfo.getSampleRate(), channelConfiguration, AudioFormat.ENCODING_PCM_16BIT);
        audioTrack = new AudioTrack(AudioManager.STREAM_MUSIC, (int) decodeStreamInfo.getSampleRate(), channelConfiguration, 
        		AudioFormat.ENCODING_PCM_16BIT, minSize, AudioTrack.MODE_STREAM);

        audioTrack.play();
        
       
        if (playerState.get() == PlayerStates.READING_HEADER) {
	        events.sendEvent(PlayerEvents.READY_TO_PLAY);
	        //We're ready to starting to read actual content
	        playerState.set(PlayerStates.READY_TO_PLAY);
        }
        
        events.sendEvent(PlayerEvents.TRACK_INFO, decodeStreamInfo.getVendor(),
    			decodeStreamInfo.getTitle(),
    			decodeStreamInfo.getArtist(),
    			decodeStreamInfo.getAlbum(),
    			decodeStreamInfo.getDate(),
    			decodeStreamInfo.getTrack());
        
    }

    /**
     * Puts the decode feed in the reading header state
     */
    @Override
    public void onStartReadingHeader() {
        if (playerState.isStopped()) {
        	events.sendEvent(PlayerEvents.READING_HEADER);
            playerState.set(PlayerStates.READING_HEADER);
        }
    }

    
	/**
	 * returns the number of bytes used by a buffer of given mili seconds, sample rate and channels
	 * we multiply by 2 to compensate for the 'short' size
	 */
	public static int convertMsToBytes(int ms, long sampleRate, long channels ) {
        return (int)(((long) ms) * sampleRate * channels / 1000) * 2; 
    }
	public int converMsToBytes(int ms) {
		return convertMsToBytes(ms, streamInfo.getSampleRate(), streamInfo.getChannels());
	}
	
	/**
     * returns the number of samples needed to hold a buffer of given mili seconds, sample rate and channels
     */
    public static int convertMsToSamples( int ms, long sampleRate, long channels ) {
        return (int)(((long) ms) * sampleRate * channels / 1000);
    }
    public int convertMsToSamples(int ms) {
        return convertMsToSamples(ms, streamInfo.getSampleRate(), streamInfo.getChannels());
    }

    /**
     * converts bytes of buffer to mili seconds
     * we divide by 2 to compensate for the 'short' size
     */
    public static int convertBytesToMs( long bytes, long sampleRate, long channels ) {
        return (int)(1000L * bytes / (sampleRate * channels));
    }
    public int convertBytesToMs( long bytes) {
        return convertBytesToMs(bytes, streamInfo.getSampleRate(), streamInfo.getChannels());
    }

    /**
     * Converts samples of buffer to milliseconds.
     * @param samples the size of the buffer in samples (all channels)
     * @return the time in milliseconds
     */
    public static int convertSamplesToMs( int samples, long sampleRate, long channels ) {
        return (int)(1000L * samples / (sampleRate * channels));
    }
    public int convertSamplesToMs( int samples) {
        return convertSamplesToMs(samples, streamInfo.getSampleRate(), streamInfo.getChannels());
    }

    /**
     * Return DataSource of the current feed
     */
    public DataSource getDataSource() {
        return data;
    }
}