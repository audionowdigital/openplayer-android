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
public class BufferedDecodeFeed implements DecodeFeed {
	/**
	 * The debug tag
	 */
	private String TAG = "BufferedDecodeFeed";
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
     * The initial buffer size
     */
    private final long bufferSize;
    
    /**
     * Content Read
     */
    private long readSize = 0;

    /**
     * The input stream to decode from
     */
    private InputStream inputStream;

    /**
     * The amount of written pcm data to the audio track
     */
    private long writtenPCMData = 0;

    /**
     * Creates a decode feed that reads from a file and writes to an {@link AudioTrack}
     *
     * @param streamToDecode the stream to decode
     */
    public BufferedDecodeFeed(InputStream streamToDecode, long bufferSize, PlayerStates playerState, PlayerEvents events) {
        if (streamToDecode == null) {
            throw new IllegalArgumentException("Stream to decode must not be null.");
        }
        this.playerState = playerState;
        this.events = events;
        this.inputStream = streamToDecode;
        this.bufferSize = bufferSize;
    }

    @Override
    public int onReadVorbisData(byte[] buffer, int amountToRead) {
        //If the player is not playing or reading the header, return 0 to end the native decode method
        if (playerState.get() == PlayerStates.STOPPED) {
            return 0;
        }

        //Otherwise read from the file
        try {
            Log.d(TAG, "Reading...");
            int read = inputStream.read(buffer, 0, amountToRead);
            readSize += read;
            Log.d(TAG, "Read...so far:" + readSize);
            return read == -1 ? 0 : read;
        } catch (IOException e) {
            //There was a problem reading from the file
            Log.e(TAG, "Failed to read vorbis data from file.  Aborting.", e);
            return 0;
        }
    }

    @Override
    public void onWritePCMData(short[] pcmData, int amountToWrite) {
        //If we received data and are playing, write to the audio track
        Log.d(TAG, "Writing data to track");
        if (pcmData != null && amountToWrite > 0 && audioTrack != null && (playerState.isPlaying() || playerState.isBuffering())) {
        	Log.d(TAG, "writePCMData:" + amountToWrite);
            audioTrack.write(pcmData, 0, amountToWrite);
            writtenPCMData += amountToWrite;
            if (writtenPCMData >= bufferSize) {
                audioTrack.play();
                playerState.set(PlayerStates.PLAYING);
            }
        }
    }

    @Override
    public void onStop() {
    	Log.d(TAG, "stop state:" + playerState.get());
        if (!playerState.isStopped()) {
            //If we were in a state of buffering before we actually started playing, start playing and write some silence to the track
            if (playerState.get() == PlayerStates.BUFFERING) {
                audioTrack.play();
                audioTrack.write(new byte[20000], 0, 20000);
                Log.d(TAG, "silence written");
            }

            //Closes the file input stream
            if (inputStream != null) {
                try {
                    inputStream.close();
                    Log.d(TAG, "stream closed");
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
                Log.d(TAG, "audio track stopped");
            }
        }
        //Set our state to stopped
        playerState.set(PlayerStates.STOPPED);
    }

    @Override
    public void onStart(DecodeStreamInfo decodeStreamInfo) {
    	Log.d(TAG, "onStart call:" + decodeStreamInfo.getChannels() + " " + decodeStreamInfo.getSampleRate() + " " + decodeStreamInfo.getVendor());
    	
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
        playerState.set(PlayerStates.BUFFERING);
    }

    @Override
    public void onStartReadingHeader() {
    	Log.d(TAG, "onStartReadingHeader call:" + playerState.get());
        if (playerState.isStopped()) {
            events.sendEvent(PlayerEvents.PLAYING_STARTED);
            playerState.set(PlayerStates.READING_HEADER);
        }
    }

}
