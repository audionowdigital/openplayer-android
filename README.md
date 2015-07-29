#OpenPlayer for Android

OpenPlayer is an Android library developed by AudioNow Digital, for playing audio in your mobile applications. You can use it for:

*  Music
*  Radio streaming

It delivers support for the following codecs:

*  OPUS
*  VORBIS
*  MP3 - by accessing the native android library Stagefright

OpenPlayer delivers great performance, by using native codec implementation. It has been designed to decode OPUS and VORBIS content within an OGG Wrapper.


#Quick start

First of all, you need to add OpenPlayer to your project. If you are using Gradle, add the following line to your build.gradle file and sync your project.

`compile 'com.audionowdigital:openplayer:1.0.0'`


Using OpenPlayer to decode your media is very simple. 

**1 You need to create a Handler, which will receive the media events from the library**

```java
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
```

**2 Create a `Player` instance, using the created `Handler` and the media type you want to decode**

```java
    Player player = new Player(playbackHandler, Player.DecoderType.OPUS);
```

**3 Chose the content to play:**

```java
        player.setDataSource("http://www.yourserver.ro/test_file.opus", FILE_LENGTH_SECONDS);
```

**Note:** FILE_LENGTH_SECONDS is necessary for media files, for computing progress and allowing media seek. If you are decoding a live stream, you will need to use FILE_LENGTH_SECONDS = -1.

#Player Events
OpenPlayer is an event-based library. The events that will be fired in the decoding process are the following:

-  `READING_HEADER` - triggered when the library starts to read the OGG header of the stream
-  `TRACK_INFO` - while reading the OGG Header, the library will parse track information. This event is triggered when track information is available.
-  `READY_TO_PLAY` - called after the stream has been prepared. At this point, you can call `player.play()` in order to start playback
-  `PLAY_UPDATE` - use this trigger for play progress
-  `PLAYING_FAILED` - there has been a problem while decoding the stream.
-  `PLAYING_FINISHED` - player reached the end of the stream


#iOS Version
OpenPlayer has also been developed for iOS platform. You can find it here:



License
-------

	© 2014 AudioNow® Digital, LLC.

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at
    
        http://www.apache.org/licenses/LICENSE-2.0
    
    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.
