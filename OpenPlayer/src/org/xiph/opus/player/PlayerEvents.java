package org.xiph.opus.player;

import android.os.Handler;
import android.os.Message;


public class PlayerEvents {
    /**
     * Playing finished handler message
     */
    public static final int PLAYING_FINISHED = 1001;

    /**
     * Playing failed handler message
     */
    public static final int PLAYING_FAILED = 1002;

    /**
     * Started to read the stream
     */
    public static final int READING_HEADER = 1003;

    /**
     * Header was received, we are ready to play
     */
    public static final int READY_TO_PLAY = 1004;
    
    /**
     * Progress indicator, sent out periodically when playing
     */
    public static final int PLAY_UPDATE = 1005;
    
    
    /**
     * Handler for sending status updates
     */
    private final Handler handler;
    
    /**
     * Constructs a new instance of the PlayerEvents
     *
     * @param handler    handler to send player status updates to
     */
    public PlayerEvents(Handler handler) {
        if (handler == null) {
            throw new IllegalArgumentException("Handler must not be null.");
        }
        this.handler = handler;
    }
    
    public void sendEvent(int event) {
    	handler.sendEmptyMessage(event);
    }
    
    public void sendEvent(int event, int param) {
    	Message msg = new Message();
    	msg.arg1 = param;
    	msg.what = event;
    	handler.sendMessage(msg);
    } 
}
