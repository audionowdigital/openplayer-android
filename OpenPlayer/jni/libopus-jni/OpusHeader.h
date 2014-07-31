#include <jni.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <ogg/ogg.h>
#include <opus.h>
#include <opus_header.h>
#include <android/log.h>
#include "../decodefeed/Log.h"


/*Define message codes*/
#define INVALID_HEADER -1
#define DECODE_ERROR -2
#define SUCCESS 0

// read an int from multiple bytes
#define readint(buf, offset) (((buf[offset + 3] << 24) & 0xff000000) | ((buf[offset + 2] << 16) & 0xff0000) | ((buf[offset + 1] << 8) & 0xff00) | (buf[offset] & 0xff))
#define max(a,b) ({ __typeof__ (a) _a = (a); __typeof__ (b) _b = (b); _a > _b ? _a : _b; })
#define min(a,b) ({ __typeof__ (a) _a = (a); __typeof__ (b) _b = (b); _a < _b ? _a : _b; })


 OpusDecoder *process_header(ogg_packet *op, int *rate, int *channels, int *preskip, int quiet);
int process_comments(char *c, int length, char *vendor, char *title,  char *artist, char *album, char *date, char *track, int maxlen);
