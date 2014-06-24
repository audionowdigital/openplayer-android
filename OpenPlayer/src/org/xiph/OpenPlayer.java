package org.xiph;

import java.io.InputStream;

/**
 * Created by andrei on 24/06/14.
 */
public interface OpenPlayer extends Runnable {

    /**
     * Set an input stream as data source and starts reading from it
     *
     * @param streamToDecode the stream to read from
     */
    public void setDataSource(InputStream streamToDecode, long streamSize, long streamLength);

    public void play();

    public void pause();

    public void stop();

    /**
     * Seek to a certain percentage in the current playing file.
     * @throws java.lang.IllegalStateException for live streams
     * @param percentage - position where to seek
     */
    public void setPosition(int percentage);

    public int getCurrentPosition();

}
