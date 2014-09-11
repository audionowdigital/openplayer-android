package com.audionowdigital.android.openplayer;

/**
 * A feed interface which raw PCM data will be written to and encoded  data will be read from
 */
public interface DecodeFeed {
    /**
     * Everything was a success
     */
    public static final int SUCCESS = 0;

    /**
     * The data is not a valid  header
     */
    public static final int INVALID_HEADER = -1;
    
    /**
     * failed to decode
     */
    public static final int DECODE_ERROR = -2;

    /**
     * Triggered from the native {@link Decoder} that is requesting to read the next bit of encoded data
     *
     * @param buffer        the buffer to write to
     * @param amountToWrite the amount of encoded data to write
     * @return the amount actually written
     */
    public int onReadEncodedData(byte[] buffer, int amountToWrite);

    /**
     * Triggered from the native {@link Decoder} that is requesting to write the next bit of raw PCM data
     *
     * @param pcmData      the raw pcm data
     * @param amountToRead the amount available to read in the buffer
     * @param currentSeconds if progress is known (MX decoder), we will push it here, else -1
     */
    public void onWritePCMData(short[] pcmData, int amountToRead, int currentSeconds);

    /**
     * To be called when decoding has completed
     */
    public void onStop();

    /**
     * Puts the decode feed in the reading header state
     */
    public void onStartReadingHeader();

    /**
     * To be called when reading header is complete and we are ready to play the stream. decoding has started
     *
     * @param decodeStreamInfo the stream information of what's about to be played
     */
    public void onStart(long sampleRate, long channels, String vendorString, String titleString, String artistString, String albumString,
    		String dateString, String trackString);    

    /**
     * Called to seek the stream to a position
     * @param percent - percentage where to seek
     */
    public void setPosition(int percent);
    
    public DataSource getDataSource();

}
