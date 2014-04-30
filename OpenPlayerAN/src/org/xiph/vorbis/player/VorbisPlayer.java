package org.xiph.vorbis.player;

import android.media.AudioFormat;
import android.media.AudioManager;
import android.media.AudioTrack;
import android.os.*;
import android.os.Process;
import android.util.Log;

import org.xiph.vorbis.decodefeed.BufferedDecodeFeed;
import org.xiph.vorbis.decodefeed.FileDecodeFeed;
import org.xiph.vorbis.decoderjni.DecodeFeed;
import org.xiph.vorbis.decoderjni.DecodeStreamInfo;
import org.xiph.vorbis.decoderjni.VorbisDecoder;

import java.io.BufferedInputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.IOException;
import java.io.InputStream;
import java.net.URL;
import java.net.URLConnection;
import java.util.concurrent.atomic.AtomicReference;

/**
 * The VorbisPlayer is responsible for decoding a vorbis bitsream into raw PCM data to play to an {@link AudioTrack}
 */
public class VorbisPlayer implements Runnable {


    /**
     * Logging tag
     */
    private static final String TAG = "VorbisPlayer";

    /**
     * The decode feed to read and write pcm/vorbis data respectively
     */
    private final DecodeFeed decodeFeed;

    /**
     * Current state of the vorbis player
     */
    private PlayerStates playerState = new PlayerStates();
    
    /**
     * The player events used to inform a client
     */
    private PlayerEvents events = null;

   

    /**
     * Constructs a new instance of the player with default parameters other than it will decode from a file
     *
     * @param fileToPlay the file to play
     * @param handler    handler to send player status updates to
     * @throws FileNotFoundException thrown if the file could not be located/opened to playing
     */
    public VorbisPlayer(File fileToPlay, Handler handler) throws FileNotFoundException {
        if (fileToPlay == null) {
            throw new IllegalArgumentException("File to play must not be null.");
        }
        if (handler == null) {
            throw new IllegalArgumentException("Handler must not be null.");
        }
        Log.d(TAG, "new VorbisPlayer");
        events = new PlayerEvents(handler);
        this.decodeFeed = new FileDecodeFeed(fileToPlay, playerState, events);
    }

    /**
     * Constructs a player that will read from an {@link InputStream} and write to an {@link AudioTrack}
     *
     * @param audioDataStream the audio data stream to read from
     * @param handler         handler to send player status updates to
     */
    public VorbisPlayer(InputStream audioDataStream, Handler handler) {
        if (audioDataStream == null) {
            throw new IllegalArgumentException("Input stream must not be null.");
        }
        if (handler == null) {
            throw new IllegalArgumentException("Handler must not be null.");
        }
        events = new PlayerEvents(handler);
        this.decodeFeed = new BufferedDecodeFeed(audioDataStream, 24000, playerState, events);
    
    }

    
    /**
     * Constructs a player with a custom {@link DecodeFeed}
     *
     * @param decodeFeed the custom decode feed
     * @param handler    handler to send player status updates to
     */
    public VorbisPlayer(DecodeFeed decodeFeed, Handler handler) {
        if (decodeFeed == null) {
            throw new IllegalArgumentException("Decode feed must not be null.");
        }
        if (handler == null) {
            throw new IllegalArgumentException("Handler must not be null.");
        }
        this.decodeFeed = decodeFeed;
        events = new PlayerEvents(handler);
    }

    /**
     * Starts the audio recorder with a given sample rate and channels
     */
    @SuppressWarnings("all")
    public synchronized void start() {
        if (isStopped()) {
        	Log.d(TAG, "Starting new player thread state:" + playerState.get());
            new Thread(this).start();
        } else {
        	Log.d(TAG, "Not creating new thread.");
        }
    }

    /**
     * Stops the player and notifies the decode feed
     */
    public synchronized void stop() {
        decodeFeed.onStop();
    }

    @Override
    public void run() {
    	Log.e(TAG, "Start the native decoder");
        //Start the native decoder: VORBIS
        android.os.Process.setThreadPriority(Process.THREAD_PRIORITY_URGENT_AUDIO);
        int result = VorbisDecoder.startDecoding(decodeFeed);
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
                Log.e(TAG, "Not a vorbis header error received");
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
     * Checks whether the player is currently buffering
     *
     * @return <code>true</code> if buffering, <code>false</code> otherwise
     */
    public synchronized boolean isBuffering() {
        return playerState.isBuffering();
    }
   
}
