package org.xiph.opus.player;

import android.util.Log;

public class PlayerStates {
	 /**
     * Playing state which can either be stopped, playing, or reading the header before playing
     */
    public static final int
    	READY_TO_PLAY = 0,
        PLAYING = 1, 
        STOPPED = 2, 
        READING_HEADER = 3; 
        //BUFFERING = 4;
    
    public int playerState = STOPPED;
    
    public int get() {
    	return playerState;
    }
    
    public void set(int state) { 
    	Log.e("PlayerStates", "new state:"+state);
    	playerState = state;
    }
    
    /**
     * Checks whether the player is currently playing
     *
     * @return <code>true</code> if playing, <code>false</code> otherwise
     */
    public synchronized boolean isPlaying() {
        return playerState == PlayerStates.PLAYING;
    }
    
    /**
     * Checks whether the player is ready to play, this is the state used also for Pause
     *
     * @return <code>true</code> if ready, <code>false</code> otherwise
     */
    public synchronized boolean isReadyToPlay() {
        return playerState == PlayerStates.READY_TO_PLAY;
    }
    
    /**
     * Checks whether the player is currently stopped (not playing)
     *
     * @return <code>true</code> if playing, <code>false</code> otherwise
     */
    public synchronized boolean isStopped() {
        return playerState == PlayerStates.STOPPED;
    }

    /**
     * Checks whether the player is currently reading the header
     *
     * @return <code>true</code> if reading the header, <code>false</code> otherwise
     */
    public synchronized boolean isReadingHeader() {
        return playerState == PlayerStates.READING_HEADER;
    }


}
