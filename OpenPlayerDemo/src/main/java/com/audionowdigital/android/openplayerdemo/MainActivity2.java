package com.audionowdigital.android.openplayerdemo;

import android.app.Activity;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.util.Log;
import com.audionowdigital.android.openplayer.Player;
import com.audionowdigital.android.openplayer.PlayerEvents;

/**
 * Created by cristi on 12/12/14.
 */
public class MainActivity2 extends Activity {
    private static final String TAG = "OpenPlayer";


    private Player player;


    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        Handler playbackHandler = new Handler() {
            @Override
            public void handleMessage(Message msg) {
                switch (msg.what) {
                    case PlayerEvents.PLAYING_FAILED:
                        Log.d(TAG, "The decoder failed to playback the file, check logs for more details");
                        break;
                    case PlayerEvents.PLAYING_FINISHED:
                        Log.d(TAG, "The decoder finished successfully");
                        break;
                    case PlayerEvents.READING_HEADER:
                        Log.d(TAG, "Starting to read header");
                        break;
                    case PlayerEvents.READY_TO_PLAY:
                        Log.d(TAG, "READY to play - press play :)");
                        break;
                    case PlayerEvents.PLAY_UPDATE:
                        Log.d(TAG, "Playing:" + (msg.arg1 / 60) + ":" + (msg.arg1 % 60) + " (" + (msg.arg1) + "s)");
                        break;
                    case PlayerEvents.TRACK_INFO:
                        Bundle data = msg.getData();
                        Log.d(TAG, "title:" + data.getString("title") + " artist:" + data.getString("artist") + " album:" + data.getString("album") +
                                " date:" + data.getString("date") + " track:" + data.getString("track"));
                        break;
                }
            }
        };

        Player player = new Player(playbackHandler, Player.DecoderType.OPUS);

        new Thread(new Runnable() {
            @Override
            public void run() {
                player.setDataSource("http://www.markosoft.ro/opus/02_Archangel.opus", 200);

            }
        }).start();

    }
}
