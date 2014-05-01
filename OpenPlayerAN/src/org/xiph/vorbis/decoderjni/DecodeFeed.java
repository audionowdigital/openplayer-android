package org.xiph.vorbis.decoderjni;

/**
 * A feed interface which raw PCM data will be written to and encoded vorbis data will be read from
 */
public interface DecodeFeed {
    /**
     * Everything was a success
     */
    public static final int SUCCESS = 0;

    /**
     * The bitstream is not ogg
     */
    public static final int INVALID_OGG_BITSTREAM = -21;

    /**
     * Failed to read first page
     */
    public static final int ERROR_READING_FIRST_PAGE = -22;

    /**
     * Failed reading the initial header packet
     */
    public static final int ERROR_READING_INITIAL_HEADER_PACKET = -23;

    /**
     * The data is not a vorbis header
     */
    public static final int NOT_VORBIS_HEADER = -24;

    /**
     * The secondary header is corrupt
     */
    public static final int CORRUPT_SECONDARY_HEADER = -25;

    /**
     * Reached a premature end of file
     */
    public static final int PREMATURE_END_OF_FILE = -26;

    /**
     * Triggered from the native {@link VorbisDecoder} that is requesting to read the next bit of vorbis data
     *
     * @param buffer        the buffer to write to
     * @param amountToWrite the amount of vorbis data to write
     * @return the amount actually written
     */
    public int onReadVorbisData(byte[] buffer, int amountToWrite);

    /**
     * Triggered from the native {@link VorbisDecoder} that is requesting to write the next bit of raw PCM data
     *
     * @param pcmData      the raw pcm data
     * @param amountToRead the amount available to read in the buffer
     */
    public void onWritePCMData(short[] pcmData, int amountToRead);

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
    public void onStart(DecodeStreamInfo decodeStreamInfo);
    
    /**
     * To be called from JNI when starting a new loop , useful to control pause
     */
    public void onNewIteration();
}
