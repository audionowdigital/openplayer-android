# libvorbis - Vorbis Encoder/Decoder example for Android/JNI
libvorbis - Vorbis Encoder/Decoder example for Android/JNI.

## Purpose
* The purpose of this project is to demonstrate how Android can use a JNI wrapper around the libvorbis library to both encode PCM data, and decode a ogg/vorbis bitstream

## Usage
* Press the "Start Recording" button to start recording, then press the "Stop Recording" button to stop recording.
* Recorded content is saved saveTo.ogg on the external storage location.
* Press the "Start Playing" button to start playing the recorded data, then press the "Stop Playing" button to stop playing or simply wait for the playback to finish on its own.
* Content is played form saveTo.ogg on the external storage location

## To Build
* Verify the ndk-build tool is in your path and simply execute ./build_jni.sh from your terminal

## Library Usage
* Encoder
 * To record to a file

<pre>
VorbisRecorder vorbisRecorder = new VorbisRecorder(fileToSaveTo, recordHandler);

//Start recording with a sample rate of 44KHz, 2 channels (stereo), at 0.2 quality
vorbisRecorder.start(44100, 2, 0.2f);

//or
//Start recording with a sample rate of 44KHz, 2 channels (stereo), at 128 bitrate
vorbisRecorder.start(44100, 2, 128000);
</pre>

 * To read from other input, create a custom ```EncodeFeed```
<pre>
EncodeFeed encodeFeed = new EncodeFeed() {
             @Override
             public long readPCMData(byte[] pcmDataBuffer, int amountToWrite) {
                 //Read data from pcm data source
             }

             @Override
             public int writeVorbisData(byte[] vorbisData, int amountToRead) {
                 //write encoded data to where ever
             }

             @Override
             public void stop() {
                 //The native encoder has finished
             }

             @Override
             public void stopEncoding() {
                 //The encoder should wrap up until the native encoder calls stop()
             }

             @Override
             public void start() {
                 //The native encoder has started
             }
         };
VorbisRecorder vorbisRecorder = new VorbisRecorder(encodeFeed, recordHandler);
vorbisRecorder.start(...);
</pre>

* Decoder
 * Decode from file
<pre>
VorbisPlayer vorbisPlayer = new VorbisPlayer(fileToPlay, playerHandler);
vorbisPlayer.start();
</pre>  

 * To write to custom output, create a custom ```DecodeFeed```
<pre>
DecodeFeed decodeFeed = new DecodeFeed() {
             @Override
             public int readVorbisData(byte[] buffer, int amountToWrite) {
                 //Read from vorbis data source
             }

             @Override
             public void writePCMData(short[] pcmData, int amountToRead) {
                 //Write encoded pcm data
             }

             @Override
             public void stop() {
                 //Stop called from the native decoder
             }

             @Override
             public void startReadingHeader() {
                 //Called from the native decoder to read header information first
             }

             @Override
             public void start(DecodeStreamInfo decodeStreamInfo) {
                 //Called from the native decoder that we're ready and have processed the header information
             }
         };
VorbisPlayer vorbisPlayer = new VorbisPlayer(decodeFeed);
...
</pre>

## License
* For simplicity sake, this code is licensed under the same license as the libvorbis library from (http://xiph.org/vorbis/)

## Others
* Tested and working under the Nexus 4, feedback for other devices are welcome
* Pull requests welcome
