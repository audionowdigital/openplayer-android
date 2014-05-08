package net.pocketmagic.android.openplayer;

import android.app.Activity;
import android.os.Bundle;
import android.os.Environment;
import android.os.Handler;
import android.os.Message;
import android.util.Log;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.ViewGroup;
import android.widget.*;

import org.xiph.opus.player.OpusPlayer;
import org.xiph.vorbis.player.PlayerEvents;
import org.xiph.vorbis.player.VorbisPlayer;

import java.io.*;
import java.net.MalformedURLException;
import java.net.URL;
import java.net.URLConnection;


// This activity demonstrates how to use JNI to encode and decode ogg/vorbis audio
public class MainActivity extends Activity {
 
	private static final String TAG = "MainActivity";
    // The vorbis player
    private VorbisPlayer vorbisPlayer;
    private OpusPlayer opusPlayer;
    
    // Playback handler for callbacks
    private Handler playbackHandler;

    // Text view to show logged messages
    private TextView logArea;

    private int DEBUG_PODCAST_LENGTH = 200;
    // Creates and sets our activities layout
    @Override public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        
        LinearLayout panelV = new LinearLayout(this);
        panelV.setOrientation(LinearLayout.VERTICAL);
        setContentView(panelV);
        
        logArea = new TextView(this);
        logArea.setText("Welcome to OPENPlayer v 1.0.101  Press Init->Play");
        panelV.addView(logArea);
        
        Button b = new Button(this);
        b.setText("init with file (/sdcard/test.ogg)");
        b.setOnClickListener(new OnClickListener() {
			@Override public void onClick(View arg0) {
				logArea.setText("");
				// TODO: andrei: buffer size inca nu e folosit, dar va trebui sa finalizez si partea aia, poti pune peste tot 24k
			    File decodedFile = getLocalFile("test.ogg");
                InputStream decodedStream = getLocalStream(decodedFile);
                vorbisPlayer.setDataSource(
						//getLocalFile("sita-1.1-final.opus")
                        decodedStream,
                        decodedFile.length(),
                        DEBUG_PODCAST_LENGTH
						);//test-katie.ogg");
		    }
		});
        panelV.addView(b);
        
        final EditText et = new EditText(this);
        et.setTextSize(10);
        et.setText("http://markosoft.ro/test.ogg"); //http://test01.va.audionow.com:8000/eugen_vorbis
        panelV.addView(et);
        
        b = new Button(this);
        b.setText("VORBIS init with URL");
        b.setOnClickListener(new OnClickListener() {
			@Override public void onClick(View arg0) {
				logArea.setText("");
				// String url = "http://test01.va.audionow.com:8000/eugen_vorbis";
		    	// String url = "http://icecast1.pulsradio.com:80/mxHD.ogg";
		        // TODO: andrei: buffer size inca nu e folosit, dar va trebui sa finalizez si partea aia, poti pune peste tot 24k
				InputStream urlStrem = getStreamURL(et.getEditableText().toString());
                vorbisPlayer.setDataSource(urlStrem, urlContentLength, DEBUG_PODCAST_LENGTH);
		    }
		});
        panelV.addView(b);


        b = new Button(this);
        b.setText("Play");
        b.setOnClickListener(new OnClickListener() {
			@Override public void onClick(View arg0) {
				if (vorbisPlayer != null && vorbisPlayer.isReadyToPlay()) {
					logArea.setText("Playing... ");
					vorbisPlayer.Play();
		        } else 
		        	logArea.setText("Player not initialized or not ready to play");
			}
		});
        panelV.addView(b);
        
        b = new Button(this);
        b.setText("Pause");
        b.setOnClickListener(new OnClickListener() {
			@Override public void onClick(View arg0) {
				if (vorbisPlayer != null && vorbisPlayer.isPlaying() ) {
					logArea.setText("Paused");
					vorbisPlayer.Pause();
				} else
					logArea.setText("Player not initialized or not playing");
			}
		});
        panelV.addView(b);
        
        b = new Button(this);
        b.setText("Stop");
        b.setOnClickListener(new OnClickListener() {
			@Override public void onClick(View arg0) {
				if (vorbisPlayer != null) {
					logArea.setText("Stopped");
					vorbisPlayer.Stop();	
				}
				else
					logArea.setText("Player not initialized");
			}
		});
        panelV.addView(b);
        
        //------------------- OPUS -------------- //
        final EditText etOpus = new EditText(this);
        etOpus.setTextSize(10);
        etOpus.setText("http://icecast1.pulsradio.com:80/mxHD.ogg"); //http://test01.va.audionow.com:8000/eugen_vorbis
        panelV.addView(etOpus);
        
        b = new Button(this);
        b.setText("OPUS init with URL");
        b.setOnClickListener(new OnClickListener() {
			@Override public void onClick(View arg0) {
				logArea.setText("");
				// String url = "http://test01.va.audionow.com:8000/eugen_vorbis";
		    	// String url = "http://icecast1.pulsradio.com:80/mxHD.ogg";
		        // TODO: andrei: buffer size inca nu e folosit, dar va trebui sa finalizez si partea aia, poti pune peste tot 24k
				opusPlayer.setDataSource(getStreamURL(etOpus.getEditableText().toString()), -1);
		    }
		});
        panelV.addView(b);


        b = new Button(this);
        b.setText("Opus Play");
        b.setOnClickListener(new OnClickListener() {
			@Override public void onClick(View arg0) {
				if (opusPlayer != null && opusPlayer.isReadyToPlay()) {
					logArea.setText("Playing... ");
					opusPlayer.Play();
		        } else 
		        	logArea.setText("Player not initialized or not ready to play");
			}
		});
        panelV.addView(b);
        
        b = new Button(this);
        b.setText("Opus Pause");
        b.setOnClickListener(new OnClickListener() {
			@Override public void onClick(View arg0) {
				if (opusPlayer != null && opusPlayer.isPlaying() ) {
					logArea.setText("Paused");
					opusPlayer.Pause();
				} else
					logArea.setText("Player not initialized or not playing");
			}
		});
        panelV.addView(b);
        
        b = new Button(this);
        b.setText("Opus Stop");
        b.setOnClickListener(new OnClickListener() {
			@Override public void onClick(View arg0) {
				if (opusPlayer != null) {
					logArea.setText("Stopped");
					opusPlayer.Stop();	
				}
				else
					logArea.setText("Player not initialized");
			}
		});
        panelV.addView(b);
        
        //------------------------------ //
        playbackHandler = new Handler() {
            @Override
            public void handleMessage(Message msg) {
                switch (msg.what) {
                    case PlayerEvents.PLAYING_FAILED:
                        logArea.setText("The decoder failed to playback the file, check logs for more details");
                        break;
                    case PlayerEvents.PLAYING_FINISHED:
                    	logArea.setText("The decoder finished successfully");
                        break;
                    case PlayerEvents.READING_HEADER:
                    	logArea.setText("Starting to read header");
                        break;
                    case PlayerEvents.READY_TO_PLAY:
                    	logArea.setText("READY to play - press play :)");
                        break;
                    case PlayerEvents.PLAY_UPDATE:
                    	logArea.setText("Playing:" + (msg.arg1 ) + "s");
                        break;
                }
            }
        };

        //Create the seekbar
        SeekBar seekBar = new SeekBar(this);
        seekBar.setLayoutParams(new RelativeLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.WRAP_CONTENT));
        seekBar.setMax(100);
        panelV.addView(seekBar);

        seekBar.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener() {
            @Override
            public void onProgressChanged(SeekBar seekBar, int i, boolean b) {
                Log.d("SEEK", i+"");
                vorbisPlayer.setPosition(i);

            }

            @Override
            public void onStartTrackingTouch(SeekBar seekBar) {

            }

            @Override
            public void onStopTrackingTouch(SeekBar seekBar) {

            }
        });
        // create the vorbis player
        vorbisPlayer = new VorbisPlayer( playbackHandler);
        
        // quick test for a quick player
        opusPlayer = new OpusPlayer( playbackHandler);
        
    }

 
	private File getLocalFile(String fileOnSdCard) {
		File fileToPlay = new File(Environment.getExternalStorageDirectory(), fileOnSdCard);//"tsfh - arc.ogg");//test-katie.ogg");
        return fileToPlay;
//        try {
//            return new BufferedInputStream(new FileInputStream(fileToPlay));
//        } catch (FileNotFoundException e) {}
//        return null;
	}

    private InputStream getLocalStream(File fileToPlay) {
        try {
            return new BufferedInputStream(new FileInputStream(fileToPlay));
        } catch (FileNotFoundException e) {}
        return null;
    }
    int urlContentLength = -1;
	private InputStream getStreamURL(String url) {
		try {
			URLConnection cn = new URL( url ).openConnection();
			cn.connect();
            urlContentLength = cn.getContentLength();
			return cn.getInputStream();
		} catch (MalformedURLException e1) {
			e1.printStackTrace();
		} catch (IOException e1) {
			e1.printStackTrace();
		}
        
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
	


}
