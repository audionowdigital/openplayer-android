package com.audionowdigital.android.openplayerdemo;

import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.util.Log;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.Button;
import android.widget.EditText;
import android.widget.SeekBar;
import android.widget.TextView;
import com.audionowdigital.android.openplayer.Player;
import com.audionowdigital.android.openplayer.Player.DecoderType;
import com.audionowdigital.android.openplayer.PlayerEvents;
import roboguice.activity.RoboActivity;
import roboguice.inject.InjectView;


// This activity demonstrates how to use JNI to encode and decode ogg/vorbis audio
public class MainActivity extends RoboActivity {

    private static final String TAG = "MainActivity ";


    @InjectView(R.id.log_area)
    private TextView logArea;

    @InjectView(R.id.url_area)
    private EditText urlArea;

    @InjectView(R.id.init_file)
    private Button initWithFile;

    @InjectView(R.id.init_url)
    private Button initWithUrl;

    @InjectView(R.id.play)
    private Button play;

    @InjectView(R.id.pause)
    private Button pause;

    @InjectView(R.id.stop)
    private Button stop;

    @InjectView(R.id.seek)
    private SeekBar seekBar;

    private Player.DecoderType type = DecoderType.OPUS;

    private Player player;

    // Playback handler for callbacks
    private Handler playbackHandler;


    private int LENGTH = 0;


    // Creates and sets our activities layout
    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.main);
        initUi();

        switch (type) {
            case VORBIS: //urlArea.setText("http://icecast1.pulsradio.com:80/mxHD.ogg"); LENGTH = -1; break;
                urlArea.setText("http://markosoft.ro/test.ogg");
                LENGTH = 215;
                break;
            case OPUS:
                urlArea.setText("http://www.markosoft.ro/opus/02_Archangel.opus");
                LENGTH = 154;
                break;
            case MX:
                urlArea.setText("http://stream.rfi.fr/rfimonde/all/rfimonde-64k.mp3");
                LENGTH = -1;
                break;
        }

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
                        logArea.setText("Playing:" + (msg.arg1 / 60) + ":" + (msg.arg1 % 60) + " (" + (msg.arg1) + "s)");
                        seekBar.setProgress((int) (msg.arg1 * 100 / player.getDuration()));
                        break;
                    case PlayerEvents.TRACK_INFO:
                        Bundle data = msg.getData();
                        logArea.setText("title:" + data.getString("title") + " artist:" + data.getString("artist") + " album:" + data.getString("album") +
                                " date:" + data.getString("date") + " track:" + data.getString("track"));
                        break;
                }
            }
        };

        // quick test for a quick player
        player = new Player(playbackHandler, type);

    }

    private void initUi(){
        initWithFile.setOnClickListener(new OnClickListener() {
            @Override
            public void onClick(View arg0) {
                logArea.setText("");
                switch (type) {
                    case VORBIS:
                        player.setDataSource("/sdcard/countdown.ogg", 11);
                        break;
                    case OPUS:
                        player.setDataSource("/sdcard/countdown.opus", 11);
                        break;
                    case MX:
                        player.setDataSource("/sdcard/countdown.mp3", 11);
                        break;
                }
            }
        });

        initWithUrl.setOnClickListener(new OnClickListener() {
            @Override
            public void onClick(View arg0) {
                logArea.setText("");
                Log.d(TAG, "Set source:" + urlArea.getEditableText().toString());
                new Thread(new Runnable() {
                    @Override
                    public void run() {
                        player.setDataSource(urlArea.getEditableText().toString(), LENGTH);
                    }
                }).start();
            }
        });

        play.setOnClickListener(new OnClickListener() {
            @Override
            public void onClick(View arg0) {
                if (player != null && player.isReadyToPlay()) {
                    logArea.setText("Playing... ");
                    player.play();
                } else
                    logArea.setText("Player not initialized or not ready to play");
            }
        });

        pause.setOnClickListener(new OnClickListener() {
            @Override
            public void onClick(View arg0) {
                if (player != null && player.isPlaying()) {
                    logArea.setText("Paused");
                    player.pause();
                } else
                    logArea.setText("Player not initialized or not playing");
            }
        });

        stop.setOnClickListener(new OnClickListener() {
            @Override
            public void onClick(View arg0) {
                if (player != null) {
                    logArea.setText("Stopped");
                    player.stop();
                } else
                    logArea.setText("Player not initialized");
            }
        });

        seekBar.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener() {
            @Override
            public void onProgressChanged(SeekBar seekBar, int i, boolean b) {
                if (b) {
                    Log.d(TAG, "Seek:" + i + "");
                    player.setPosition(i);
                }
            }

            @Override
            public void onStartTrackingTouch(SeekBar seekBar) {
            }

            @Override
            public void onStopTrackingTouch(SeekBar seekBar) {
            }
        });
    }
}
