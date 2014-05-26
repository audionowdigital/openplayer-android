package org.xiph.vorbis.decoderjni;

/**
 * A class to define stream info, called with native {@link start} method in {@link DecodeFeed}
 */
public class DecodeStreamInfo {
    private long sampleRate;

    private long channels;

    private String vendor;
    
    private String title, artist, album, date, track;

    public DecodeStreamInfo(long sampleRate, long channels, String vendor, String title, String artist, String album, String date, String track) {
        this.sampleRate = sampleRate;
        this.channels = channels;
        this.vendor = vendor;
        // additional info
        this.title = title;
        this.artist = artist;
        this.album = album;
        this.date = date;
        this.track = track;
    }

    public long getSampleRate() {
        return sampleRate;
    }

    public void setSampleRate(long sampleRate) {
        this.sampleRate = sampleRate;
    }

    public long getChannels() {
        return channels;
    }

    public void setChannels(long channels) {
        this.channels = channels;
    }

    public String getVendor() {
        return vendor;
    }

    public void setVendor(String vendor) {
        this.vendor = vendor;
    }
    
    // Getters for the additional data
    public String getTitle() {
        return title;
    }
    public String getArtist() {
        return artist;
    }
    public String getAlbum() {
        return album;
    }
    public String getDate() {
        return date;
    }
    public String getTrack() {
        return track;
    }
}
