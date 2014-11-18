/*
 * OpusHeader.c
 * Process an Opus header and setup the opus decoder based on it. It takes several pointers for header values which are needed elsewhere in the code.
 *
 * (C) 2014 Radu Motisan, radu.motisan@gmail.com
 *
 * Part of the OpenPlayer implementation for Alpine Audio Now Digital LLC
 */

#include "OpusHeader.h"

/*
// testing a very dirty fix for the Android 5.0 crash of opus player:
// 11-18 19:10:18.114: E/AndroidRuntime(3555): FATAL EXCEPTION: main
// 11-18 19:10:18.114: E/AndroidRuntime(3555): Process: com.audionowdigital.android.openplayerdemo, PID: 3555
// 11-18 19:10:18.114: E/AndroidRuntime(3555): java.lang.UnsatisfiedLinkError: dlopen failed: cannot locate symbol "opus_header_parse" referenced by "libopus-jni.so"...
// 11-18 19:10:18.114: E/AndroidRuntime(3555): 	at java.lang.Runtime.loadLibrary(Runtime.java:371)
// for some reason the opus_header.c code doesn't get included, so here it is...

edit: added to Android.mk

/* Header contents:
  - "OpusHead" (64 bits)
  - version number (8 bits)
  - Channels C (8 bits)
  - Pre-skip (16 bits)
  - Sampling rate (32 bits)
  - Gain in dB (16 bits, S7.8)
  - Mapping (8 bits, 0=single stream (mono/stereo) 1=Vorbis mapping,
             2..254: reserved, 255: multistream with no mapping)

  - if (mapping != 0)
     - N = totel number of streams (8 bits)
     - M = number of paired streams (8 bits)
     - C times channel origin
          - if (C<2*M)
             - stream = byte/2
             - if (byte&0x1 == 0)
                 - left
               else
                 - right
          - else
             - stream = byte-M
*/
/*
typedef struct {
   unsigned char *data;
   int maxlen;
   int pos;
} Packet;

typedef struct {
   const unsigned char *data;
   int maxlen;
   int pos;
} ROPacket;

static int write_uint32(Packet *p, ogg_uint32_t val)
{
   if (p->pos>p->maxlen-4)
      return 0;
   p->data[p->pos  ] = (val    ) & 0xFF;
   p->data[p->pos+1] = (val>> 8) & 0xFF;
   p->data[p->pos+2] = (val>>16) & 0xFF;
   p->data[p->pos+3] = (val>>24) & 0xFF;
   p->pos += 4;
   return 1;
}

static int write_uint16(Packet *p, ogg_uint16_t val)
{
   if (p->pos>p->maxlen-2)
      return 0;
   p->data[p->pos  ] = (val    ) & 0xFF;
   p->data[p->pos+1] = (val>> 8) & 0xFF;
   p->pos += 2;
   return 1;
}

static int write_chars(Packet *p, const unsigned char *str, int nb_chars)
{
   int i;
   if (p->pos>p->maxlen-nb_chars)
      return 0;
   for (i=0;i<nb_chars;i++)
      p->data[p->pos++] = str[i];
   return 1;
}

static int read_uint32(ROPacket *p, ogg_uint32_t *val)
{
   if (p->pos>p->maxlen-4)
      return 0;
   *val =  (ogg_uint32_t)p->data[p->pos  ];
   *val |= (ogg_uint32_t)p->data[p->pos+1]<< 8;
   *val |= (ogg_uint32_t)p->data[p->pos+2]<<16;
   *val |= (ogg_uint32_t)p->data[p->pos+3]<<24;
   p->pos += 4;
   return 1;
}

static int read_uint16(ROPacket *p, ogg_uint16_t *val)
{
   if (p->pos>p->maxlen-2)
      return 0;
   *val =  (ogg_uint16_t)p->data[p->pos  ];
   *val |= (ogg_uint16_t)p->data[p->pos+1]<<8;
   p->pos += 2;
   return 1;
}

static int read_chars(ROPacket *p, unsigned char *str, int nb_chars)
{
   int i;
   if (p->pos>p->maxlen-nb_chars)
      return 0;
   for (i=0;i<nb_chars;i++)
      str[i] = p->data[p->pos++];
   return 1;
}

int opus_header_parse(const unsigned char *packet, int len, OpusHeader *h) {
   int i;
   char str[9];
   ROPacket p;
   unsigned char ch;
   ogg_uint16_t shortval;

   p.data = packet;
   p.maxlen = len;
   p.pos = 0;
   str[8] = 0;
   if (len<19)return 0;
   read_chars(&p, (unsigned char*)str, 8);
   if (memcmp(str, "OpusHead", 8)!=0)
      return 0;

   if (!read_chars(&p, &ch, 1))
      return 0;
   h->version = ch;
   if((h->version&240) != 0) // Only major version 0 supported. 
      return 0;

   if (!read_chars(&p, &ch, 1))
      return 0;
   h->channels = ch;
   if (h->channels == 0)
      return 0;

   //LOGD("ver:%d channels:%d", h->version, h->channels);

   if (!read_uint16(&p, &shortval))
      return 0;
   h->preskip = shortval;

   if (!read_uint32(&p, &h->input_sample_rate))
      return 0;
   //LOGD("ver:%d channels:%d sample:%d", h->version, h->channels,h->input_sample_rate);

   if (!read_uint16(&p, &shortval))
      return 0;
   h->gain = (short)shortval;

   if (!read_chars(&p, &ch, 1))
      return 0;
   h->channel_mapping = ch;

   if (h->channel_mapping != 0)
   {
      if (!read_chars(&p, &ch, 1))
         return 0;

      if (ch<1)
         return 0;
      h->nb_streams = ch;

      if (!read_chars(&p, &ch, 1))
         return 0;

      if (ch>h->nb_streams || (ch+h->nb_streams)>255)
         return 0;
      h->nb_coupled = ch;

      // Multi-stream support 
      for (i=0;i<h->channels;i++)
      {
         if (!read_chars(&p, &h->stream_map[i], 1))
            return 0;
         if (h->stream_map[i]>(h->nb_streams+h->nb_coupled) && h->stream_map[i]!=255)
            return 0;
      }
   } else {
      if(h->channels>2)
         return 0;
      h->nb_streams = 1;
      h->nb_coupled = h->channels>1;
      h->stream_map[0]=0;
      h->stream_map[1]=1;
   }
   // For version 0/1 we know there won't be any more data so reject any that have data past the end.
   if ((h->version==0 || h->version==1) && p.pos != len)
      return 0;
   return 1;
}

int opus_header_to_packet(const OpusHeader *h, unsigned char *packet, int len)
{
   int i;
   Packet p;
   unsigned char ch;

   p.data = packet;
   p.maxlen = len;
   p.pos = 0;
   if (len<19)return 0;
   if (!write_chars(&p, (const unsigned char*)"OpusHead", 8))
      return 0;
   // Version is 1 
   ch = 1;
   if (!write_chars(&p, &ch, 1))
      return 0;

   ch = h->channels;
   if (!write_chars(&p, &ch, 1))
      return 0;

   if (!write_uint16(&p, h->preskip))
      return 0;

   if (!write_uint32(&p, h->input_sample_rate))
      return 0;

   if (!write_uint16(&p, h->gain))
      return 0;

   ch = h->channel_mapping;
   if (!write_chars(&p, &ch, 1))
      return 0;

   if (h->channel_mapping != 0)
   {
      ch = h->nb_streams;
      if (!write_chars(&p, &ch, 1))
         return 0;

      ch = h->nb_coupled;
      if (!write_chars(&p, &ch, 1))
         return 0;

      // Multi-stream support 
      for (i=0;i<h->channels;i++)
      {
         if (!write_chars(&p, &h->stream_map[i], 1))
            return 0;
      }
   }

   return p.pos;
}

// This is just here because it's a convenient file linked by both opusenc and opusdec (to guarantee this maps stays in sync). 
const int wav_permute_matrix[8][8] =
{
  {0},              // 1.0 mono   
  {0,1},            // 2.0 stereo 
  {0,2,1},          // 3.0 channel ('wide') stereo 
  {0,1,2,3},        // 4.0 discrete quadraphonic 
  {0,2,1,3,4},      // 5.0 surround 
  {0,2,1,4,5,3},    // 5.1 surround 
  {0,2,1,5,6,4,3},  // 6.1 surround 
  {0,2,1,6,7,4,5,3} // 7.1 surround (classic theater 8-track) 
};
*/

 OpusDecoder *process_header(ogg_packet *op, int *rate, int *channels, int *preskip, int quiet) {
	int err;
	OpusDecoder *st;
	OpusHeader header;

	if (opus_header_parse(op->packet, op->bytes, &header) == 0) {
		LOGE(LOG_TAG, "Cannot parse header");
		return NULL;
	} else
		LOGD(LOG_TAG, "Header parsed: ch:%d samplerate:%d", header.channels, header.input_sample_rate);
	*channels = header.channels;

	// validate sample rate: If the rate is unspecified we decode to 48000
	if (!*rate) *rate = header.input_sample_rate;
	if (*rate == 0) *rate = 48000;
	if (*rate < 8000 || *rate > 192000) {
		LOGE(LOG_TAG, "Invalid input_rate %d, defaulting to 48000 instead.",*rate);
		*rate = 48000;
	}

	*preskip = header.preskip;
	st = opus_decoder_create(*rate, header.channels, &err); // was 48000

	//st->bandwidth;

	//LOGE(LOG_TAG, "ST: bandwidth:%d framesize:%d",st->bandwidth,st->frame_size);



	if (err != OPUS_OK)	{
		LOGE(LOG_TAG, "Cannot create decoder: %s", opus_strerror(err));
		return NULL;
	}
	if (!st) {
		LOGE(LOG_TAG, "Decoder initialization failed: %s", opus_strerror(err));
		return NULL;
	}

	if (header.gain != 0) {
		/*Gain API added in a newer libopus version, if we don't have it
		 we apply the gain ourselves. We also add in a user provided
		 manual gain at the same time.*/
		int gainadj = (int) header.gain;
		err = opus_decoder_ctl(st, OPUS_SET_GAIN(gainadj));
		if (err != OPUS_OK) {
		  LOGE(LOG_TAG, "Error setting gain: %s", opus_strerror(err));
		  return NULL;
		}
	}

	if (!quiet) {
		LOGE(LOG_TAG, "Decoding to %d Hz (%d channels)", *rate, *channels);
		if (header.version != 1) LOGE(LOG_TAG, "Header v%d",header.version);

		if (header.gain != 0) {
			LOGE(LOG_TAG, "Playback gain: %f dB\n", header.gain / 256.);
		}
	}

	return st;
}



// OpusTags | Len 4B | Vendor String (len) | Len 4B Tags | Len Tag 1 4B | Tag 1 String (Len) | Len Tag 2 4B ..
int process_comments(char *c, int length, char *vendor, char *title,  char *artist, char *album, char *date, char *track, int maxlen) {
	int err = SUCCESS;
	LOGE(LOG_TAG,"process_comments called for %d bytes.", length);

	if (length < (8 + 4 + 4)) {
		err = INVALID_HEADER;
		return err;;
	}
	if (strncmp(c, "OpusTags", 8) != 0) {
		err = INVALID_HEADER;
		return err;
	}
	c += 8; // skip header
	int len = readint(c, 0);
	c += 4;
	if (len < 0 || len > (length - 16)) {
		LOGE(LOG_TAG, "invalid/corrupt comments");
		err = INVALID_HEADER;
		return err;
	}
	strncpy(vendor, c, min(len, maxlen));

	c += len;
	int fields = readint(c, 0); // the -16 check above makes sure we can read this.
	c += 4;
	length -= 16 + len;
	if (fields < 0 || fields > (length >> 2)) {
		LOGE(LOG_TAG, "invalid/corrupt comments");
		err = INVALID_HEADER;
		return err;
	}
	LOGD(LOG_TAG, "Go and read %d fields:", fields);
	int i = 0;
	for (i = 0; i < fields; i++) {
	    if (length < 4){
	    	LOGE(LOG_TAG, "invalid/corrupt comments");
			err = INVALID_HEADER;
			return err;
	    }
	    len = readint(c, 0);
	    c += 4;
	    length -= 4;
	    if (len < 0 || len > length)
	    {
	    	LOGE(LOG_TAG, "invalid/corrupt comments");
			err = INVALID_HEADER;
			return err;
	    }
	    char *tmp = (char *)malloc(len + 1); // we also need the ending 0
	    strncpy(tmp, c, len);
	    tmp[len] = 0;
	    LOGD(LOG_TAG,"Header comment:%d len:%d [%s]", i, len, tmp);
	    free(tmp);

	    // keys we are looking for in the comments
		char keys[5][10] = { "title=", "artist=", "album=", "date=", "track=" };
		char *values[5] = { title, artist, album, date, track }; // put the values in these pointers
	    int j = 0;
	    for (j = 0; j < 5; j++) { // iterate all keys
	    	int keylen = strlen(keys[j]);
	    	if (!strncasecmp(c, keys[j], keylen )) strncpy(values[j], c + keylen, min(len - keylen , maxlen));
	    }
	    /*if (!strncasecmp(c, "title=", 6)) strncpy(title, c + 6, min(len - 6 , maxlen));
	    if (!strncasecmp(c, "artist=", 7)) strncpy(artist, c + 7, min(len - 7 , maxlen));
	    if (!strncasecmp(c, "album=", 6)) strncpy(album, c + 6, min(len - 6 , maxlen));
	    if (!strncasecmp(c, "date=", 5)) strncpy(date, c + 5, min(len - 5 , maxlen));
	    if (!strncasecmp(c, "track=", 6)) strncpy(track, c + 6, min(len - 6 , maxlen));*/

	    c += len;
	    length -= len;
	  }
}
