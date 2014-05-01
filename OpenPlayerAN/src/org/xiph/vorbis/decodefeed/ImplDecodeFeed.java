package org.xiph.vorbis.decodefeed;

import java.io.IOException;
import java.io.InputStream;

import org.xiph.vorbis.decoderjni.DecodeFeed;
import org.xiph.vorbis.decoderjni.DecodeStreamInfo;
import org.xiph.vorbis.player.PlayerEvents;
import org.xiph.vorbis.player.PlayerStates;

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
     * The initial buffer size
     */
    protected long bufferSize = 0;

    /**
     * The input stream to decode from
     */
    protected InputStream inputStream;

    /**
     * The amount of written pcm data to the audio track
     */
    protected long writtenPCMData = 0;

    /**
     * Creates a decode feed that reads from a file and writes to an {@link AudioTrack}
     *
     * @param streamToDecode the stream to decode
     */
    
    public ImplDecodeFeed(PlayerStates playerState, PlayerEvents events) {
    	this.playerState = playerState;
        this.events = events;
	}

    
    public void setData(InputStream streamToDecode, long bufferSize) {
    	if (streamToDecode == null) {
            throw new IllegalArgumentException("Stream to decode must not be null.");
        }
    	this.inputStream = streamToDecode;
        this.bufferSize = bufferSize;
    }

    public synchronized void waitPlay(){
        while(playerState.get() == PlayerStates.READY_TO_PLAY){
            try {
                wait();
            } catch (InterruptedException e) {
                e.printStackTrace();
            }
        }
    }
    public synchronized void syncNotify() {
    	notify();
    }
    
    @Override
    public int onReadVorbisData(byte[] buffer, int amountToWrite) {
    	Log.d(TAG, "readVorbisData call: " + amountToWrite);
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
            Log.e(TAG, "Failed to read vorbis data from file.  Aborting.", e);
            return 0;
        }
    }

    @Override
    public synchronized void onWritePCMData(short[] pcmData, int amountToRead) {
		waitPlay();
		
        //If we received data and are playing, write to the audio track
        if (pcmData != null && amountToRead > 0 && audioTrack != null && playerState.isPlaying()) {
            audioTrack.write(pcmData, 0, amountToRead);
        }
    }

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

    @Override
    public void onStart(DecodeStreamInfo decodeStreamInfo) {
        if (playerState.get() != PlayerStates.READING_HEADER) {
            throw new IllegalStateException("Must read header first!");
        }
        if (decodeStreamInfo.getChannels() != 1 && decodeStreamInfo.getChannels() != 2) {
            throw new IllegalArgumentException("Channels can only be one or two");
        }
        if (decodeStreamInfo.getSampleRate() <= 0) {
            throw new IllegalArgumentException("Invalid sample rate, must be above 0");
        }

        //Create the audio track
        int channelConfiguration = decodeStreamInfo.getChannels() == 1 ? AudioFormat.CHANNEL_OUT_MONO : AudioFormat.CHANNEL_OUT_STEREO;
        int minSize = AudioTrack.getMinBufferSize((int) decodeStreamInfo.getSampleRate(), channelConfiguration, AudioFormat.ENCODING_PCM_16BIT);
        audioTrack = new AudioTrack(AudioManager.STREAM_MUSIC, (int) decodeStreamInfo.getSampleRate(), channelConfiguration, AudioFormat.ENCODING_PCM_16BIT, minSize, AudioTrack.MODE_STREAM);
        audioTrack.play();
        
        events.sendEvent(PlayerEvents.READY_TO_PLAY);

        //We're ready to starting to read actual content
        playerState.set(PlayerStates.READY_TO_PLAY); 
    }

    @Override
    public void onStartReadingHeader() {
        if (playerState.isStopped()) {
        	events.sendEvent(PlayerEvents.READING_HEADER);
            playerState.set(PlayerStates.READING_HEADER);
        }
    }


	@Override
	public void onNewIteration() {
		Log.d(TAG, "onNewIteration");
		
	}

}