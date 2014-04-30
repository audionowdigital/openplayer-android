package org.xiph.vorbis.player;

import java.io.File;
import java.io.FileNotFoundException;


import android.os.Handler;


public class PlayerEvents {
    /**
     * Playing finished handler message
     */
    public static final int PLAYING_FINISHED = 46314;

    /**
     * Playing failed handler message
     */
    public static final int PLAYING_FAILED = 46315;

    /**
     * Playing started handler message
     */
    public static final int PLAYING_STARTED = 46316;

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
    
}
