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
     * The data is not a vorbis header
     */
    public static final int NOT_VORBIS_HEADER = -1;

    /**
     * The  header is corrupt
     */
    public static final int CORRUPT_HEADER = -2;

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
     * Called to seek the stream to a position
     * @param percent - percentage where to seek
     */
    public void setPosition(int percent);

}
