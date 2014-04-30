package org.xiph.vorbis.player;

import android.util.Log;

public class PlayerStates {
	 /**
     * Playing state which can either be stopped, playing, or reading the header before playing
     */
    public static final int
        PLAYING = 1, 
        STOPPED = 2, 
        READING_HEADER = 3, 
        BUFFERING = 4;
    
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

    /**
     * Checks whether the player is currently buffering
     *
     * @return <code>true</code> if buffering, <code>false</code> otherwise
     */
    public synchronized boolean isBuffering() {
        return playerState == PlayerStates.BUFFERING;
    }
}
