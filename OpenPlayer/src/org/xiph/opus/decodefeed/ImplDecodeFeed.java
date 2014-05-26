package org.xiph.opus.decodefeed;

import android.media.AudioFormat;
import android.media.AudioManager;
import android.media.AudioTrack;
import android.util.Log;

import org.xiph.opus.decoderjni.DecodeFeed;
import org.xiph.opus.decoderjni.DecodeStreamInfo;
import org.xiph.opus.decoderjni.OpusDecoder;
import org.xiph.opus.player.PlayerEvents;
import org.xiph.opus.player.PlayerStates;

import java.io.BufferedInputStream;
import java.io.IOException;
import java.io.InputStream;

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
    protected InputStream inputStream;
    
    protected long streamLength;
    
    protected long streamSize;

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
    	return (int) (writtenMiliSeconds);
    }
    /**
     * Pass a stream as data source
     * @param streamToDecode
     */
    public void setData(InputStream streamToDecode, long streamSize, long streamLength) {
    	if (streamToDecode == null) {
            throw new IllegalArgumentException("Stream to decode must not be null.");
        }
        this.streamLength = streamLength;
        this.streamSize = streamSize;

        // Make sure that for podcasts, the stream is a BufferedInputStream
        if (!(streamToDecode instanceof BufferedInputStream) && streamSize > 0) {
            this.inputStream = new BufferedInputStream(streamToDecode);
        } else {
            this.inputStream = streamToDecode;

        }
          if (streamSize > 0) {
            this.inputStream.markSupported();
            this.inputStream.mark((int)streamSize);
        }
    }
    
    /**
     * A pause mechanism that would block current thread when pause flag is set (READY_TO_PLAY)
     */
    public synchronized void waitPlay(){
        while(playerState.get() == PlayerStates.READY_TO_PLAY) {
            if (streamLength == -1){
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
     * Triggered from the native {@link VorbisDecoder} that is requesting to read the next bit of opus data
     *
     * @param buffer        the buffer to write to
     * @param amountToWrite the amount of opus data to write (from inputstream to our buffer)
     * @return the amount actually written
     */
    @Override public int onReadOpusData(byte[] buffer, int amountToWrite) {
    	//Log.d(TAG, "onReadOpusData call: " + amountToWrite);
        //If the player is not playing or reading the header, return 0 to end the native decode method
        if (playerState.get() == PlayerStates.STOPPED) {
            return 0;
        }
        
        waitPlay();

        //Otherwise read from the file
        try {
            int read = inputStream.read(buffer, 0, amountToWrite);
            return read == -1 ? 0 : read;
        } catch (IOException e) {
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
        if (streamSize < 0) {
            throw new IllegalStateException("Stream length must be a positive number");
        }
        long seekPosition = percent * streamSize / 100;
        if (inputStream!=null) {
            try {
                audioTrack.flush();
                inputStream.reset();
                long skippedBytes = 0;
                long skipSize;
                while (skippedBytes < seekPosition) {
                    skipSize = seekPosition - skippedBytes;
                    skippedBytes += inputStream.skip(skipSize);
                }
                Log.d("SEEK ","  " + inputStream.available() + " SKIP_POS  " + seekPosition + " SKIPPED:  " + skippedBytes + " SEC:" + streamLength + "  " + getCurrentPosition());
                writtenMiliSeconds = percent * streamLength / 100 * 1000; // save in millis for now.
            } catch (IOException e) {
                e.printStackTrace();
            }
        }
    }


    /**
     * Triggered from the native {@link VorbisDecoder} that is requesting to write the next bit of raw PCM data
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
            // count data
            writtenPCMData += amountToRead;
            writtenMiliSeconds += convertBytesToMs(amountToRead); 
            // send a notification of progress
            events.sendEvent(PlayerEvents.PLAY_UPDATE, (int) (writtenMiliSeconds / 1000));
            
            // at this point we know all stream parameters, including the sampleRate, use it to compute current time.
            Log.e(TAG, "sample rate: " + streamInfo.getSampleRate() + " " + streamInfo.getChannels() + " " + streamInfo.getVendor() + 
            		" time:" + writtenMiliSeconds + " bytes:" + writtenPCMData);
        }
    }

    /**
     * Called when decoding has completed and we consumed all input data
     */
    @Override
    public synchronized void onStop() {
        if (!playerState.isStopped()) {
            //Closes the file input stream
            if (inputStream != null) {
                try {
                    inputStream.close();
                } catch (IOException e) {
                    Log.e(TAG, "Failed to close file input stream", e);
                }
                inputStream = null;
            }

            //Stop the audio track
            if (audioTrack != null) {
                audioTrack.stop();
                audioTrack.release();
                audioTrack = null;
            }
        }

        //Set our state to stopped
        playerState.set(PlayerStates.STOPPED);
    }

    /**
     * Called when reading header is complete and we are ready to play the stream. decoding has started
     *
     * @param decodeStreamInfo the stream information of what's about to be played
     */
    @Override
    public void onStart(DecodeStreamInfo decodeStreamInfo) {
    	if (playerState.get() != PlayerStates.READING_HEADER &&
        		playerState.get() != PlayerStates.PLAYING) {
            throw new IllegalStateException("Must read header first!");
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

        writtenPCMData = 0; writtenMiliSeconds = 0;
        
        streamInfo = decodeStreamInfo;
        
        // we are already playing but track changed
        if (playerState.get() == PlayerStates.PLAYING) {
        	//Stop the audio track, we will restart it
            if (audioTrack != null) {
                audioTrack.stop();
                audioTrack.release();
                audioTrack = null;
            }
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
}