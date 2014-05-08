package org.xiph.opus.player;

import android.media.AudioTrack;
import android.os.Handler;
import android.os.Process;
import android.util.Log;
import org.xiph.opus.decodefeed.ImplDecodeFeed;
import org.xiph.opus.decoderjni.DecodeFeed;
import org.xiph.opus.decoderjni.OpusDecoder;

import java.io.InputStream;

/**
 * The OpusPlayer is responsible for decoding a opus bitsream into raw PCM data to play to an {@link AudioTrack}
 */
public class OpusPlayer implements Runnable {


    /**
     * Logging tag
     */
    private static final String TAG = "OpusPlayer";

    /**
     * The decode feed to read and write pcm/opus data respectively
     */
    private final ImplDecodeFeed decodeFeed;

    /**
     * Current state of the opus player
     */
    private PlayerStates playerState = new PlayerStates();
    
    /**
     * The player events used to inform a client
     */
    private PlayerEvents events = null;

    

    public OpusPlayer(Handler handler) {
    	 if (handler == null) {
             throw new IllegalArgumentException("Handler must not be null.");
         }
    	 events = new PlayerEvents(handler);
    	 this.decodeFeed = new ImplDecodeFeed(playerState, events);
    	 
    	 // pass the DecodeFeed interface to the native JNI layer, we will get all calls there
    	 Log.d(TAG,"OpusPlayer const.");
    	 int result = OpusDecoder.initJni(1);
    }
    
    /**
     * Set an input stream as data source and starts reading from it
     * @param streamToDecode the stream to read from 
     */
    public void setDataSource(InputStream streamToDecode, long streamLength) {
    	Log.d(TAG, "setDataSource:" + streamLength);
    	// set an input stream as data source
    	decodeFeed.setData(streamToDecode, streamLength);
    	// start the thread, will go directly to "run" method
    	new Thread(this).start();
    }



    public void Play() {
    	if (playerState.get() != PlayerStates.READY_TO_PLAY) {
            throw new IllegalStateException("Must be ready first!");
        }
    	playerState.set(PlayerStates.PLAYING);
    	// make sure the thread gets unlocked
    	decodeFeed.syncNotify();
    }
    
    public void Pause() {
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
    public synchronized void Stop() {
        decodeFeed.onStop();
        // make sure the thread gets unlocked
    	//decodeFeed.syncNotify();
    }
    

    @Override
    public void run() {
    	Log.e(TAG, "Start the native decoder");
        //Start the native decoder: VORBIS
        android.os.Process.setThreadPriority(Process.THREAD_PRIORITY_URGENT_AUDIO);
        
        int result = OpusDecoder.readDecodeWriteLoop(decodeFeed);
                
        Log.e(TAG, "Result: " + result);
        switch (result) {
            case DecodeFeed.SUCCESS:
                Log.d(TAG, "Successfully finished decoding");
                events.sendEvent(PlayerEvents.PLAYING_FINISHED);
                break;
            case DecodeFeed.INVALID_OGG_BITSTREAM:
            	events.sendEvent(PlayerEvents.PLAYING_FAILED);
                Log.e(TAG, "Invalid ogg bitstream error received");
                break;
            case DecodeFeed.ERROR_READING_FIRST_PAGE:
                events.sendEvent(PlayerEvents.PLAYING_FAILED);
                Log.e(TAG, "Error reading first page error received");
                break;
            case DecodeFeed.ERROR_READING_INITIAL_HEADER_PACKET:
                events.sendEvent(PlayerEvents.PLAYING_FAILED);
                Log.e(TAG, "Error reading initial header packet error received");
                break;
            case DecodeFeed.NOT_VORBIS_HEADER:
                events.sendEvent(PlayerEvents.PLAYING_FAILED);
                Log.e(TAG, "Not a opus header error received");
                break;
            case DecodeFeed.CORRUPT_SECONDARY_HEADER:
                events.sendEvent(PlayerEvents.PLAYING_FAILED);
                Log.e(TAG, "Corrupt secondary header error received");
                break;
            case DecodeFeed.PREMATURE_END_OF_FILE:
                events.sendEvent(PlayerEvents.PLAYING_FAILED);
                Log.e(TAG, "Premature end of file error received");
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
    
    public int getCurrentPosition(){
        return decodeFeed.getCurrentPosition();
    }
   
}
