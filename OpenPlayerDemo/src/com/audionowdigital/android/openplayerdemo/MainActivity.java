package com.audionowdigital.android.openplayerdemo;

import java.io.BufferedInputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.IOException;
import java.io.InputStream;
import java.net.MalformedURLException;
import java.net.URL;
import java.net.URLConnection;

import com.audionowdigital.android.openplayer.Player;
import com.audionowdigital.android.openplayer.Player.DecoderType;
import com.audionowdigital.android.openplayer.PlayerEvents;

import android.app.Activity;
import android.os.Bundle;
import android.os.Environment;
import android.os.Handler;
import android.os.Message;
import android.util.Log;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.ViewGroup.LayoutParams;
import android.widget.Button;
import android.widget.EditText;
import android.widget.LinearLayout;
import android.widget.ScrollView;
import android.widget.SeekBar;
import android.widget.TextView;


// This activity demonstrates how to use JNI to encode and decode ogg/vorbis audio
public class MainActivity extends Activity {
 
	/*
	 * DEFINE PLAYER TYPE
	 */
	Player.DecoderType type = DecoderType.MX;
	
	private static final String TAG = "MainActivity ";
	/*
	http://icecast1.pulsradio.com:80/mxHD.ogg
	http://test01.va.audionow.com:8000/eugen_vorbis
    http://ice01.va.audionow.com:8000/sagalswahiliopus.ogg
	http://stream-tx4.radioparadise.com:80/ogg-96
    http://markosoft.ro/test.ogg
	http://test01.va.audionow.com:8000/eugen_opus_lo
	http://test01.va.audionow.com:8000/eugen_opus_hi
	http://test01.va.audionow.com:8000/eugen_opus
	http://ai-radio.org:8000/radio.opus
	*/

	private Player player;
    
    // Playback handler for callbacks
    private Handler playbackHandler;

    // Text view to show logged messages
    private TextView logArea;
    private SeekBar seekBar;
    
    private int LENGTH = 0;
    
    private int DEBUG_PODCAST_LENGTH = 200;
    // Creates and sets our activities layout
    @Override public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        ScrollView scrollView = new ScrollView(this);
        setContentView(scrollView);
        LinearLayout panelV = new LinearLayout(this);
        panelV.setOrientation(LinearLayout.VERTICAL);
//        setContentView(panelV);
        
        scrollView.addView(panelV);
        logArea = new TextView(this);
        logArea.setText("Welcome to OPENPlayer v 1.0.110  Press Init->Play");
        panelV.addView(logArea);
        
        Button b = new Button(this);
        b.setText("init with file");
        b.setOnClickListener(new OnClickListener() {
			@Override public void onClick(View arg0) {
				logArea.setText("");
				switch (type) {
					case VORBIS: player.setDataSource("/sdcard/countdown.ogg", 11); break;
					case OPUS: player.setDataSource("/sdcard/countdown.opus", 11); break;
					case MX: player.setDataSource("/sdcard/countdown.mp3", 11); break;
				}
			
		    }
		});
        panelV.addView(b);
        
        final EditText et = new EditText(this);
        et.setTextSize(10);
        switch (type) {
        	case VORBIS: //et.setText("http://icecast1.pulsradio.com:80/mxHD.ogg"); LENGTH = -1; break;
        		et.setText("http://markosoft.ro/test.ogg"); LENGTH = 215; break;
        	case OPUS: et.setText("http://www.markosoft.ro/opus/02_Archangel.opus"); LENGTH = 154; break;
        	case MX: 
        		et.setText("http://stream.rfi.fr/rfimonde/all/rfimonde-64k.mp3"); LENGTH = -1; break;
        		//et.setText("http://www.pocketmagic.net/tmp3/demondance.mp3"); LENGTH = 30; break; // 30sec
        }
        
        panelV.addView(et);
        
        b = new Button(this);
        b.setText("init with URL");
        b.setOnClickListener(new OnClickListener() {
			@Override public void onClick(View arg0) {
				logArea.setText("");
				Log.d(TAG, "Set source:" + et.getEditableText().toString());
				//InputStream urlStrem = getStreamURL(et.getEditableText().toString());
				//player.setDataSource(et.getEditableText().toString(), 17169);
				player.setDataSource(et.getEditableText().toString(), LENGTH);
		    }
		});
        panelV.addView(b);

        b = new Button(this);
        b.setText("Play");
        b.setOnClickListener(new OnClickListener() {
			@Override public void onClick(View arg0) {
				if (player != null && player.isReadyToPlay()) {
					logArea.setText("Playing... ");
					player.play();
		        } else 
		        	logArea.setText("Player not initialized or not ready to play");
			}
		});
        panelV.addView(b);
        
        b = new Button(this);
        b.setText("Pause");
        b.setOnClickListener(new OnClickListener() {
			@Override public void onClick(View arg0) {
				if (player != null && player.isPlaying() ) {
					logArea.setText("Paused");
					player.pause();
				} else
					logArea.setText("Player not initialized or not playing");
			}
		});
        panelV.addView(b);
        
        b = new Button(this);
        b.setText("Stop");
        b.setOnClickListener(new OnClickListener() {
			@Override public void onClick(View arg0) {
				if (player != null) {
					logArea.setText("Stopped");
					player.stop();	
				}
				else
					logArea.setText("Player not initialized");
			}
		});
        panelV.addView(b);
        

        seekBar = new SeekBar(this);
        seekBar.setLayoutParams(new LinearLayout.LayoutParams(LayoutParams.MATCH_PARENT, LayoutParams.WRAP_CONTENT));
        seekBar.setMax(100);
        seekBar.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener() {
            @Override public void onProgressChanged(SeekBar seekBar, int i, boolean b) {
            	if (b) {
            		Log.d(TAG, "Seek:"+i+""); player.setPosition(i);
            	}
            }
            @Override public void onStartTrackingTouch(SeekBar seekBar) {}
            @Override public void onStopTrackingTouch(SeekBar seekBar) {}
        });
        
        panelV.addView(seekBar);
        
       
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
                    	logArea.setText("Playing:" + (msg.arg1 / 60) + ":" + (msg.arg1 % 60) + " (" + (msg.arg1 ) + "s)");
                    	seekBar.setProgress( (int) (msg.arg1 * 100/ player.getDuration()) );
                        break;
                    case PlayerEvents.TRACK_INFO:
                    	Bundle data = msg.getData();
                    	logArea.setText("title:" + data.getString("title") + " artist:" + data.getString("artist") + " album:"+ data.getString("album") + 
                    			" date:" + data.getString("date") + " track:" + data.getString("track"));
                    	break;
                }
            }
        };

        
        // quick test for a quick player
        player = new Player( playbackHandler, type);
        
    }

	


}
