package org.xiph.vorbis.decodefeed;

import java.io.BufferedInputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
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
 * Custom class to easily decode from a file and write to an {@link AudioTrack}
 */
public class FileDecodeFeed implements DecodeFeed {
	/**
	 * The debug tag
	 */
	private String TAG = "FileDecodeFeed";
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
    private AudioTrack audioTrack;

    /**
     * The input stream to decode from
     */
    private InputStream inputStream;
    /**
     * The file to decode ogg/vorbis data from
     */
    private final File fileToDecode;

    /**
     * Creates a decode feed that reads from a file and writes to an {@link AudioTrack}
     *
     * @param fileToDecode the file to decode
     */
    public FileDecodeFeed(File fileToDecode, PlayerStates playerState, PlayerEvents events) throws FileNotFoundException {
        if (fileToDecode == null) {
            throw new IllegalArgumentException("File to decode must not be null.");
        }
        this.playerState = playerState;
        this.events = events;
        this.fileToDecode = fileToDecode;
    }

    @Override
    public synchronized int onReadVorbisData(byte[] buffer, int amountToWrite) {
    	Log.d(TAG, "readVorbisData " + amountToWrite + " state:"+playerState.get());
        //If the player is not playing or reading the header, return 0 to end the native decode method
        if (playerState.get() == PlayerStates.STOPPED) {
            return 0;
        }

        //Otherwise read from the file
        try {
            int read = inputStream.read(buffer, 0, amountToWrite);
            return read == -1 ? 0 : read;
        } catch (IOException e) {
            //There was a problem reading from the file
            Log.e(TAG, "Failed to read vorbis data from file.  Aborting.", e);
            onStop();
            return 0;
        }
    }

    @Override
    public synchronized void onWritePCMData(short[] pcmData, int amountToRead) {
    	Log.d(TAG, "writePCMData call:" + amountToRead);
        //If we received data and are playing, write to the audio track
        if (pcmData != null && amountToRead > 0 && audioTrack != null && playerState.isPlaying()) {
            audioTrack.write(pcmData, 0, amountToRead);
        }
    }

    @Override
    public void onStop() {
    	Log.d(TAG, "stop call " + playerState.get());
    	if (playerState.isPlaying() || playerState.isReadingHeader()) {
    		
    		
            
            //Closes the file input stream
            if (inputStream != null) {
                try {
                	Log.d(TAG, "closing inputstream");
                    inputStream.close();
                } catch (IOException e) {
                    Log.e(TAG, "Failed to close file input stream", e);
                }
                Log.d(TAG, "nulling inputstream");
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
    	Log.d(TAG, "start call: " + playerState.get());
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

        //We're starting to read actual content
        playerState.set(PlayerStates.PLAYING);
    }

    @Override
    public void onStartReadingHeader() {
    	Log.d(TAG, "startReadingHeader called.");
        if (inputStream == null && playerState.isStopped()) {
            events.sendEvent(PlayerEvents.PLAYING_STARTED);
            
            try {
                inputStream = new BufferedInputStream(new FileInputStream(fileToDecode));
                Log.d(TAG, "new buffer created");
                playerState.set(PlayerStates.READING_HEADER);
            } catch (FileNotFoundException e) {
                Log.e(TAG, "Failed to find file to decode", e);
                onStop();
            }
        }
    }

}
