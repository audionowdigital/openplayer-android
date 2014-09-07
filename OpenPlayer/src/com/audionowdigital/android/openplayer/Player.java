package com.audionowdigital.android.openplayer;

import android.media.AudioTrack;
import android.os.Handler;
import android.os.Process;
import android.util.Log;

import net.pocketmagic.android.openmxplayer.MXDecoder;

import org.xiph.opus.decoderjni.OpusDecoder;
import org.xiph.vorbis.decoderjni.VorbisDecoder;

import java.io.InputStream;

/**
 * The OpusPlayer is responsible for decoding a opus bitstream into raw PCM data to play to an {@link AudioTrack}
 */
public class Player implements Runnable {

	public enum DecoderType {
		OPUS,
		VORBIS,
		MX,
		UNKNOWN
	};
	
	DecoderType type = DecoderType.UNKNOWN;
	
    /**
     * Logging tag
     */
    private static final String TAG = "Player";

    /**
     * The decode feed to read and write pcm/encoded data respectively
     */
    private  ImplDecodeFeed decodeFeed;

    /**
     * Current state of the player
     */
    private PlayerStates playerState = new PlayerStates();
    
    /**
     * The player events used to inform a client
     */
    private PlayerEvents events = null;

    private long streamSecondsLength = -1;

    public Player(Handler handler, DecoderType type) {
    	 if (handler == null) {
             throw new IllegalArgumentException("Handler must not be null.");
         }
    	 this.type = type;
    	 events = new PlayerEvents(handler);
    	 this.decodeFeed = new ImplDecodeFeed(playerState, events);
    	 
    	 
    	 // pass the DecodeFeed interface to the native JNI layer, we will get all calls there
    	 Log.d(TAG,"Player constructor, type:"+type);
    	/* switch (type) {
    	 case DecoderType.OPUS: 
    	 }*/
    	 Log.e(TAG, "preparing to init:"+type);
    	 switch (type) {
    		 case OPUS: OpusDecoder.initJni(1); break;
    		 case VORBIS: VorbisDecoder.initJni(1); break;
    		 case MX: MXDecoder.init(1); break;
		default:
			break;
    	 }
    	  
    }

    public void stopAudioTrack(){
        decodeFeed.stopAudioTrack();
        Log.d("Player_Status", "stop audio track");

    }

    /**
     * Set an input stream as data source and starts reading from it
     */
    public void setDataSource(String path, long streamSecondsLength) {
    	if (playerState.get() != PlayerStates.STOPPED) {
            throw new IllegalStateException("Must be stopped to change source!");
        }
    	Log.d(TAG, "setDataSource: given length:" + streamSecondsLength);
    	// set an input stream as data source
    	this.streamSecondsLength = streamSecondsLength;
    	decodeFeed.setData(path, streamSecondsLength);
    	// start the thread, will go directly to "run" method
    	new Thread(this).start();
    }

    /**
     * Return the data source from the decode feed
     */
    public DataSource getDataSource(){
        return decodeFeed.getDataSource();
    }

    public long getDuration() {
    	return streamSecondsLength;
    }

    public void play() {
        if (playerState.get() == PlayerStates.READING_HEADER){
            stop();
            return;
        }
        if (playerState.get() == PlayerStates.STOPPED){
            return;
        }
    	if (playerState.get() != PlayerStates.READY_TO_PLAY) {
            throw new IllegalStateException("Must be ready first!");    		
        }
    	playerState.set(PlayerStates.PLAYING);
    	// make sure the thread gets unlocked
    	decodeFeed.syncNotify();
    }
    
    public void pause() {
        if (playerState.get() == PlayerStates.READING_HEADER){
            stop();
            return;
        }
        if (playerState.get() == PlayerStates.READY_TO_PLAY){
            stop();
            return;
        }
        if (playerState.get() == PlayerStates.STOPPED){
            return;
        }
    	if (playerState.get() != PlayerStates.PLAYING) {
            throw new IllegalStateException("Must be playing first!");
        }
    	playerState.set(PlayerStates.READY_TO_PLAY);
    	// make sure the thread gets locked
    	decodeFeed.syncNotify();
    }
    
    /**
     * Stops the player and notifies the decode feed
     */
    public synchronized void stop() {
    	if (type == DecoderType.MX)
    		MXDecoder.stop();
    	
    	decodeFeed.onStop();
        // make sure the thread gets unlocked
    	decodeFeed.syncNotify();
    }
    

    @Override
    public void run() {
    	Log.e(TAG, "Start the native decoder");
        
        android.os.Process.setThreadPriority(Process.THREAD_PRIORITY_URGENT_AUDIO);
        
        int result = 0;
        switch (type) {
        	case OPUS:
        		Log.e(TAG, "call opus readwrite loop");
        		result = OpusDecoder.readDecodeWriteLoop(decodeFeed);
        	break;
        	case VORBIS:
        		Log.e(TAG, "call vorbis readwrite loop");
        		result = VorbisDecoder.readDecodeWriteLoop(decodeFeed);
        	break;
        	case MX:
        		Log.e(TAG, "call mx readwrite loop");
        		result = MXDecoder.readDecodeWriteLoop(decodeFeed);
        	break;
        }

        if (decodeFeed.getDataSource()!=null && !decodeFeed.getDataSource().isSourceValid()) {
            // Invalid data source
            events.sendEvent(PlayerEvents.PLAYING_FAILED);
            return;
        }
        Log.e(TAG, "Result: " + result);
        switch (result) {
            case DecodeFeed.SUCCESS:
                Log.d(TAG, "Successfully finished decoding");
                events.sendEvent(PlayerEvents.PLAYING_FINISHED);
                break;
            case DecodeFeed.INVALID_HEADER:
                events.sendEvent(PlayerEvents.PLAYING_FAILED);
                Log.e(TAG, "Invalid header error received");
                break;
        }
    }

    /**
     * Checks whether the player is currently playing
     *
     * @return <code>true</code> if playing, <code>false</code> otherwise
     */
    public synchronized boolean isPlaying() {
        return playerState.isPlaying();
    }

    /**
     * Checks whether the player is ready to play, this is the state used also for Pause
     *
     * @return <code>true</code> if ready, <code>false</code> otherwise
     */
    public synchronized boolean isReadyToPlay() {
        return playerState.isReadyToPlay();
    }
    
    /**
     * Checks whether the player is currently stopped (not playing)
     *
     * @return <code>true</code> if playing, <code>false</code> otherwise
     */
    public synchronized boolean isStopped() {
        return playerState.isStopped();
    }

    /**
     * Checks whether the player is currently reading the header
     *
     * @return <code>true</code> if reading the header, <code>false</code> otherwise
     */
    public synchronized boolean isReadingHeader() {
        return playerState.isReadingHeader();
    }

    /**
     * Seek to a certain percentage in the current playing file.
     * @throws java.lang.IllegalStateException for live streams
     * @param percentage - position where to seek
     */
    public synchronized void setPosition(int percentage) {
        decodeFeed.setPosition(percentage);
    }
    
    /**
     * 
     * @return returns the player position in track in seconds
     */
    public int getCurrentPosition(){
        return decodeFeed.getCurrentPosition();
    }

}
