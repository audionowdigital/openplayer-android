package com.audionowdigital.android.openplayer;

import java.io.BufferedInputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.IOException;
import java.io.InputStream;
import java.net.MalformedURLException;
import java.net.URL;
import java.net.URLConnection;
import java.nio.ByteBuffer;

import android.util.Log;

public class DataSource  {

	/**
	 * The debug tag
	 */
	private String TAG = "DataSource";
	
	private final static int DATA_SRC_INVALID = -1;
	private final static int DATA_SRC_LOCAL = 0;
	private final static int DATA_SRC_REMOTE = 1;
	
	private InputStream inputStream;
	private String dataPath = null;
	private int dataSource = DATA_SRC_INVALID; 
	
	private long length = -1;
	
	private InputStream getRemote(String url, long offset) {
		try {
			URLConnection cn = new URL( url ).openConnection();
			cn.setRequestProperty ("Range", "bytes="+offset+"-");
			/* iOS implementtion:
			 NSString * str = [NSString stringWithFormat:@"GET %@ HTTP/1.0\r\nHost: %@\r\nRange: bytes=%ld-\r\n\r\n",
			                   [sourceUrl path], [sourceUrl host], offset];
			 */
			cn.connect();
			
			if (offset == 0)
				length = cn.getContentLength();
			
			return cn.getInputStream();
		} catch (MalformedURLException e ) { e.printStackTrace();
		} catch (IOException e) { e.printStackTrace(); }
        
        // check URL for type of content! other parameters can be accessed here!
        /*for (java.util.Map.Entry<String, java.util.List<String>> me : cn.getHeaderFields().entrySet()) {
            if ("content-type".equalsIgnoreCase( me.getKey())) {
                for (String s : me.getValue()) {
                    String ct = s;
                    Log.e(TAG, "content:" + s); // 04-18 13:20:51.121: E/Player(7417): content:audio/aacp
                }
            }
        }*/
		return null;
	}
	
	private InputStream getLocal(String path) {
		File f = new File(path);
		if (f.exists() && f.length() > 0) {
			try {
				InputStream is = new BufferedInputStream(new FileInputStream(f));
				length = f.length();
				is.markSupported();
		        is.mark((int)length);
		            
				return is;
			} catch (FileNotFoundException e) { e.printStackTrace(); }
		}
		return null;
	}
	
	/**
	 * 
	 * @param path can be a local path or a remote url
	 */
	DataSource(String path) {
		dataSource = DATA_SRC_INVALID;
		inputStream = null;
		dataPath = path;
		
		// first see if it's a local path
		inputStream = getLocal(path);
		if (inputStream != null ) {
			Log.d(TAG, "Local Source length:" + length);
			dataSource = DATA_SRC_LOCAL;
			return;
		} 
		
		inputStream = getRemote(path, 0);
		if (inputStream != null) {
			Log.d(TAG, "Remote Source length:" + length);
			dataSource = DATA_SRC_REMOTE;
			return;
		}
		
	}

	public void release() {
		Log.d(TAG, "release called.");
		try {
			inputStream.close();
		} catch (IOException e) { e.printStackTrace(); }
		inputStream = null;
		dataSource = DATA_SRC_INVALID;
	}
	
	/**
	 * 
	 * @return the current track size in bytes, or -1 for live streams
	 */
	public long getSourceLength() {
		return length;
	}
	
	public boolean isSourceValid() {
		return dataSource != DATA_SRC_INVALID;
	}

	
	public synchronized int read(byte buffer[], int byteOffset, int byteCount) {
		try {
			if (dataSource != DATA_SRC_INVALID)
				return inputStream.read(buffer, byteOffset, byteCount);
		} catch (IOException e) { e.printStackTrace(); }
	
		return DATA_SRC_INVALID;	
	}
	
	public synchronized int skip(long offset) {
		// invalid content fix : make sure we are pass the header always
		if (offset < 500) offset = 500;
		
		if (dataSource == DATA_SRC_LOCAL) {
			try {
				inputStream.reset(); // will reset to mark: TODO: test if memory is ok on local big files
				Long skip = inputStream.skip(offset);
			} catch (IOException e) {
				// TODO Auto-generated catch block
				e.printStackTrace();
			}
		}
		
		if (dataSource == DATA_SRC_REMOTE) {
			inputStream = getRemote(dataPath, offset);
			if (inputStream != null) {
				Log.d(TAG, "Skip reconnect to:" + offset);
				dataSource = DATA_SRC_REMOTE;
			}
		}
		
		return DATA_SRC_INVALID;
	}
}