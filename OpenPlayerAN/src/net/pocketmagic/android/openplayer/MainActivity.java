package net.pocketmagic.android.openplayer;

import android.app.Activity;
import android.os.Bundle;
import android.os.Environment;
import android.os.Handler;
import android.os.Message;
import android.text.method.ScrollingMovementMethod;
import android.util.Log;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.Button;
import android.widget.LinearLayout;
import android.widget.RadioButton;
import android.widget.RadioGroup;
import android.widget.Spinner;
import android.widget.TextView;
import android.widget.Toast;

import org.xiph.vorbis.player.PlayerEvents;
import org.xiph.vorbis.player.VorbisPlayer;

import java.io.File;
import java.io.FileNotFoundException;
import java.io.IOException;
import java.io.InputStream;
import java.net.MalformedURLException;
import java.net.URL;
import java.net.URLConnection;


// This activity demonstrates how to use JNI to encode and decode ogg/vorbis audio
public class MainActivity extends Activity {
 
	private static final String TAG = "MainActivity";
    // The vorbis player
    private VorbisPlayer vorbisPlayer;
    // Playback handler for callbacks
    private Handler playbackHandler;

    // Text view to show logged messages
    private TextView logArea;

    // Creates and sets our activities layout
    @Override public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        
        LinearLayout panelV = new LinearLayout(this);
        panelV.setOrientation(LinearLayout.VERTICAL);
        setContentView(panelV);
        
        logArea = new TextView(this);
        panelV.addView(logArea);
        
        Button b = new Button(this);
        b.setText("Play");
        b.setOnClickListener(playClicked);
        panelV.addView(b);
        
        b = new Button(this);
        b.setText("Play file");
        b.setOnClickListener(playClicked2);
        panelV.addView(b);

        b = new Button(this);
        b.setText("Stop");
        b.setOnClickListener(stopClicked);
        panelV.addView(b);
        
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
                    case PlayerEvents.PLAYING_STARTED:
                    	logArea.setText("Starting to decode");
                        break;
                }
            }
        };
    }

    OnClickListener playClicked = new OnClickListener() {
		@Override
		public void onClick(View v) {
			logArea.setText("");
			//Checks whether the vorbis player is playing
	        if (vorbisPlayer == null || vorbisPlayer.isStopped()) {
	            
	            //String url = "http://test01.va.audionow.com:8000/eugen_vorbis";
	        	String url = "http://icecast1.pulsradio.com:80/mxHD.ogg";
	            URLConnection cn = null;
	            InputStream is = null;
				try {
					cn = new URL( url ).openConnection();
					cn.connect();
					is = cn.getInputStream();
				} catch (MalformedURLException e1) {
					// TODO Auto-generated catch block
					e1.printStackTrace();
				} catch (IOException e1) {
					// TODO Auto-generated catch block
					e1.printStackTrace();
				}
	            
	            // check URL for type of content!
	            for (java.util.Map.Entry<String, java.util.List<String>> me : cn.getHeaderFields().entrySet()) {
	                if ("content-type".equalsIgnoreCase( me.getKey())) {
	                    for (String s : me.getValue()) {
	                        String ct = s;
	                        Log.e(TAG, "content:" + s); // 04-18 13:20:51.121: E/Player(7417): content:audio/aacp
	                    }
	                }
	            }
	            //cn.getInputStream().toString());
	            // TODO: try to get the expectedKBitSecRate from headers
	            //Create the vorbis player if necessary
	            if (vorbisPlayer == null) {
	                vorbisPlayer = new VorbisPlayer(is, playbackHandler);
	            }

	            //Start playing the vorbis audio
	            vorbisPlayer.start();
	        }
		}
	};
	
	OnClickListener playClicked2 = new OnClickListener() {
		@Override
		public void onClick(View v) {
			Log.d(TAG, "play file");
			logArea.setText("");
			if (vorbisPlayer == null || vorbisPlayer.isStopped()) {
	            //Get file to play
	            File fileToPlay = new File(Environment.getExternalStorageDirectory(), "tsfh - arc.ogg");//test-katie.ogg");
	            
	            //Create the vorbis player if necessary
	            if (vorbisPlayer == null) {
	                try {
	                    vorbisPlayer = new VorbisPlayer(fileToPlay, playbackHandler);
	                } catch (FileNotFoundException e) {
	                    Log.e(TAG, "Failed to find saveTo.ogg", e);
	                    Toast.makeText(MainActivity.this, "Failed to find file to play! ", 2000).show();
	                }
	            }
	            // 
	            Log.d(TAG, "starting player");
	            //Start playing the vorbis audio
	            vorbisPlayer.start();
	        }
		}
	};
	
	OnClickListener stopClicked = new OnClickListener() {
		@Override
		public void onClick(View v) {
			if (vorbisPlayer != null && vorbisPlayer.isPlaying()) {
	            vorbisPlayer.stop();
	        }
		}
	};


}
