// Test all DLNA video profiles

#include <libmediascan.h>

#include <unistd.h>
#include <stdint.h>
#include <math.h>

#include "../src/mediascan.h"
#include "../src/common.h"

#include "tap.h"
#include "common.h"

#define TEST_COUNT 1426

#ifndef FALSE
#define FALSE 0
#endif

#ifndef TRUE
#define TRUE 1
#endif

#ifdef _MSC_VER

int strcasecmp(const char *string1, const char *string2 )
{
	return _stricmp(string1, string2);
}


int strncasecmp(const char *string1, const char *string2, size_t count )
{
	return _strnicmp(string1, string2, count);
}

///-------------------------------------------------------------------------------------------------
///  Inline assembly version of float rounding function
///
/// @author Henry Bennett
/// @date 03/24/2011
///
/// @param x Float to round
///
/// @return .
///-------------------------------------------------------------------------------------------------

int __inline lrintf(const float x)
{
  __asm cvtss2si eax, x
} /* lrintf() */

#endif

// From ffmpeg utils.c:print_fps
static const char *
fps2str(double fps)
{
  static char str[10];
  uint64_t v = lrint(fps*100);
  if     (v% 100      ) sprintf(str, "%3.2f", fps);
  else if(v%(100*1000)) sprintf(str, "%1.0f", fps);
  else                  sprintf(str, "%1.0fk", fps/1000);
  return str;
}


static void my_result_callback(MediaScan *s, MediaScanResult *r, void *userdata) {
  char *file;
  if ((file = strrchr(r->path, '/')) == NULL)
    if ((file = strrchr(r->path, '\\')) == NULL)
      return;
  file++;

  //ms_dump_result(r);

  /// MPEG1

  {
    if (!strcmp(file, "MPEG1.mpg")) {
      ok(r->type == TYPE_VIDEO, "MPEG1.mpg type is video ok");
      is(r->mime_type, "video/mpeg", "MPEG1.mpg MIME type video/mpeg ok");
      is(r->dlna_profile, "MPEG1", "MPEG1.mpg DLNA profile MPEG1 ok");
      ok(r->size == 51200, "MPEG1.mpg file size is 51200 ok");
      ok(r->bitrate == 1363969, "MPEG1.mpg bitrate is 1363969 ok");
      ok(r->duration_ms == 300, "MPEG1.mpg duration is 0.3s ok");
      is(r->video->codec, "mpeg1video", "MPEG1.mpg codec mpeg1video ok");
      ok(r->video->width == 352, "MPEG1.mpg video width 352 ok");
      ok(r->video->height == 240, "MPEG1.mpg video height 240 ok");
      is(fps2str(r->video->fps), "29.97", "MPEG1.mpg framerate 29.97 ok");
      is(r->audio->codec, "mp2", "MPEG1.mpg audio codec mp2 ok");
      ok(r->audio->bitrate == 224000, "MPEG1.mpg audio bitrate 224000 ok");
      ok(r->audio->samplerate == 44100, "MPEG1.mpg audio samplerate 44100 ok");
      ok(r->audio->channels == 2, "MPEG1.mpg audio channels 2 ok");
    }
  }

  /// MPEG2

  // MPEG_PS_NTSC with LPCM audio
  {
    if (!strcmp(file, "MPEG_PS_NTSC-lpcm.mpg")) {
      ok(r->type == TYPE_VIDEO, "MPEG_PS_NTSC-lpcm.mpg type is video ok");
      is(r->mime_type, "video/mpeg", "MPEG_PS_NTSC-lpcm.mpg MIME type video/mpeg ok");
      is(r->dlna_profile, "MPEG_PS_NTSC", "MPEG_PS_NTSC-lpcm.mpg DLNA profile ok");
      is(r->video->codec, "mpeg2video", "MPEG_PS_NTSC-lpcm.mpg codec mpeg2video ok");
      ok(r->video->width == 720, "MPEG_PS_NTSC-lpcm.mpg video width 720 ok");
      ok(r->video->height == 480, "MPEG_PS_NTSC-lpcm.mpg video height 480 ok");
      is(fps2str(r->video->fps), "29.97", "MPEG_PS_NTSC-lpcm.mpg framerate 29.97 ok");
      is(r->audio->codec, "pcm_s16be", "MPEG_PS_NTSC-lpcm.mpg audio codec pcm_s16be ok");
      ok(r->audio->bitrate == 1536000, "MPEG_PS_NTSC-lpcm.mpg audio bitrate 1536000 ok");
      ok(r->audio->samplerate == 48000, "MPEG_PS_NTSC-lpcm.mpg audio samplerate 48000 ok");
      ok(r->audio->channels == 2, "MPEG_PS_NTSC-lpcm.mpg audio channels 2 ok");
    }
  }

  // MPEG_PS_NTSC with AC3 audio
  {
    if (!strcmp(file, "MPEG_PS_NTSC-ac3.mpg")) {
      ok(r->type == TYPE_VIDEO, "MPEG_PS_NTSC-ac3.mpg type is video ok");
      is(r->mime_type, "video/mpeg", "MPEG_PS_NTSC-ac3.mpg MIME type video/mpeg ok");
      is(r->dlna_profile, "MPEG_PS_NTSC", "MPEG_PS_NTSC-ac3.mpg DLNA profile ok");
      ok(r->video->width == 720, "MPEG_PS_NTSC-ac3.mpg video width 720 ok");
      ok(r->video->height == 480, "MPEG_PS_NTSC-ac3.mpg video height 480 ok");
      is(fps2str(r->video->fps), "29.97", "MPEG_PS_NTSC-ac3.mpg framerate 29.97 ok");
      is(r->audio->codec, "ac3", "MPEG_PS_NTSC-lpcm.mpg audio codec pcm_s16be ok");
      ok(r->audio->bitrate == 224000, "MPEG_PS_NTSC-ac3.mpg audio bitrate 224000 ok");
      ok(r->audio->samplerate == 48000, "MPEG_PS_NTSC-ac3.mpg audio samplerate 48000 ok");
      ok(r->audio->channels == 2, "MPEG_PS_NTSC-ac3.mpg audio channels 2 ok");
    }
  }

  // MPEG_PS_PAL with AC3 audio
  {
    if (!strcmp(file, "MPEG_PS_PAL-ac3.mpg")) {
      ok(r->type == TYPE_VIDEO, "MPEG_PS_PAL-ac3.mpg type is video ok");
      is(r->mime_type, "video/mpeg", "MPEG_PS_PAL-ac3.mpg MIME type video/mpeg ok");
      is(r->dlna_profile, "MPEG_PS_PAL", "MPEG_PS_PAL-ac3.mpg DLNA profile ok");
      ok(r->video->width == 720, "MPEG_PS_PAL-ac3.mpg video width 720 ok");
      ok(r->video->height == 576, "MPEG_PS_PAL-ac3.mpg video height 480 ok");
      is(fps2str(r->video->fps), "25", "MPEG_PS_PAL-ac3.mpg framerate 25 ok");
    }
  }

  // MPEG_TS_SD_NA_ISO
  {
    if (!strcmp(file, "MPEG_TS_SD_NA_ISO.ts")) {
      ok(r->type == TYPE_VIDEO, "MPEG_TS_SD_NA_ISO.ts type is video ok");
      is(r->mime_type, "video/mpeg", "MPEG_TS_SD_NA_ISO.ts MIME type video/mpeg ok");
      is(r->dlna_profile, "MPEG_TS_SD_NA_ISO", "MPEG_TS_SD_NA_ISO.ts DLNA profile ok");
      ok(r->video->width == 544, "MPEG_TS_SD_NA_ISO.ts video width 720 ok");
      ok(r->video->height == 480, "MPEG_TS_SD_NA_ISO.ts video height 480 ok");
      is(fps2str(r->video->fps), "29.97", "MPEG_TS_SD_NA_ISO.ts framerate 29.97 ok");
      is(r->audio->codec, "ac3", "MPEG_TS_SD_NA_ISO.ts audio codec pcm_s16be ok");
      ok(r->audio->bitrate == 192000, "MPEG_TS_SD_NA_ISO.ts audio bitrate 192000 ok");
      ok(r->audio->samplerate == 48000, "MPEG_TS_SD_NA_ISO.ts audio samplerate 48000 ok");
      ok(r->audio->channels == 2, "MPEG_TS_SD_NA_ISO.ts audio channels 2 ok");
    }
  }

  /**
   * libdlna supports:
   * Andy
   * MPEG_PS_NTSC
   * MPEG_TS_SD_NA_ISO

   * Henry
   * MPEG_TS_HD_NA

   * MPEG_PS_NTSC_XAC3
   * MPEG_PS_PAL
   * MPEG_PS_PAL_XAC3
   * MPEG_TS_MP_LL_AAC
   * MPEG_TS_MP_LL_AAC_T
   * MPEG_TS_MP_LL_AAC_ISO
   * MPEG_TS_SD_EU
   * MPEG_TS_SD_EU_T
   * MPEG_TS_SD_EU_ISO
   * MPEG_TS_SD_NA
   * MPEG_TS_SD_NA_T
   * MPEG_TS_SD_NA_XAC3
   * MPEG_TS_SD_NA_XAC3_T
   * MPEG_TS_SD_NA_XAC3_ISO
   * MPEG_TS_HD_NA_T
   * MPEG_TS_HD_NA_ISO
   * MPEG_TS_HD_NA_XAC3
   * MPEG_TS_HD_NA_XAC3_T
   * MPEG_TS_HD_NA_XAC3_ISO
   * MPEG_ES_PAL
   * MPEG_ES_NTSC
   * MPEG_ES_PAL_XAC3
   * MPEG_ES_NTSC_XAC3
   *
   * Additional profiles not supported:
   *
   * DIRECTV_TS_SD
   * MPEG_PS_SD_DTS
   * MPEG_PS_HD_DTS
   * MPEG_PS_HD_DTSHD
   * MPEG_PS_HD_DTSHD_HRA
   * MPEG_PS_HD_DTSHD_MA
   * MPEG_TS_DTS_ISO
   * MPEG_TS_DTS_T
   * MPEG_TS_DTSHD_HRA_ISO
   * MPEG_TS_DTSHD_HRA_T
   * MPEG_TS_DTSHD_MA_ISO
   * MPEG_TS_DTSHD_MA_T
   * MPEG_TS_HD_50_L2_ISO
   * MPEG_TS_HD_50_L2_T
   * MPEG_TS_HD_X_50_L2_T
   * MPEG_TS_HD_X_50_L2_ISO
   * MPEG_TS_HD_60_L2_ISO
   * MPEG_TS_HD_60_L2_T
   * MPEG_TS_HD_X_60_L2_T
   * MPEG_TS_HD_X_60_L2_ISO
   * MPEG_TS_HD_NA_MPEG1_L2_ISO
   * MPEG_TS_HD_NA_MPEG1_L2_T
   * MPEG_TS_JP_T
   * MPEG_TS_SD_50_AC3_T
   * MPEG_TS_SD_50_L2_T
   * MPEG_TS_SD_60_AC3_T
   * MPEG_TS_SD_60_L2_T
   * MPEG_TS_SD_EU_AC3_ISO
   * MPEG_TS_SD_EU_AC3_T
   * MPEG_TS_SD_JP_MPEG1_L2_T
   * MPEG_TS_SD_NA_MPEG1_L2_ISO
   * MPEG_TS_SD_NA_MPEG1_L2_T
   */
}

static void my_error_callback(MediaScan *s, MediaScanError *error, void *userdata) {
  LOG_ERROR("[Error] %s (%s)\n", error->error_string, error->path);
}

static int result_called = FALSE;
static MediaScanResult result;

static void my_result_callback2(MediaScan *s, MediaScanResult *r, void *userdata) {

	result.type = r->type;
	result.path = strdup(r->path);
	result.flags = r->flags;

	if(r->error)
		memcpy(result.error, r->error, sizeof(MediaScanError));

	result.mime_type = strdup(r->mime_type);
	result.dlna_profile = strdup(r->dlna_profile);
	result.size = r->size;
	result.mtime = r->mtime;
	result.bitrate = r->bitrate;
	result.duration_ms = r->duration_ms;

	if(r->audio)
	{
		result.audio = malloc(sizeof(MediaScanAudio));
		memcpy( result.audio, r->audio, sizeof(MediaScanAudio));
	}

	if(r->video)
	{
		result.video = malloc(sizeof(MediaScanVideo));
		memcpy( result.video, r->video, sizeof(MediaScanVideo));
	}

	if(r->image)
	{
		result.image = malloc(sizeof(MediaScanImage));
		memcpy( result.image, r->image, sizeof(MediaScanImage));
	}

	result_called = TRUE;
}

 typedef struct ExpectedResult {

  enum media_type type;
  char mime_type[64];
  char dlna_profile[64];
  int bitrate;
  int duration_ms;

  char audio_codec[64];
  int audio_bitrate;
  int audio_vbr;
  int audio_samplerate;
  int audio_channels;

  char video_codec[64];
  int video_width;
  int video_height;
  char video_fps[12];
} ExpectedResultType;

static void test_DLNA_files(char *file, ExpectedResultType *expected)
{
	MediaScan *s = NULL;
	char full_file[MAX_PATH_STR_LEN];

	#ifdef WIN32
	strcpy(full_file, "data\\video\\dlna_individual\\");
	#else
	strcpy(full_file, "data/video/dlna_individual/");
	#endif
	strcat(full_file, file);

	s = ms_create();
    ms_set_result_callback(s, my_result_callback2);
    ms_set_error_callback(s, my_error_callback);

	// Scan the file
	result_called = FALSE;
	memset( &result, 0, sizeof(MediaScanResult));
	ms_scan_file(s, full_file, TYPE_UNKNOWN);

	// Check the result
	ok(result_called == TRUE, "%s scanned", file);

	// Video part of the check
	ok(result.type == expected->type, "type is video ok");
	is(result.mime_type, expected->mime_type, "MIME type %s", expected->mime_type);
  is(result.dlna_profile, expected->dlna_profile, "DLNA profile ok");

	if(result.video != NULL) {
	ok(result.video->width == expected->video_width, "video width %d ok", expected->video_width);
	ok(result.video->height == expected->video_height, "video height %d ok", expected->video_height);
  is(fps2str(result.video->fps), expected->video_fps, "framerate %s ok", expected->video_fps);
	}

	if(result.audio != NULL) {
	// Audio part of the check
	is(result.audio->codec, expected->audio_codec, "audio codec %s ok", expected->audio_codec);
	ok(result.audio->bitrate == expected->audio_bitrate, "audio bitrate %d ok", expected->audio_bitrate);
	ok(result.audio->samplerate == expected->audio_samplerate, "audio samplerate %d ok", expected->audio_samplerate);
	ok(result.audio->channels == expected->audio_channels, "audio channels %d ok", expected->audio_channels);
	}

	printf("------------------------------------------------------\n");

    ms_destroy(s);
} /* test_DLNA_files() */

static void test_DLNA_audio_files(char *file, ExpectedResultType *expected)
{
	MediaScan *s = NULL;
	char full_file[MAX_PATH_STR_LEN];

	#ifdef WIN32
	strcpy(full_file, "data\\audio\\dlna_individual\\");
	#else
	strcpy(full_file, "data/audio/dlna_individual/");
	#endif
	strcat(full_file, file);

	s = ms_create();
    ms_set_result_callback(s, my_result_callback2);
    ms_set_error_callback(s, my_error_callback);

	// Scan the file
	result_called = FALSE;
	memset( &result, 0, sizeof(MediaScanResult));
	ms_scan_file(s, full_file, TYPE_UNKNOWN);

	// Check the result
	ok(result_called == TRUE, "%s scanned", file);

	// Video part of the check
	ok(result.type == expected->type, "type is audio ok");
	is(result.mime_type, expected->mime_type, "MIME type %s", expected->mime_type);
  is(result.dlna_profile, expected->dlna_profile, "DLNA profile ok");

	if(result.audio != NULL) {
	// Audio part of the check
	is(result.audio->codec, expected->audio_codec, "audio codec %s ok", expected->audio_codec);
	ok(result.audio->bitrate == expected->audio_bitrate, "audio bitrate %d ok", expected->audio_bitrate);
	ok(result.audio->samplerate == expected->audio_samplerate, "audio samplerate %d ok", expected->audio_samplerate);
	ok(result.audio->channels == expected->audio_channels, "audio channels %d ok", expected->audio_channels);
	}

	printf("------------------------------------------------------\n");

    ms_destroy(s);
} /* test_DLNA_audio_files() */

void test_MPEG_PS_NTSC()
{
    ExpectedResultType expected;

	memset( &expected, 0, sizeof(MediaScanResult) );
	expected.type = TYPE_VIDEO;
	strcpy(expected.mime_type, "video/mpeg");
	strcpy(expected.dlna_profile, "MPEG_PS_NTSC");
	expected.video_width = 720;
	expected.video_height = 480;
	strcpy(expected.video_fps, "29.97");

	// Audio part of the check
	strcpy(expected.audio_codec, "pcm_s16be");
	expected.audio_bitrate = 1536000;
	expected.audio_samplerate = 48000;
	expected.audio_channels = 2;

	test_DLNA_files("B-MP2PS_N-1.mpg", &expected);

	memset( &expected, 0, sizeof(MediaScanResult) );
	expected.type = TYPE_VIDEO;
	strcpy(expected.mime_type, "video/mpeg");
	strcpy(expected.dlna_profile, "MPEG_PS_NTSC");
	expected.video_width = 720;
	expected.video_height = 480;
	strcpy(expected.video_fps, "29.97");

	// Audio part of the check
	strcpy(expected.audio_codec, "ac3");
	expected.audio_bitrate = 224000;
	expected.audio_samplerate = 48000;
	expected.audio_channels = 2;

	test_DLNA_files("B-MP2PS_N-3.mpg", &expected);


	memset( &expected, 0, sizeof(MediaScanResult) );
	expected.type = TYPE_VIDEO;
	strcpy(expected.mime_type, "video/mpeg");
	strcpy(expected.dlna_profile, "MPEG_PS_NTSC");
	expected.video_width = 720;
	expected.video_height = 480;
	strcpy(expected.video_fps, "29.97");

	// Audio part of the check
	strcpy(expected.audio_codec, "mp2");
	expected.audio_bitrate = 224000;
	expected.audio_samplerate = 48000;
	expected.audio_channels = 2;

	test_DLNA_files("B-MP2PS_N-5.mpg", &expected);


	memset( &expected, 0, sizeof(MediaScanResult) );
	expected.type = TYPE_VIDEO;
	strcpy(expected.mime_type, "video/mpeg");
	strcpy(expected.dlna_profile, "MPEG_PS_NTSC");
	expected.video_width = 720;
	expected.video_height = 480;
	strcpy(expected.video_fps, "29.97");

	// Audio part of the check
	strcpy(expected.audio_codec, "mp2");
	expected.audio_bitrate = 256000;
	expected.audio_samplerate = 48000;
	expected.audio_channels = 2;

	test_DLNA_files("B-MP2PS_N-7.mpg", &expected);


	memset( &expected, 0, sizeof(MediaScanResult) );
	expected.type = TYPE_VIDEO;
	strcpy(expected.mime_type, "video/mpeg");
	strcpy(expected.dlna_profile, "MPEG_PS_NTSC");
	expected.video_width = 352;
	expected.video_height = 240;
	strcpy(expected.video_fps, "29.97");

	// Audio part of the check
	strcpy(expected.audio_codec, "mp2");
	expected.audio_bitrate = 64000;
	expected.audio_samplerate = 48000;
	expected.audio_channels = 1;

	test_DLNA_files("B-MP2PS_N-8.mpg", &expected);


	memset( &expected, 0, sizeof(MediaScanResult) );
	expected.type = TYPE_VIDEO;
	strcpy(expected.mime_type, "video/mpeg");
	strcpy(expected.dlna_profile, "MPEG_PS_NTSC");
	expected.video_width = 720;
	expected.video_height = 480;
	strcpy(expected.video_fps, "29.97");

	// Audio part of the check
	strcpy(expected.audio_codec, "mp2");
	expected.audio_bitrate = 256000;
	expected.audio_samplerate = 48000;
	expected.audio_channels = 2;

	test_DLNA_files("B-MP2PS_N-9.mpg", &expected);


	memset( &expected, 0, sizeof(MediaScanResult) );
	expected.type = TYPE_VIDEO;
	strcpy(expected.mime_type, "video/mpeg");
	strcpy(expected.dlna_profile, "MPEG_PS_NTSC");
	expected.video_width = 720;
	expected.video_height = 480;
	strcpy(expected.video_fps, "29.97");

	// Audio part of the check
	strcpy(expected.audio_codec, "mp2");
	expected.audio_bitrate = 224000;
	expected.audio_samplerate = 48000;
	expected.audio_channels = 2;

	test_DLNA_files("B-MP2PS_N-13.mpg", &expected);


	memset( &expected, 0, sizeof(MediaScanResult) );
	expected.type = TYPE_VIDEO;
	strcpy(expected.mime_type, "video/mpeg");
	strcpy(expected.dlna_profile, "MPEG_PS_NTSC");
	expected.video_width = 720;
	expected.video_height = 480;
	strcpy(expected.video_fps, "29.97");

	// Audio part of the check
	strcpy(expected.audio_codec, "mp2");
	expected.audio_bitrate = 224000;
	expected.audio_samplerate = 48000;
	expected.audio_channels = 2;

	test_DLNA_files("B-MP2PS_N-14.mpg", &expected);


	memset( &expected, 0, sizeof(MediaScanResult) );
	expected.type = TYPE_VIDEO;
	strcpy(expected.mime_type, "video/mpeg");
	strcpy(expected.dlna_profile, "MPEG_PS_NTSC");
	expected.video_width = 720;
	expected.video_height = 480;
	strcpy(expected.video_fps, "29.97");

	// Audio part of the check
	strcpy(expected.audio_codec, "mp2");
	expected.audio_bitrate = 1536000;
	expected.audio_samplerate = 48000;
	expected.audio_channels = 2;

	test_DLNA_files("B-MP2PS_N-61.mpg", &expected);

	memset( &expected, 0, sizeof(MediaScanResult) );
	expected.type = TYPE_VIDEO;
	strcpy(expected.mime_type, "video/mpeg");
	strcpy(expected.dlna_profile, "MPEG_PS_NTSC");
	expected.video_width = 544;
	expected.video_height = 480;
	strcpy(expected.video_fps, "29.97");

	// Audio part of the check
	strcpy(expected.audio_codec, "mp2");
	expected.audio_bitrate = 256000;
	expected.audio_samplerate = 48000;
	expected.audio_channels = 2;

	test_DLNA_files("B-MP2PS_N-67.mpg", &expected);


	memset( &expected, 0, sizeof(MediaScanResult) );
	expected.type = TYPE_VIDEO;
	strcpy(expected.mime_type, "video/mpeg");
	strcpy(expected.dlna_profile, "MPEG_PS_NTSC");
	expected.video_width = 720;
	expected.video_height = 480;
	strcpy(expected.video_fps, "29.97");

	// Audio part of the check
	strcpy(expected.audio_codec, "mp2");
	expected.audio_bitrate = 192000;
	expected.audio_samplerate = 48000;
	expected.audio_channels = 2;

	test_DLNA_files("B-MP2PS_N-68.mpg", &expected);


	memset( &expected, 0, sizeof(MediaScanResult) );
	expected.type = TYPE_VIDEO;
	strcpy(expected.mime_type, "video/mpeg");
	strcpy(expected.dlna_profile, "MPEG_PS_NTSC");
	expected.video_width = 720;
	expected.video_height = 480;
	strcpy(expected.video_fps, "29.97");

	// Audio part of the check
	strcpy(expected.audio_codec, "mp2");
	expected.audio_bitrate = 192000;
	expected.audio_samplerate = 48000;
	expected.audio_channels = 1;

	test_DLNA_files("B-MP2PS_N-69.mpg", &expected);


	memset( &expected, 0, sizeof(MediaScanResult) );
	expected.type = TYPE_VIDEO;
	strcpy(expected.mime_type, "video/mpeg");
	strcpy(expected.dlna_profile, "MPEG_PS_NTSC");
	expected.video_width = 720;
	expected.video_height = 480;
	strcpy(expected.video_fps, "29.97");

	// Audio part of the check
	strcpy(expected.audio_codec, "mp2");
	expected.audio_bitrate = 384000;
	expected.audio_samplerate = 48000;
	expected.audio_channels = 2;

	test_DLNA_files("B-MP2PS_N-70.mpg", &expected);


	memset( &expected, 0, sizeof(MediaScanResult) );
	expected.type = TYPE_VIDEO;
	strcpy(expected.mime_type, "video/mpeg");
	strcpy(expected.dlna_profile, "MPEG_PS_NTSC");
	expected.video_width = 352;
	expected.video_height = 240;
	strcpy(expected.video_fps, "29.97");

	// Audio part of the check
	strcpy(expected.audio_codec, "mp2");
	expected.audio_bitrate = 384000;
	expected.audio_samplerate = 48000;
	expected.audio_channels = 2;

	test_DLNA_files("B-MP2PS_N-80.mpg", &expected);


	memset( &expected, 0, sizeof(MediaScanResult) );
	expected.type = TYPE_VIDEO;
	strcpy(expected.mime_type, "video/mpeg");
	strcpy(expected.dlna_profile, "MPEG_PS_NTSC");
	expected.video_width = 720;
	expected.video_height = 480;
	strcpy(expected.video_fps, "29.97");

	// Audio part of the check
	strcpy(expected.audio_codec, "ac3");
	expected.audio_bitrate = 224000;
	expected.audio_samplerate = 48000;
	expected.audio_channels = 2;

	test_DLNA_files("B-MP2PS_N-86.mpg", &expected);


	memset( &expected, 0, sizeof(MediaScanResult) );
	expected.type = TYPE_VIDEO;
	strcpy(expected.mime_type, "video/mpeg");
	strcpy(expected.dlna_profile, "MPEG_PS_NTSC");
	expected.video_width = 720;
	expected.video_height = 480;
	strcpy(expected.video_fps, "29.97");

	// Audio part of the check
	strcpy(expected.audio_codec, "ac3");
	expected.audio_bitrate = 224000;
	expected.audio_samplerate = 48000;
	expected.audio_channels = 2;

	test_DLNA_files("B-MP2PS_N-87.mpg", &expected);


	memset( &expected, 0, sizeof(MediaScanResult) );
	expected.type = TYPE_VIDEO;
	strcpy(expected.mime_type, "video/mpeg");
	strcpy(expected.dlna_profile, "MPEG_PS_NTSC");
	expected.video_width = 480;
	expected.video_height = 480;
	strcpy(expected.video_fps, "29.97");

	// Audio part of the check
	strcpy(expected.audio_codec, "mp2");
	expected.audio_bitrate = 256000;
	expected.audio_samplerate = 48000;
	expected.audio_channels = 2;

	test_DLNA_files("B-MP2PS_N-107.mpg", &expected);

}	/* test_MPEG_PS_NTSC() */

void test_MPEG_TS_HD_NA()
{
    ExpectedResultType expected;

	// Test for MPEG_TS_HD_NA
	memset( &expected, 0, sizeof(MediaScanResult) );
	expected.type = TYPE_VIDEO;
	strcpy(expected.mime_type, "video/vnd.dlna.mpeg-tts");
	strcpy(expected.dlna_profile, "MPEG_TS_HD_NA");
	expected.video_width = 1920;
	expected.video_height = 1080;
	strcpy(expected.video_fps, "30");

	// Audio part of the check
	strcpy(expected.audio_codec, "ac3");
	expected.audio_bitrate = 192000;
	expected.audio_samplerate = 48000;
	expected.audio_channels = 2;

	test_DLNA_files("O-MP2TS_HN-1.mpg", &expected);

	memset( &expected, 0, sizeof(MediaScanResult) );
	expected.type = TYPE_VIDEO;
	strcpy(expected.mime_type, "video/vnd.dlna.mpeg-tts");
	strcpy(expected.dlna_profile, "MPEG_TS_HD_NA");
	expected.video_width = 1920;
	expected.video_height = 1080;
	strcpy(expected.video_fps, "30");

	// Audio part of the check
	strcpy(expected.audio_codec, "ac3");
	expected.audio_bitrate = 192000;
	expected.audio_samplerate = 48000;
	expected.audio_channels = 2;

	test_DLNA_files("O-MP2TS_HN-2.mpg", &expected);


	memset( &expected, 0, sizeof(MediaScanResult) );
	expected.type = TYPE_VIDEO;
	strcpy(expected.mime_type, "video/vnd.dlna.mpeg-tts");
	strcpy(expected.dlna_profile, "MPEG_TS_HD_NA");
	expected.video_width = 1920;
	expected.video_height = 1080;
	strcpy(expected.video_fps, "30");

	// Audio part of the check
	strcpy(expected.audio_codec, "ac3");
	expected.audio_bitrate = 192000;
	expected.audio_samplerate = 48000;
	expected.audio_channels = 2;

	test_DLNA_files("O-MP2TS_HN-3.mpg", &expected);

	memset( &expected, 0, sizeof(MediaScanResult) );
	expected.type = TYPE_VIDEO;
	strcpy(expected.mime_type, "video/vnd.dlna.mpeg-tts");
	strcpy(expected.dlna_profile, "MPEG_TS_HD_NA");
	expected.video_width = 1920;
	expected.video_height = 1080;
	strcpy(expected.video_fps, "30");

	// Audio part of the check
	strcpy(expected.audio_codec, "ac3");
	expected.audio_bitrate = 192000;
	expected.audio_samplerate = 48000;
	expected.audio_channels = 2;

	test_DLNA_files("O-MP2TS_HN-4.mpg", &expected);

	memset( &expected, 0, sizeof(MediaScanResult) );
	expected.type = TYPE_VIDEO;
	strcpy(expected.mime_type, "video/vnd.dlna.mpeg-tts");
	strcpy(expected.dlna_profile, "MPEG_TS_HD_NA");
	expected.video_width = 1920;
	expected.video_height = 1080;
	strcpy(expected.video_fps, "29.97");

	// Audio part of the check
	strcpy(expected.audio_codec, "ac3");
	expected.audio_bitrate = 128000;
	expected.audio_samplerate = 48000;
	expected.audio_channels = 1;

	test_DLNA_files("O-MP2TS_HN-5.mpg", &expected);

	memset( &expected, 0, sizeof(MediaScanResult) );
	expected.type = TYPE_VIDEO;
	strcpy(expected.mime_type, "video/vnd.dlna.mpeg-tts");
	strcpy(expected.dlna_profile, "MPEG_TS_HD_NA");
	expected.video_width = 1920;
	expected.video_height = 1080;
	strcpy(expected.video_fps, "24");

	// Audio part of the check
	strcpy(expected.audio_codec, "ac3");
	expected.audio_bitrate = 192000;
	expected.audio_samplerate = 48000;
	expected.audio_channels = 2;

	test_DLNA_files("O-MP2TS_HN-6.mpg", &expected);

	memset( &expected, 0, sizeof(MediaScanResult) );
	expected.type = TYPE_VIDEO;
	strcpy(expected.mime_type, "video/vnd.dlna.mpeg-tts");
	strcpy(expected.dlna_profile, "MPEG_TS_HD_NA");
	expected.video_width = 1920;
	expected.video_height = 1080;
	strcpy(expected.video_fps, "29.97");

	// Audio part of the check
	strcpy(expected.audio_codec, "ac3");
	expected.audio_bitrate = 192000;
	expected.audio_samplerate = 48000;
	expected.audio_channels = 2;

	test_DLNA_files("O-MP2TS_HN-7.mpg", &expected);

	memset( &expected, 0, sizeof(MediaScanResult) );
	expected.type = TYPE_VIDEO;
	strcpy(expected.mime_type, "video/vnd.dlna.mpeg-tts");
	strcpy(expected.dlna_profile, "MPEG_TS_HD_NA");
	expected.video_width = 1920;
	expected.video_height = 1080;
	strcpy(expected.video_fps, "23.98");

	// Audio part of the check
	strcpy(expected.audio_codec, "ac3");
	expected.audio_bitrate = 192000;
	expected.audio_samplerate = 48000;
	expected.audio_channels = 2;

	test_DLNA_files("O-MP2TS_HN-8.mpg", &expected);
}	/* test_MPEG_TS_HD_NA() */

void test_MPEG_TS_HD_NA_T()
{
    ExpectedResultType expected;

	// Test for MPEG_TS_HD_NA
	memset( &expected, 0, sizeof(MediaScanResult) );
	expected.type = TYPE_VIDEO;
	strcpy(expected.mime_type, "video/vnd.dlna.mpeg-tts");
	strcpy(expected.dlna_profile, "MPEG_TS_HD_NA_T");
	expected.video_width = 1920;
	expected.video_height = 1080;
	strcpy(expected.video_fps, "30");

	// Audio part of the check
	strcpy(expected.audio_codec, "ac3");
	expected.audio_bitrate = 192000;
	expected.audio_samplerate = 48000;
	expected.audio_channels = 2;

	test_DLNA_files("O-MP2TS_HN_T-1.mpg", &expected);

	memset( &expected, 0, sizeof(MediaScanResult) );
	expected.type = TYPE_VIDEO;
	strcpy(expected.mime_type, "video/vnd.dlna.mpeg-tts");
	strcpy(expected.dlna_profile, "MPEG_TS_HD_NA_T");
	expected.video_width = 1920;
	expected.video_height = 1080;
	strcpy(expected.video_fps, "30");

	// Audio part of the check
	strcpy(expected.audio_codec, "ac3");
	expected.audio_bitrate = 192000;
	expected.audio_samplerate = 48000;
	expected.audio_channels = 2;

	test_DLNA_files("O-MP2TS_HN_T-2.mpg", &expected);


	memset( &expected, 0, sizeof(MediaScanResult) );
	expected.type = TYPE_VIDEO;
	strcpy(expected.mime_type, "video/vnd.dlna.mpeg-tts");
	strcpy(expected.dlna_profile, "MPEG_TS_HD_NA_T");
	expected.video_width = 1920;
	expected.video_height = 1080;
	strcpy(expected.video_fps, "30");

	// Audio part of the check
	strcpy(expected.audio_codec, "ac3");
	expected.audio_bitrate = 192000;
	expected.audio_samplerate = 48000;
	expected.audio_channels = 2;

	test_DLNA_files("O-MP2TS_HN_T-3.mpg", &expected);

	memset( &expected, 0, sizeof(MediaScanResult) );
	expected.type = TYPE_VIDEO;
	strcpy(expected.mime_type, "video/vnd.dlna.mpeg-tts");
	strcpy(expected.dlna_profile, "MPEG_TS_HD_NA_T");
	expected.video_width = 1920;
	expected.video_height = 1080;
	strcpy(expected.video_fps, "30");

	// Audio part of the check
	strcpy(expected.audio_codec, "ac3");
	expected.audio_bitrate = 192000;
	expected.audio_samplerate = 48000;
	expected.audio_channels = 2;

	test_DLNA_files("O-MP2TS_HN_T-4.mpg", &expected);

	memset( &expected, 0, sizeof(MediaScanResult) );
	expected.type = TYPE_VIDEO;
	strcpy(expected.mime_type, "video/vnd.dlna.mpeg-tts");
	strcpy(expected.dlna_profile, "MPEG_TS_HD_NA_T");
	expected.video_width = 1920;
	expected.video_height = 1080;
	strcpy(expected.video_fps, "29.97");

	// Audio part of the check
	strcpy(expected.audio_codec, "ac3");
	expected.audio_bitrate = 128000;
	expected.audio_samplerate = 48000;
	expected.audio_channels = 1;

	test_DLNA_files("O-MP2TS_HN_T-5.mpg", &expected);

	memset( &expected, 0, sizeof(MediaScanResult) );
	expected.type = TYPE_VIDEO;
	strcpy(expected.mime_type, "video/vnd.dlna.mpeg-tts");
	strcpy(expected.dlna_profile, "MPEG_TS_HD_NA_T");
	expected.video_width = 1920;
	expected.video_height = 1080;
	strcpy(expected.video_fps, "24");

	// Audio part of the check
	strcpy(expected.audio_codec, "ac3");
	expected.audio_bitrate = 192000;
	expected.audio_samplerate = 48000;
	expected.audio_channels = 2;

	test_DLNA_files("O-MP2TS_HN_T-6.mpg", &expected);

	memset( &expected, 0, sizeof(MediaScanResult) );
	expected.type = TYPE_VIDEO;
	strcpy(expected.mime_type, "video/vnd.dlna.mpeg-tts");
	strcpy(expected.dlna_profile, "MPEG_TS_HD_NA_T");
	expected.video_width = 1920;
	expected.video_height = 1080;
	strcpy(expected.video_fps, "29.97");

	// Audio part of the check
	strcpy(expected.audio_codec, "ac3");
	expected.audio_bitrate = 192000;
	expected.audio_samplerate = 48000;
	expected.audio_channels = 2;

	test_DLNA_files("O-MP2TS_HN_T-7.mpg", &expected);

	memset( &expected, 0, sizeof(MediaScanResult) );
	expected.type = TYPE_VIDEO;
	strcpy(expected.mime_type, "video/vnd.dlna.mpeg-tts");
	strcpy(expected.dlna_profile, "MPEG_TS_HD_NA_T");
	expected.video_width = 1920;
	expected.video_height = 1080;
	strcpy(expected.video_fps, "23.98");

	// Audio part of the check
	strcpy(expected.audio_codec, "ac3");
	expected.audio_bitrate = 192000;
	expected.audio_samplerate = 48000;
	expected.audio_channels = 2;

	test_DLNA_files("O-MP2TS_HN_T-8.mpg", &expected);
}	/* test_MPEG_TS_HD_NA_T() */

void test_MPEG_TS_SD_NA()
{
    ExpectedResultType expected;

	// Test for MPEG_TS_HD_NA
	memset( &expected, 0, sizeof(MediaScanResult) );
	expected.type = TYPE_VIDEO;
	strcpy(expected.mime_type, "video/vnd.dlna.mpeg-tts");
	strcpy(expected.dlna_profile, "MPEG_TS_SD_NA");
	expected.video_width = 640;
	expected.video_height = 480;
	strcpy(expected.video_fps, "60");

	// Audio part of the check
	strcpy(expected.audio_codec, "ac3");
	expected.audio_bitrate = 192000;
	expected.audio_samplerate = 48000;
	expected.audio_channels = 2;

	test_DLNA_files("O-MP2TS_SN-1.mpg", &expected);

	memset( &expected, 0, sizeof(MediaScanResult) );
	expected.type = TYPE_VIDEO;
	strcpy(expected.mime_type, "video/vnd.dlna.mpeg-tts");
	strcpy(expected.dlna_profile, "MPEG_TS_SD_NA");
	expected.video_width = 640;
	expected.video_height = 480;
	strcpy(expected.video_fps, "59.96");

	// Audio part of the check
	strcpy(expected.audio_codec, "ac3");
	expected.audio_bitrate = 192000;
	expected.audio_samplerate = 48000;
	expected.audio_channels = 2;

	test_DLNA_files("O-MP2TS_SN-2.mpg", &expected);


	memset( &expected, 0, sizeof(MediaScanResult) );
	expected.type = TYPE_VIDEO;
	strcpy(expected.mime_type, "video/vnd.dlna.mpeg-tts");
	strcpy(expected.dlna_profile, "MPEG_TS_SD_NA");
	expected.video_width = 544;
	expected.video_height = 480;
	strcpy(expected.video_fps, "29.97");

	// Audio part of the check
	strcpy(expected.audio_codec, "ac3");
	expected.audio_bitrate = 192000;
	expected.audio_samplerate = 48000;
	expected.audio_channels = 2;

	test_DLNA_files("O-MP2TS_SN-3.mpg", &expected);

	memset( &expected, 0, sizeof(MediaScanResult) );
	expected.type = TYPE_VIDEO;
	strcpy(expected.mime_type, "video/vnd.dlna.mpeg-tts");
	strcpy(expected.dlna_profile, "MPEG_TS_SD_NA");
	expected.video_width = 480;
	expected.video_height = 480;
	strcpy(expected.video_fps, "29.97");

	// Audio part of the check
	strcpy(expected.audio_codec, "ac3");
	expected.audio_bitrate = 192000;
	expected.audio_samplerate = 48000;
	expected.audio_channels = 2;

	test_DLNA_files("O-MP2TS_SN-4.mpg", &expected);

	memset( &expected, 0, sizeof(MediaScanResult) );
	expected.type = TYPE_VIDEO;
	strcpy(expected.mime_type, "video/vnd.dlna.mpeg-tts");
	strcpy(expected.dlna_profile, "MPEG_TS_SD_NA");
	expected.video_width = 352;
	expected.video_height = 480;
	strcpy(expected.video_fps, "29.97");

	// Audio part of the check
	strcpy(expected.audio_codec, "ac3");
	expected.audio_bitrate = 192000;
	expected.audio_samplerate = 48000;
	expected.audio_channels = 2;

	test_DLNA_files("O-MP2TS_SN-5.mpg", &expected);
} /* test_MPEG_TS_SD_NA() */

void test_MPEG_TS_SD_NA_T()
{
    ExpectedResultType expected;

	// Test for MPEG_TS_HD_NA_T

	memset( &expected, 0, sizeof(MediaScanResult) );
	expected.type = TYPE_VIDEO;
	strcpy(expected.mime_type, "video/vnd.dlna.mpeg-tts");
	strcpy(expected.dlna_profile, "MPEG_TS_SD_NA_T");
	expected.video_width = 720;
	expected.video_height = 480;
	strcpy(expected.video_fps, "29.97");

	// Audio part of the check
	strcpy(expected.audio_codec, "ac3");
	expected.audio_bitrate = 192000;
	expected.audio_samplerate = 48000;
	expected.audio_channels = 2;

	test_DLNA_files("O-MP2TS_SNT-1.mpg", &expected);

	memset( &expected, 0, sizeof(MediaScanResult) );
	expected.type = TYPE_VIDEO;
	strcpy(expected.mime_type, "video/vnd.dlna.mpeg-tts");
	strcpy(expected.dlna_profile, "MPEG_TS_SD_NA_T");
	expected.video_width = 720;
	expected.video_height = 480;
	strcpy(expected.video_fps, "29.97");

	// Audio part of the check
	strcpy(expected.audio_codec, "ac3");
	expected.audio_bitrate = 192000;
	expected.audio_samplerate = 48000;
	expected.audio_channels = 2;

	test_DLNA_files("O-MP2TS_SNT-2.mpg", &expected);

	memset( &expected, 0, sizeof(MediaScanResult) );
	expected.type = TYPE_VIDEO;
	strcpy(expected.mime_type, "video/vnd.dlna.mpeg-tts");
	strcpy(expected.dlna_profile, "MPEG_TS_SD_NA_T");
	expected.video_width = 704;
	expected.video_height = 480;
	strcpy(expected.video_fps, "60");

	// Audio part of the check
	strcpy(expected.audio_codec, "ac3");
	expected.audio_bitrate = 192000;
	expected.audio_samplerate = 48000;
	expected.audio_channels = 2;

	test_DLNA_files("O-MP2TS_SNT-3.mpg", &expected);

	memset( &expected, 0, sizeof(MediaScanResult) );
	expected.type = TYPE_VIDEO;
	strcpy(expected.mime_type, "video/vnd.dlna.mpeg-tts");
	strcpy(expected.dlna_profile, "MPEG_TS_SD_NA_T");
	expected.video_width = 704;
	expected.video_height = 480;
	strcpy(expected.video_fps, "30");

	// Audio part of the check
	strcpy(expected.audio_codec, "ac3");
	expected.audio_bitrate = 192000;
	expected.audio_samplerate = 48000;
	expected.audio_channels = 2;

	test_DLNA_files("O-MP2TS_SNT-4.mpg", &expected);

	memset( &expected, 0, sizeof(MediaScanResult) );
	expected.type = TYPE_VIDEO;
	strcpy(expected.mime_type, "video/vnd.dlna.mpeg-tts");
	strcpy(expected.dlna_profile, "MPEG_TS_SD_NA_T");
	expected.video_width = 704;
	expected.video_height = 480;
	strcpy(expected.video_fps, "30");

	// Audio part of the check
	strcpy(expected.audio_codec, "ac3");
	expected.audio_bitrate = 192000;
	expected.audio_samplerate = 48000;
	expected.audio_channels = 2;

	test_DLNA_files("O-MP2TS_SNT-5.mpg", &expected);

	memset( &expected, 0, sizeof(MediaScanResult) );
	expected.type = TYPE_VIDEO;
	strcpy(expected.mime_type, "video/vnd.dlna.mpeg-tts");
	strcpy(expected.dlna_profile, "MPEG_TS_SD_NA_T");
	expected.video_width = 704;
	expected.video_height = 480;
	strcpy(expected.video_fps, "30");

	// Audio part of the check
	strcpy(expected.audio_codec, "ac3");
	expected.audio_bitrate = 192000;
	expected.audio_samplerate = 48000;
	expected.audio_channels = 2;

	test_DLNA_files("O-MP2TS_SNT-6.mpg", &expected);

	memset( &expected, 0, sizeof(MediaScanResult) );
	expected.type = TYPE_VIDEO;
	strcpy(expected.mime_type, "video/vnd.dlna.mpeg-tts");
	strcpy(expected.dlna_profile, "MPEG_TS_SD_NA_T");
	expected.video_width = 704;
	expected.video_height = 480;
	strcpy(expected.video_fps, "30");

	// Audio part of the check
	strcpy(expected.audio_codec, "ac3");
	expected.audio_bitrate = 128000;
	expected.audio_samplerate = 48000;
	expected.audio_channels = 1;

	test_DLNA_files("O-MP2TS_SNT-7.mpg", &expected);

	memset( &expected, 0, sizeof(MediaScanResult) );
	expected.type = TYPE_VIDEO;
	strcpy(expected.mime_type, "video/vnd.dlna.mpeg-tts");
	strcpy(expected.dlna_profile, "MPEG_TS_SD_NA_T");
	expected.video_width = 704;
	expected.video_height = 480;
	strcpy(expected.video_fps, "30");

	// Audio part of the check
	strcpy(expected.audio_codec, "ac3");
	expected.audio_bitrate = 192000;
	expected.audio_samplerate = 48000;
	expected.audio_channels = 2;

	test_DLNA_files("O-MP2TS_SNT-8.mpg", &expected);

	memset( &expected, 0, sizeof(MediaScanResult) );
	expected.type = TYPE_VIDEO;
	strcpy(expected.mime_type, "video/vnd.dlna.mpeg-tts");
	strcpy(expected.dlna_profile, "MPEG_TS_SD_NA_T");
	expected.video_width = 704;
	expected.video_height = 480;
	strcpy(expected.video_fps, "29.97");

	// Audio part of the check
	strcpy(expected.audio_codec, "ac3");
	expected.audio_bitrate = 192000;
	expected.audio_samplerate = 48000;
	expected.audio_channels = 2;

	test_DLNA_files("O-MP2TS_SNT-9.mpg", &expected);

	memset( &expected, 0, sizeof(MediaScanResult) );
	expected.type = TYPE_VIDEO;
	strcpy(expected.mime_type, "video/vnd.dlna.mpeg-tts");
	strcpy(expected.dlna_profile, "MPEG_TS_SD_NA_T");
	expected.video_width = 704;
	expected.video_height = 480;
	strcpy(expected.video_fps, "29.97");

	// Audio part of the check
	strcpy(expected.audio_codec, "ac3");
	expected.audio_bitrate = 192000;
	expected.audio_samplerate = 48000;
	expected.audio_channels = 2;

	test_DLNA_files("O-MP2TS_SNT-10.mpg", &expected);

	memset( &expected, 0, sizeof(MediaScanResult) );
	expected.type = TYPE_VIDEO;
	strcpy(expected.mime_type, "video/vnd.dlna.mpeg-tts");
	strcpy(expected.dlna_profile, "MPEG_TS_SD_NA_T");
	expected.video_width = 704;
	expected.video_height = 480;
	strcpy(expected.video_fps, "24");

	// Audio part of the check
	strcpy(expected.audio_codec, "ac3");
	expected.audio_bitrate = 192000;
	expected.audio_samplerate = 48000;
	expected.audio_channels = 2;

	test_DLNA_files("O-MP2TS_SNT-11.mpg", &expected);

	memset( &expected, 0, sizeof(MediaScanResult) );
	expected.type = TYPE_VIDEO;
	strcpy(expected.mime_type, "video/vnd.dlna.mpeg-tts");
	strcpy(expected.dlna_profile, "MPEG_TS_SD_NA_T");
	expected.video_width = 704;
	expected.video_height = 480;
	strcpy(expected.video_fps, "59.96");

	// Audio part of the check
	strcpy(expected.audio_codec, "ac3");
	expected.audio_bitrate = 192000;
	expected.audio_samplerate = 48000;
	expected.audio_channels = 2;

	test_DLNA_files("O-MP2TS_SNT-12.mpg", &expected);

		memset( &expected, 0, sizeof(MediaScanResult) );
	expected.type = TYPE_VIDEO;
	strcpy(expected.mime_type, "video/vnd.dlna.mpeg-tts");
	strcpy(expected.dlna_profile, "MPEG_TS_SD_NA_T");
	expected.video_width = 704;
	expected.video_height = 480;
	strcpy(expected.video_fps, "23.98");

	// Audio part of the check
	strcpy(expected.audio_codec, "ac3");
	expected.audio_bitrate = 192000;
	expected.audio_samplerate = 48000;
	expected.audio_channels = 2;

	test_DLNA_files("O-MP2TS_SNT-13.mpg", &expected);


} /* test_MPEG_TS_SD_NA_T() */

void test_MPEG_TS_SD_NA_ISO()
{
    ExpectedResultType expected;


	memset( &expected, 0, sizeof(MediaScanResult) );
	expected.type = TYPE_VIDEO;
	strcpy(expected.mime_type, "video/mpeg");
	strcpy(expected.dlna_profile, "MPEG_TS_SD_NA_ISO");
	expected.video_width = 640;
	expected.video_height = 480;
	strcpy(expected.video_fps, "60");

	// Audio part of the check
	strcpy(expected.audio_codec, "ac3");
	expected.audio_bitrate = 192000;
	expected.audio_samplerate = 48000;
	expected.audio_channels = 2;

	test_DLNA_files("O-MP2TS_SN_I-1.mpg", &expected);


	memset( &expected, 0, sizeof(MediaScanResult) );
	expected.type = TYPE_VIDEO;
	strcpy(expected.mime_type, "video/mpeg");
	strcpy(expected.dlna_profile, "MPEG_TS_SD_NA_ISO");
	expected.video_width = 640;
	expected.video_height = 480;
	strcpy(expected.video_fps, "59.96");

	// Audio part of the check
	strcpy(expected.audio_codec, "ac3");
	expected.audio_bitrate = 192000;
	expected.audio_samplerate = 48000;
	expected.audio_channels = 2;

	test_DLNA_files("O-MP2TS_SN_I-2.mpg", &expected);


	memset( &expected, 0, sizeof(MediaScanResult) );
	expected.type = TYPE_VIDEO;
	strcpy(expected.mime_type, "video/mpeg");
	strcpy(expected.dlna_profile, "MPEG_TS_SD_NA_ISO");
	expected.video_width = 544;
	expected.video_height = 480;
	strcpy(expected.video_fps, "29.97");

	// Audio part of the check
	strcpy(expected.audio_codec, "ac3");
	expected.audio_bitrate = 192000;
	expected.audio_samplerate = 48000;
	expected.audio_channels = 2;

	test_DLNA_files("O-MP2TS_SN_I-3.mpg", &expected);

	memset( &expected, 0, sizeof(MediaScanResult) );
	expected.type = TYPE_VIDEO;
	strcpy(expected.mime_type, "video/mpeg");
	strcpy(expected.dlna_profile, "MPEG_TS_SD_NA_ISO");
	expected.video_width = 480;
	expected.video_height = 480;
	strcpy(expected.video_fps, "29.97");

	// Audio part of the check
	strcpy(expected.audio_codec, "ac3");
	expected.audio_bitrate = 192000;
	expected.audio_samplerate = 48000;
	expected.audio_channels = 2;

	test_DLNA_files("O-MP2TS_SN_I-4.mpg", &expected);

	memset( &expected, 0, sizeof(MediaScanResult) );
	expected.type = TYPE_VIDEO;
	strcpy(expected.mime_type, "video/mpeg");
	strcpy(expected.dlna_profile, "MPEG_TS_SD_NA_ISO");
	expected.video_width = 352;
	expected.video_height = 480;
	strcpy(expected.video_fps, "29.97");

	// Audio part of the check
	strcpy(expected.audio_codec, "ac3");
	expected.audio_bitrate = 192000;
	expected.audio_samplerate = 48000;
	expected.audio_channels = 2;

	test_DLNA_files("O-MP2TS_SN_I-5.mpg", &expected);

} /* test_MPEG_TS_SD_NA_ISO() */

void test_MPEG_TS_HD_NA_ISO()
{
    ExpectedResultType expected;


	memset( &expected, 0, sizeof(MediaScanResult) );
	expected.type = TYPE_VIDEO;
	strcpy(expected.mime_type, "video/mpeg");
	strcpy(expected.dlna_profile, "MPEG_TS_HD_NA_ISO");
	expected.video_width = 1920;
	expected.video_height = 1080;
	strcpy(expected.video_fps, "30");

	// Audio part of the check
	strcpy(expected.audio_codec, "ac3");
	expected.audio_bitrate = 192000;
	expected.audio_samplerate = 48000;
	expected.audio_channels = 2;

	test_DLNA_files("O-MP2TS_HN_I-1.mpg", &expected);


	memset( &expected, 0, sizeof(MediaScanResult) );
	expected.type = TYPE_VIDEO;
	strcpy(expected.mime_type, "video/mpeg");
	strcpy(expected.dlna_profile, "MPEG_TS_HD_NA_ISO");
	expected.video_width = 1920;
	expected.video_height = 1080;
	strcpy(expected.video_fps, "30");

	// Audio part of the check
	strcpy(expected.audio_codec, "ac3");
	expected.audio_bitrate = 192000;
	expected.audio_samplerate = 48000;
	expected.audio_channels = 2;

	test_DLNA_files("O-MP2TS_HN_I-2.mpg", &expected);


	memset( &expected, 0, sizeof(MediaScanResult) );
	expected.type = TYPE_VIDEO;
	strcpy(expected.mime_type, "video/mpeg");
	strcpy(expected.dlna_profile, "MPEG_TS_HD_NA_ISO");
	expected.video_width = 1920;
	expected.video_height = 1080;
	strcpy(expected.video_fps, "30");

	// Audio part of the check
	strcpy(expected.audio_codec, "ac3");
	expected.audio_bitrate = 192000;
	expected.audio_samplerate = 48000;
	expected.audio_channels = 2;

	test_DLNA_files("O-MP2TS_HN_I-3.mpg", &expected);

	memset( &expected, 0, sizeof(MediaScanResult) );
	expected.type = TYPE_VIDEO;
	strcpy(expected.mime_type, "video/mpeg");
	strcpy(expected.dlna_profile, "MPEG_TS_HD_NA_ISO");
	expected.video_width = 1920;
	expected.video_height = 1080;
	strcpy(expected.video_fps, "30");

	// Audio part of the check
	strcpy(expected.audio_codec, "ac3");
	expected.audio_bitrate = 192000;
	expected.audio_samplerate = 48000;
	expected.audio_channels = 2;

	test_DLNA_files("O-MP2TS_HN_I-4.mpg", &expected);

	memset( &expected, 0, sizeof(MediaScanResult) );
	expected.type = TYPE_VIDEO;
	strcpy(expected.mime_type, "video/mpeg");
	strcpy(expected.dlna_profile, "MPEG_TS_HD_NA_ISO");
	expected.video_width = 1920;
	expected.video_height = 1080;
	strcpy(expected.video_fps, "29.97");

	// Audio part of the check
	strcpy(expected.audio_codec, "ac3");
	expected.audio_bitrate = 128000;
	expected.audio_samplerate = 48000;
	expected.audio_channels = 1;

	test_DLNA_files("O-MP2TS_HN_I-5.mpg", &expected);

	memset( &expected, 0, sizeof(MediaScanResult) );
	expected.type = TYPE_VIDEO;
	strcpy(expected.mime_type, "video/mpeg");
	strcpy(expected.dlna_profile, "MPEG_TS_HD_NA_ISO");
	expected.video_width = 1920;
	expected.video_height = 1080;
	strcpy(expected.video_fps, "24");

	// Audio part of the check
	strcpy(expected.audio_codec, "ac3");
	expected.audio_bitrate = 192000;
	expected.audio_samplerate = 48000;
	expected.audio_channels = 2;

	test_DLNA_files("O-MP2TS_HN_I-6.mpg", &expected);

	memset( &expected, 0, sizeof(MediaScanResult) );
	expected.type = TYPE_VIDEO;
	strcpy(expected.mime_type, "video/mpeg");
	strcpy(expected.dlna_profile, "MPEG_TS_HD_NA_ISO");
	expected.video_width = 1920;
	expected.video_height = 1080;
	strcpy(expected.video_fps, "29.97");

	// Audio part of the check
	strcpy(expected.audio_codec, "ac3");
	expected.audio_bitrate = 192000;
	expected.audio_samplerate = 48000;
	expected.audio_channels = 2;

	test_DLNA_files("O-MP2TS_HN_I-7.mpg", &expected);

	memset( &expected, 0, sizeof(MediaScanResult) );
	expected.type = TYPE_VIDEO;
	strcpy(expected.mime_type, "video/mpeg");
	strcpy(expected.dlna_profile, "MPEG_TS_HD_NA_ISO");
	expected.video_width = 1920;
	expected.video_height = 1080;
	strcpy(expected.video_fps, "23.98");

	// Audio part of the check
	strcpy(expected.audio_codec, "ac3");
	expected.audio_bitrate = 192000;
	expected.audio_samplerate = 48000;
	expected.audio_channels = 2;

	test_DLNA_files("O-MP2TS_HN_I-8.mpg", &expected);

} /* test_MPEG_TS_HD_NA_ISO() */

void test_DLNA_large_files()
{
    ExpectedResultType expected;

	memset( &expected, 0, sizeof(MediaScanResult) );
	expected.type = TYPE_VIDEO;
	strcpy(expected.mime_type, "video/vnd.dlna.mpeg-tts");
	strcpy(expected.dlna_profile, "MPEG_TS_HD_NA_T");
	expected.video_width = 1920;
	expected.video_height = 1080;
	strcpy(expected.video_fps, "30");

	// Audio part of the check
	strcpy(expected.audio_codec, "ac3");
	expected.audio_bitrate = 192000;
	expected.audio_samplerate = 48000;
	expected.audio_channels = 2;

	test_DLNA_files("O-MP2TS_HN-2-long.mpg", &expected);


	// Test for MPEG_TS_HD_NA
	memset( &expected, 0, sizeof(MediaScanResult) );
	expected.type = TYPE_VIDEO;
	strcpy(expected.mime_type, "video/vnd.dlna.mpeg-tts");
	strcpy(expected.dlna_profile, "MPEG_TS_SD_NA_T");
	expected.video_width = 720;
	expected.video_height = 480;
	strcpy(expected.video_fps, "29.97");

	// Audio part of the check
	strcpy(expected.audio_codec, "ac3");
	expected.audio_bitrate = 192000;
	expected.audio_samplerate = 48000;
	expected.audio_channels = 2;

	test_DLNA_files("O-MP2TS_SNT-1-long.mpg", &expected);


} /* test_DLNA_large_files() */

void test_MPEG_PS_PAL()
{
    ExpectedResultType expected;


	memset( &expected, 0, sizeof(MediaScanResult) );
	expected.type = TYPE_VIDEO;
	strcpy(expected.mime_type, "video/mpeg");
	strcpy(expected.dlna_profile, "MPEG_PS_PAL");
	expected.video_width = 720;
	expected.video_height = 576;
	strcpy(expected.video_fps, "25");

	// Audio part of the check
	strcpy(expected.audio_codec, "pcm_s16be");
	expected.audio_bitrate = 1536000;
	expected.audio_samplerate = 48000;
	expected.audio_channels = 2;

	test_DLNA_files("B-MP2PS_P-2.mpg", &expected);


	memset( &expected, 0, sizeof(MediaScanResult) );
	expected.type = TYPE_VIDEO;
	strcpy(expected.mime_type, "video/mpeg");
	strcpy(expected.dlna_profile, "MPEG_PS_PAL");
	expected.video_width = 720;
	expected.video_height = 576;
	strcpy(expected.video_fps, "25");

	// Audio part of the check
	strcpy(expected.audio_codec, "ac3");
	expected.audio_bitrate = 224000;
	expected.audio_samplerate = 48000;
	expected.audio_channels = 2;

	test_DLNA_files("B-MP2PS_P-4.mpg", &expected);


	memset( &expected, 0, sizeof(MediaScanResult) );
	expected.type = TYPE_VIDEO;
	strcpy(expected.mime_type, "video/mpeg");
	strcpy(expected.dlna_profile, "MPEG_PS_PAL");
	expected.video_width = 720;
	expected.video_height = 576;
	strcpy(expected.video_fps, "25");

	// Audio part of the check
	strcpy(expected.audio_codec, "mp2");
	expected.audio_bitrate = 224000;
	expected.audio_samplerate = 48000;
	expected.audio_channels = 2;

	test_DLNA_files("B-MP2PS_P-6.mpg", &expected);

	memset( &expected, 0, sizeof(MediaScanResult) );
	expected.type = TYPE_VIDEO;
	strcpy(expected.mime_type, "video/mpeg");
	strcpy(expected.dlna_profile, "MPEG_PS_PAL");
	expected.video_width = 352;
	expected.video_height = 288;
	strcpy(expected.video_fps, "25");

	// Audio part of the check
	strcpy(expected.audio_codec, "mp2");
	expected.audio_bitrate = 64000;
	expected.audio_samplerate = 48000;
	expected.audio_channels = 1;

	test_DLNA_files("B-MP2PS_P-11.mpg", &expected);

	memset( &expected, 0, sizeof(MediaScanResult) );
	expected.type = TYPE_VIDEO;
	strcpy(expected.mime_type, "video/mpeg");
	strcpy(expected.dlna_profile, "MPEG_PS_PAL");
	expected.video_width = 720;
	expected.video_height = 576;
	strcpy(expected.video_fps, "25");

	// Audio part of the check
	strcpy(expected.audio_codec, "mp2");
	expected.audio_bitrate = 256000;
	expected.audio_samplerate = 48000;
	expected.audio_channels = 2;

	test_DLNA_files("B-MP2PS_P-12.mpg", &expected);

	memset( &expected, 0, sizeof(MediaScanResult) );
	expected.type = TYPE_VIDEO;
	strcpy(expected.mime_type, "video/mpeg");
	strcpy(expected.dlna_profile, "MPEG_PS_PAL");
	expected.video_width = 720;
	expected.video_height = 576;
	strcpy(expected.video_fps, "25");

	// Audio part of the check
	strcpy(expected.audio_codec, "mp2");
	expected.audio_bitrate = 224000;
	expected.audio_samplerate = 48000;
	expected.audio_channels = 2;

	test_DLNA_files("B-MP2PS_P-20.mpg", &expected);

	memset( &expected, 0, sizeof(MediaScanResult) );
	expected.type = TYPE_VIDEO;
	strcpy(expected.mime_type, "video/mpeg");
	strcpy(expected.dlna_profile, "MPEG_PS_PAL");
	expected.video_width = 720;
	expected.video_height = 576;
	strcpy(expected.video_fps, "25");

	// Audio part of the check
	strcpy(expected.audio_codec, "mp2");
	expected.audio_bitrate = 224000;
	expected.audio_samplerate = 48000;
	expected.audio_channels = 2;

	test_DLNA_files("B-MP2PS_P-21.mpg", &expected);

	memset( &expected, 0, sizeof(MediaScanResult) );
	expected.type = TYPE_VIDEO;
	strcpy(expected.mime_type, "video/mpeg");
	strcpy(expected.dlna_profile, "MPEG_PS_PAL");
	expected.video_width = 720;
	expected.video_height = 576;
	strcpy(expected.video_fps, "25");

	// Audio part of the check
	strcpy(expected.audio_codec, "ac3");
	expected.audio_bitrate = 224000;
	expected.audio_samplerate = 48000;
	expected.audio_channels = 2;

	test_DLNA_files("B-MP2PS_P-66.mpg", &expected);

	memset( &expected, 0, sizeof(MediaScanResult) );
	expected.type = TYPE_VIDEO;
	strcpy(expected.mime_type, "video/mpeg");
	strcpy(expected.dlna_profile, "MPEG_PS_PAL");
	expected.video_width = 720;
	expected.video_height = 576;
	strcpy(expected.video_fps, "25");

	// Audio part of the check
	strcpy(expected.audio_codec, "mp2");
	expected.audio_bitrate = 192000;
	expected.audio_samplerate = 48000;
	expected.audio_channels = 2;

	test_DLNA_files("B-MP2PS_P-88.mpg", &expected);

	memset( &expected, 0, sizeof(MediaScanResult) );
	expected.type = TYPE_VIDEO;
	strcpy(expected.mime_type, "video/mpeg");
	strcpy(expected.dlna_profile, "MPEG_PS_PAL");
	expected.video_width = 720;
	expected.video_height = 576;
	strcpy(expected.video_fps, "25");

	// Audio part of the check
	strcpy(expected.audio_codec, "mp2");
	expected.audio_bitrate = 192000;
	expected.audio_samplerate = 48000;
	expected.audio_channels = 1;

	test_DLNA_files("B-MP2PS_P-89.mpg", &expected);

	memset( &expected, 0, sizeof(MediaScanResult) );
	expected.type = TYPE_VIDEO;
	strcpy(expected.mime_type, "video/mpeg");
	strcpy(expected.dlna_profile, "MPEG_PS_PAL");
	expected.video_width = 720;
	expected.video_height = 576;
	strcpy(expected.video_fps, "25");

	// Audio part of the check
	strcpy(expected.audio_codec, "mp2");
	expected.audio_bitrate = 384000;
	expected.audio_samplerate = 48000;
	expected.audio_channels = 2;

	test_DLNA_files("B-MP2PS_P-90.mpg", &expected);

	memset( &expected, 0, sizeof(MediaScanResult) );
	expected.type = TYPE_VIDEO;
	strcpy(expected.mime_type, "video/mpeg");
	strcpy(expected.dlna_profile, "MPEG_PS_PAL");
	expected.video_width = 544;
	expected.video_height = 576;
	strcpy(expected.video_fps, "25");

	// Audio part of the check
	strcpy(expected.audio_codec, "mp2");
	expected.audio_bitrate = 384000;
	expected.audio_samplerate = 48000;
	expected.audio_channels = 2;

	test_DLNA_files("B-MP2PS_P-94.mpg", &expected);

	memset( &expected, 0, sizeof(MediaScanResult) );
	expected.type = TYPE_VIDEO;
	strcpy(expected.mime_type, "video/mpeg");
	strcpy(expected.dlna_profile, "MPEG_PS_PAL");
	expected.video_width = 480;
	expected.video_height = 576;
	strcpy(expected.video_fps, "25");

	// Audio part of the check
	strcpy(expected.audio_codec, "mp2");
	expected.audio_bitrate = 384000;
	expected.audio_samplerate = 48000;
	expected.audio_channels = 2;

	test_DLNA_files("B-MP2PS_P-96.mpg", &expected);

	memset( &expected, 0, sizeof(MediaScanResult) );
	expected.type = TYPE_VIDEO;
	strcpy(expected.mime_type, "video/mpeg");
	strcpy(expected.dlna_profile, "MPEG_PS_PAL");
	expected.video_width = 352;
	expected.video_height = 576;
	strcpy(expected.video_fps, "25");

	// Audio part of the check
	strcpy(expected.audio_codec, "mp2");
	expected.audio_bitrate = 384000;
	expected.audio_samplerate = 48000;
	expected.audio_channels = 2;

	test_DLNA_files("B-MP2PS_P-98.mpg", &expected);

	memset( &expected, 0, sizeof(MediaScanResult) );
	expected.type = TYPE_VIDEO;
	strcpy(expected.mime_type, "video/mpeg");
	strcpy(expected.dlna_profile, "MPEG_PS_PAL");
	expected.video_width = 720;
	expected.video_height = 576;
	strcpy(expected.video_fps, "25");

	// Audio part of the check
	strcpy(expected.audio_codec, "ac3");
	expected.audio_bitrate = 224000;
	expected.audio_samplerate = 48000;
	expected.audio_channels = 2;

	test_DLNA_files("B-MP2PS_P-106.mpg", &expected);

	memset( &expected, 0, sizeof(MediaScanResult) );
	expected.type = TYPE_VIDEO;
	strcpy(expected.mime_type, "video/mpeg");
	strcpy(expected.dlna_profile, "MPEG_PS_PAL");
	expected.video_width = 720;
	expected.video_height = 576;
	strcpy(expected.video_fps, "25");

	// Audio part of the check
	strcpy(expected.audio_codec, "ac3");
	expected.audio_bitrate = 224000;
	expected.audio_samplerate = 48000;
	expected.audio_channels = 2;

	test_DLNA_files("B-MP2PS_P-108.mpg", &expected);
} /* test_MPEG_PS_PAL() */

void test_MPEG_TS_SD_EU()
{
    ExpectedResultType expected;


	memset( &expected, 0, sizeof(MediaScanResult) );
	expected.type = TYPE_VIDEO;
	strcpy(expected.mime_type, "video/vnd.dlna.mpeg-tts");
	strcpy(expected.dlna_profile, "MPEG_TS_SD_EU");
	expected.video_width = 544;
	expected.video_height = 576;
	strcpy(expected.video_fps, "25");

	// Audio part of the check
	strcpy(expected.audio_codec, "ac3");
	expected.audio_bitrate = 128000;
	expected.audio_samplerate = 48000;
	expected.audio_channels = 1;

	test_DLNA_files("O-MP2TS_SE-1.mpg", &expected);


	memset( &expected, 0, sizeof(MediaScanResult) );
	expected.type = TYPE_VIDEO;
	strcpy(expected.mime_type, "video/vnd.dlna.mpeg-tts");
	strcpy(expected.dlna_profile, "MPEG_TS_SD_EU");
	expected.video_width = 480;
	expected.video_height = 576;
	strcpy(expected.video_fps, "25");

	// Audio part of the check
	strcpy(expected.audio_codec, "ac3");
	expected.audio_bitrate = 192000;
	expected.audio_samplerate = 44100;
	expected.audio_channels = 2;

	test_DLNA_files("O-MP2TS_SE-2.mpg", &expected);


	memset( &expected, 0, sizeof(MediaScanResult) );
	expected.type = TYPE_VIDEO;
	strcpy(expected.mime_type, "video/vnd.dlna.mpeg-tts");
	strcpy(expected.dlna_profile, "MPEG_TS_SD_EU");
	expected.video_width = 352;
	expected.video_height = 576;
	strcpy(expected.video_fps, "25");

	// Audio part of the check
	strcpy(expected.audio_codec, "ac3");
	expected.audio_bitrate = 192000;
	expected.audio_samplerate = 32000;
	expected.audio_channels = 2;

	test_DLNA_files("O-MP2TS_SE-3.mpg", &expected);

	memset( &expected, 0, sizeof(MediaScanResult) );
	expected.type = TYPE_VIDEO;
	strcpy(expected.mime_type, "video/vnd.dlna.mpeg-tts");
	strcpy(expected.dlna_profile, "MPEG_TS_SD_EU");
	expected.video_width = 352;
	expected.video_height = 288;
	strcpy(expected.video_fps, "25");

	// Audio part of the check
	strcpy(expected.audio_codec, "ac3");
	expected.audio_bitrate = 192000;
	expected.audio_samplerate = 44100;
	expected.audio_channels = 2;

	test_DLNA_files("O-MP2TS_SE-4.mpg", &expected);

} /* test_MPEG_TS_SD_EU() */

void test_MPEG_TS_SD_EU_T()
{
    ExpectedResultType expected;


	memset( &expected, 0, sizeof(MediaScanResult) );
	expected.type = TYPE_VIDEO;
	strcpy(expected.mime_type, "video/vnd.dlna.mpeg-tts");
	strcpy(expected.dlna_profile, "MPEG_TS_SD_EU_T");
	expected.video_width = 720;
	expected.video_height = 576;
	strcpy(expected.video_fps, "25");

	// Audio part of the check
	strcpy(expected.audio_codec, "mp2");
	expected.audio_bitrate = 384000;
	expected.audio_samplerate = 48000;
	expected.audio_channels = 2;

	test_DLNA_files("O-MP2TS_SET-1.mpg", &expected);


	memset( &expected, 0, sizeof(MediaScanResult) );
	expected.type = TYPE_VIDEO;
	strcpy(expected.mime_type, "video/vnd.dlna.mpeg-tts");
	strcpy(expected.dlna_profile, "MPEG_TS_SD_EU_T");
	expected.video_width = 720;
	expected.video_height = 576;
	strcpy(expected.video_fps, "25");

	// Audio part of the check
	strcpy(expected.audio_codec, "mp2");
	expected.audio_bitrate = 64000;
	expected.audio_samplerate = 48000;
	expected.audio_channels = 1;

	test_DLNA_files("O-MP2TS_SET-2.mpg", &expected);


	memset( &expected, 0, sizeof(MediaScanResult) );
	expected.type = TYPE_VIDEO;
	strcpy(expected.mime_type, "video/vnd.dlna.mpeg-tts");
	strcpy(expected.dlna_profile, "MPEG_TS_SD_EU_T");
	expected.video_width = 720;
	expected.video_height = 576;
	strcpy(expected.video_fps, "25");

	// Audio part of the check
	strcpy(expected.audio_codec, "mp2");
	expected.audio_bitrate = 384000;
	expected.audio_samplerate = 32000;
	expected.audio_channels = 2;

	test_DLNA_files("O-MP2TS_SET-3.mpg", &expected);

	memset( &expected, 0, sizeof(MediaScanResult) );
	expected.type = TYPE_VIDEO;
	strcpy(expected.mime_type, "video/vnd.dlna.mpeg-tts");
	strcpy(expected.dlna_profile, "MPEG_TS_SD_EU_T");
	expected.video_width = 720;
	expected.video_height = 576;
	strcpy(expected.video_fps, "25");

	// Audio part of the check
	strcpy(expected.audio_codec, "mp2");
	expected.audio_bitrate = 384000;
	expected.audio_samplerate = 44100;
	expected.audio_channels = 2;

	test_DLNA_files("O-MP2TS_SET-4.mpg", &expected);

	memset( &expected, 0, sizeof(MediaScanResult) );
	expected.type = TYPE_VIDEO;
	strcpy(expected.mime_type, "video/vnd.dlna.mpeg-tts");
	strcpy(expected.dlna_profile, "MPEG_TS_SD_EU_T");
	expected.video_width = 720;
	expected.video_height = 576;
	strcpy(expected.video_fps, "25");

	// Audio part of the check
	strcpy(expected.audio_codec, "mp1");
	expected.audio_bitrate = 448000;
	expected.audio_samplerate = 48000;
	expected.audio_channels = 2;

	test_DLNA_files("O-MP2TS_SET-5.mpg", &expected);

	memset( &expected, 0, sizeof(MediaScanResult) );
	expected.type = TYPE_VIDEO;
	strcpy(expected.mime_type, "video/vnd.dlna.mpeg-tts");
	strcpy(expected.dlna_profile, "MPEG_TS_SD_EU_T");
	expected.video_width = 720;
	expected.video_height = 576;
	strcpy(expected.video_fps, "25");

	// Audio part of the check
	strcpy(expected.audio_codec, "mp1");
	expected.audio_bitrate = 96000;
	expected.audio_samplerate = 48000;
	expected.audio_channels = 1;

	test_DLNA_files("O-MP2TS_SET-6.mpg", &expected);

	memset( &expected, 0, sizeof(MediaScanResult) );
	expected.type = TYPE_VIDEO;
	strcpy(expected.mime_type, "video/vnd.dlna.mpeg-tts");
	strcpy(expected.dlna_profile, "MPEG_TS_SD_EU_T");
	expected.video_width = 720;
	expected.video_height = 576;
	strcpy(expected.video_fps, "25");

	// Audio part of the check
	strcpy(expected.audio_codec, "mp1");
	expected.audio_bitrate = 448000;
	expected.audio_samplerate = 32000;
	expected.audio_channels = 2;

	test_DLNA_files("O-MP2TS_SET-7.mpg", &expected);

	memset( &expected, 0, sizeof(MediaScanResult) );
	expected.type = TYPE_VIDEO;
	strcpy(expected.mime_type, "video/vnd.dlna.mpeg-tts");
	strcpy(expected.dlna_profile, "MPEG_TS_SD_EU_T");
	expected.video_width = 720;
	expected.video_height = 576;
	strcpy(expected.video_fps, "25");

	// Audio part of the check
	strcpy(expected.audio_codec, "mp1");
	expected.audio_bitrate = 448000;
	expected.audio_samplerate = 48000;
	expected.audio_channels = 2;

	test_DLNA_files("O-MP2TS_SET-8.mpg", &expected);

	memset( &expected, 0, sizeof(MediaScanResult) );
	expected.type = TYPE_VIDEO;
	strcpy(expected.mime_type, "video/vnd.dlna.mpeg-tts");
	strcpy(expected.dlna_profile, "MPEG_TS_SD_EU_T");
	expected.video_width = 720;
	expected.video_height = 576;
	strcpy(expected.video_fps, "25");

	// Audio part of the check
	strcpy(expected.audio_codec, "ac3");
	expected.audio_bitrate = 192000;
	expected.audio_samplerate = 48000;
	expected.audio_channels = 2;

	test_DLNA_files("O-MP2TS_SET-9.mpg", &expected);
} /* test_MPEG_TS_SD_EU_T() */

void test_MPEG_TS_SD_EU_ISO()
{
    ExpectedResultType expected;


	memset( &expected, 0, sizeof(MediaScanResult) );
	expected.type = TYPE_VIDEO;
	strcpy(expected.mime_type, "video/mpeg");
	strcpy(expected.dlna_profile, "MPEG_TS_SD_EU_ISO");
	expected.video_width = 544;
	expected.video_height = 576;
	strcpy(expected.video_fps, "25");

	// Audio part of the check
	strcpy(expected.audio_codec, "ac3");
	expected.audio_bitrate = 128000;
	expected.audio_samplerate = 48000;
	expected.audio_channels = 1;

	test_DLNA_files("O-MP2TS_SE_I-1.mpg", &expected);


	memset( &expected, 0, sizeof(MediaScanResult) );
	expected.type = TYPE_VIDEO;
	strcpy(expected.mime_type, "video/mpeg");
	strcpy(expected.dlna_profile, "MPEG_TS_SD_EU_ISO");
	expected.video_width = 480;
	expected.video_height = 576;
	strcpy(expected.video_fps, "25");

	// Audio part of the check
	strcpy(expected.audio_codec, "ac3");
	expected.audio_bitrate = 192000;
	expected.audio_samplerate = 44100;
	expected.audio_channels = 2;

	test_DLNA_files("O-MP2TS_SE_I-2.mpg", &expected);


	memset( &expected, 0, sizeof(MediaScanResult) );
	expected.type = TYPE_VIDEO;
	strcpy(expected.mime_type, "video/mpeg");
	strcpy(expected.dlna_profile, "MPEG_TS_SD_EU_ISO");
	expected.video_width = 352;
	expected.video_height = 576;
	strcpy(expected.video_fps, "25");

	// Audio part of the check
	strcpy(expected.audio_codec, "ac3");
	expected.audio_bitrate = 192000;
	expected.audio_samplerate = 32000;
	expected.audio_channels = 2;

	test_DLNA_files("O-MP2TS_SE_I-3.mpg", &expected);

	memset( &expected, 0, sizeof(MediaScanResult) );
	expected.type = TYPE_VIDEO;
	strcpy(expected.mime_type, "video/mpeg");
	strcpy(expected.dlna_profile, "MPEG_TS_SD_EU_ISO");
	expected.video_width = 352;
	expected.video_height = 288;
	strcpy(expected.video_fps, "25");

	// Audio part of the check
	strcpy(expected.audio_codec, "ac3");
	expected.audio_bitrate = 192000;
	expected.audio_samplerate = 44100;
	expected.audio_channels = 2;

	test_DLNA_files("O-MP2TS_SE_I-4.mpg", &expected);

	memset( &expected, 0, sizeof(MediaScanResult) );
	expected.type = TYPE_VIDEO;
	strcpy(expected.mime_type, "video/mpeg");
	strcpy(expected.dlna_profile, "MPEG_TS_SD_EU_ISO");
	expected.video_width = 720;
	expected.video_height = 576;
	strcpy(expected.video_fps, "25");

	// Audio part of the check
	strcpy(expected.audio_codec, "mp2");
	expected.audio_bitrate = 384000;
	expected.audio_samplerate = 48000;
	expected.audio_channels = 2;

	test_DLNA_files("O-MP2TS_SE_I-5.mpg", &expected);

} /* test_MPEG_TS_SD_EU_ISO() */

void test_MPEG_TS_SD_KO()
{
    ExpectedResultType expected;


	memset( &expected, 0, sizeof(MediaScanResult) );
	expected.type = TYPE_VIDEO;
	strcpy(expected.mime_type, "video/mpeg");
	strcpy(expected.dlna_profile, "MPEG_TS_SD_KO");
	expected.video_width = 640;
	expected.video_height = 480;
	strcpy(expected.video_fps, "59.96");

	// Audio part of the check
	strcpy(expected.audio_codec, "ac3");
	expected.audio_bitrate = 192000;
	expected.audio_samplerate = 48000;
	expected.audio_channels = 2;

	test_DLNA_files("O-MP2TS_SK-2.mpg", &expected);


	memset( &expected, 0, sizeof(MediaScanResult) );
	expected.type = TYPE_VIDEO;
	strcpy(expected.mime_type, "video/mpeg");
	strcpy(expected.dlna_profile, "MPEG_TS_SD_KO");
	expected.video_width = 704;
	expected.video_height = 480;
	strcpy(expected.video_fps, "30");

	// Audio part of the check
	strcpy(expected.audio_codec, "ac3");
	expected.audio_bitrate = 192000;
	expected.audio_samplerate = 48000;
	expected.audio_channels = 2;

	test_DLNA_files("O-MP2TS_SK-4.mpg", &expected);


	memset( &expected, 0, sizeof(MediaScanResult) );
	expected.type = TYPE_VIDEO;
	strcpy(expected.mime_type, "video/mpeg");
	strcpy(expected.dlna_profile, "MPEG_TS_SD_KO");
	expected.video_width = 704;
	expected.video_height = 480;
	strcpy(expected.video_fps, "30");

	// Audio part of the check
	strcpy(expected.audio_codec, "ac3");
	expected.audio_bitrate = 192000;
	expected.audio_samplerate = 48000;
	expected.audio_channels = 2;

	test_DLNA_files("O-MP2TS_SK-5.mpg", &expected);

	memset( &expected, 0, sizeof(MediaScanResult) );
	expected.type = TYPE_VIDEO;
	strcpy(expected.mime_type, "video/mpeg");
	strcpy(expected.dlna_profile, "MPEG_TS_SD_KO");
	expected.video_width = 704;
	expected.video_height = 480;
	strcpy(expected.video_fps, "30");

	// Audio part of the check
	strcpy(expected.audio_codec, "ac3");
	expected.audio_bitrate = 192000;
	expected.audio_samplerate = 48000;
	expected.audio_channels = 2;

	test_DLNA_files("O-MP2TS_SK-6.mpg", &expected);

	memset( &expected, 0, sizeof(MediaScanResult) );
	expected.type = TYPE_VIDEO;
	strcpy(expected.mime_type, "video/mpeg");
	strcpy(expected.dlna_profile, "MPEG_TS_SD_KO");
	expected.video_width = 704;
	expected.video_height = 480;
	strcpy(expected.video_fps, "30");

	// Audio part of the check
	strcpy(expected.audio_codec, "ac3");
	expected.audio_bitrate = 128000;
	expected.audio_samplerate = 48000;
	expected.audio_channels = 1;

	test_DLNA_files("O-MP2TS_SK-7.mpg", &expected);

	memset( &expected, 0, sizeof(MediaScanResult) );
	expected.type = TYPE_VIDEO;
	strcpy(expected.mime_type, "video/mpeg");
	strcpy(expected.dlna_profile, "MPEG_TS_SD_KO");
	expected.video_width = 704;
	expected.video_height = 480;
	strcpy(expected.video_fps, "30");

	// Audio part of the check
	strcpy(expected.audio_codec, "ac3");
	expected.audio_bitrate = 192000;
	expected.audio_samplerate = 48000;
	expected.audio_channels = 2;

	test_DLNA_files("O-MP2TS_SK-8.mpg", &expected);

	memset( &expected, 0, sizeof(MediaScanResult) );
	expected.type = TYPE_VIDEO;
	strcpy(expected.mime_type, "video/mpeg");
	strcpy(expected.dlna_profile, "MPEG_TS_SD_KO");
	expected.video_width = 704;
	expected.video_height = 480;
	strcpy(expected.video_fps, "29.97");

	// Audio part of the check
	strcpy(expected.audio_codec, "ac3");
	expected.audio_bitrate = 192000;
	expected.audio_samplerate = 48000;
	expected.audio_channels = 2;

	test_DLNA_files("O-MP2TS_SK-9.mpg", &expected);

	memset( &expected, 0, sizeof(MediaScanResult) );
	expected.type = TYPE_VIDEO;
	strcpy(expected.mime_type, "video/mpeg");
	strcpy(expected.dlna_profile, "MPEG_TS_SD_KO");
	expected.video_width = 704;
	expected.video_height = 480;
	strcpy(expected.video_fps, "24");

	// Audio part of the check
	strcpy(expected.audio_codec, "ac3");
	expected.audio_bitrate = 192000;
	expected.audio_samplerate = 48000;
	expected.audio_channels = 2;

	test_DLNA_files("O-MP2TS_SK-11.mpg", &expected);

	memset( &expected, 0, sizeof(MediaScanResult) );
	expected.type = TYPE_VIDEO;
	strcpy(expected.mime_type, "video/mpeg");
	strcpy(expected.dlna_profile, "MPEG_TS_SD_KO");
	expected.video_width = 704;
	expected.video_height = 480;
	strcpy(expected.video_fps, "23.98");

	// Audio part of the check
	strcpy(expected.audio_codec, "ac3");
	expected.audio_bitrate = 192000;
	expected.audio_samplerate = 48000;
	expected.audio_channels = 2;

	test_DLNA_files("O-MP2TS_SK-13.mpg", &expected);

	memset( &expected, 0, sizeof(MediaScanResult) );
	expected.type = TYPE_VIDEO;
	strcpy(expected.mime_type, "video/mpeg");
	strcpy(expected.dlna_profile, "MPEG_TS_SD_KO");
	expected.video_width = 704;
	expected.video_height = 480;
	strcpy(expected.video_fps, "23.98");

	// Audio part of the check
	strcpy(expected.audio_codec, "ac3");
	expected.audio_bitrate = 192000;
	expected.audio_samplerate = 48000;
	expected.audio_channels = 2;

	test_DLNA_files("O-MP2TS_SK-13a.mpg", &expected);
} /* test_MPEG_TS_SD_KO() */

void test_MPEG_TS_SD_KO_T()
{
    ExpectedResultType expected;


	memset( &expected, 0, sizeof(MediaScanResult) );
	expected.type = TYPE_VIDEO;
	strcpy(expected.mime_type, "video/mpeg");
	strcpy(expected.dlna_profile, "MPEG_TS_SD_KO_T");
	expected.video_width = 704;
	expected.video_height = 480;
	strcpy(expected.video_fps, "30");

	// Audio part of the check
	strcpy(expected.audio_codec, "ac3");
	expected.audio_bitrate = 192000;
	expected.audio_samplerate = 48000;
	expected.audio_channels = 2;

	test_DLNA_files("O-MP2TS_SKT-4.mpg", &expected);


	memset( &expected, 0, sizeof(MediaScanResult) );
	expected.type = TYPE_VIDEO;
	strcpy(expected.mime_type, "video/mpeg");
	strcpy(expected.dlna_profile, "MPEG_TS_SD_KO_T");
	expected.video_width = 704;
	expected.video_height = 480;
	strcpy(expected.video_fps, "30");

	// Audio part of the check
	strcpy(expected.audio_codec, "ac3");
	expected.audio_bitrate = 192000;
	expected.audio_samplerate = 48000;
	expected.audio_channels = 2;

	test_DLNA_files("O-MP2TS_SKT-5.mpg", &expected);


	memset( &expected, 0, sizeof(MediaScanResult) );
	expected.type = TYPE_VIDEO;
	strcpy(expected.mime_type, "video/mpeg");
	strcpy(expected.dlna_profile, "MPEG_TS_SD_KO_T");
	expected.video_width = 704;
	expected.video_height = 480;
	strcpy(expected.video_fps, "30");

	// Audio part of the check
	strcpy(expected.audio_codec, "ac3");
	expected.audio_bitrate = 192000;
	expected.audio_samplerate = 48000;
	expected.audio_channels = 2;

	test_DLNA_files("O-MP2TS_SKT-6.mpg", &expected);

	memset( &expected, 0, sizeof(MediaScanResult) );
	expected.type = TYPE_VIDEO;
	strcpy(expected.mime_type, "video/mpeg");
	strcpy(expected.dlna_profile, "MPEG_TS_SD_KO_T");
	expected.video_width = 704;
	expected.video_height = 480;
	strcpy(expected.video_fps, "30");

	// Audio part of the check
	strcpy(expected.audio_codec, "ac3");
	expected.audio_bitrate = 128000;
	expected.audio_samplerate = 48000;
	expected.audio_channels = 1;

	test_DLNA_files("O-MP2TS_SKT-7.mpg", &expected);

	memset( &expected, 0, sizeof(MediaScanResult) );
	expected.type = TYPE_VIDEO;
	strcpy(expected.mime_type, "video/mpeg");
	strcpy(expected.dlna_profile, "MPEG_TS_SD_KO_T");
	expected.video_width = 704;
	expected.video_height = 480;
	strcpy(expected.video_fps, "30");

	// Audio part of the check
	strcpy(expected.audio_codec, "ac3");
	expected.audio_bitrate = 192000;
	expected.audio_samplerate = 48000;
	expected.audio_channels = 2;

	test_DLNA_files("O-MP2TS_SKT-8.mpg", &expected);

	memset( &expected, 0, sizeof(MediaScanResult) );
	expected.type = TYPE_VIDEO;
	strcpy(expected.mime_type, "video/mpeg");
	strcpy(expected.dlna_profile, "MPEG_TS_SD_KO_T");
	expected.video_width = 704;
	expected.video_height = 480;
	strcpy(expected.video_fps, "29.97");

	// Audio part of the check
	strcpy(expected.audio_codec, "ac3");
	expected.audio_bitrate = 192000;
	expected.audio_samplerate = 48000;
	expected.audio_channels = 2;

	test_DLNA_files("O-MP2TS_SKT-9.mpg", &expected);

	memset( &expected, 0, sizeof(MediaScanResult) );
	expected.type = TYPE_VIDEO;
	strcpy(expected.mime_type, "video/mpeg");
	strcpy(expected.dlna_profile, "MPEG_TS_SD_KO_T");
	expected.video_width = 704;
	expected.video_height = 480;
	strcpy(expected.video_fps, "24");

	// Audio part of the check
	strcpy(expected.audio_codec, "ac3");
	expected.audio_bitrate = 192000;
	expected.audio_samplerate = 48000;
	expected.audio_channels = 2;

	test_DLNA_files("O-MP2TS_SKT-11.mpg", &expected);

	memset( &expected, 0, sizeof(MediaScanResult) );
	expected.type = TYPE_VIDEO;
	strcpy(expected.mime_type, "video/mpeg");
	strcpy(expected.dlna_profile, "MPEG_TS_SD_KO_T");
	expected.video_width = 704;
	expected.video_height = 480;
	strcpy(expected.video_fps, "23.98");

	// Audio part of the check
	strcpy(expected.audio_codec, "ac3");
	expected.audio_bitrate = 192000;
	expected.audio_samplerate = 48000;
	expected.audio_channels = 2;

	test_DLNA_files("O-MP2TS_SKT-13.mpg", &expected);

	memset( &expected, 0, sizeof(MediaScanResult) );
	expected.type = TYPE_VIDEO;
	strcpy(expected.mime_type, "video/mpeg");
	strcpy(expected.dlna_profile, "MPEG_TS_SD_KO_T");
	expected.video_width = 704;
	expected.video_height = 480;
	strcpy(expected.video_fps, "23.98");

	// Audio part of the check
	strcpy(expected.audio_codec, "ac3");
	expected.audio_bitrate = 192000;
	expected.audio_samplerate = 48000;
	expected.audio_channels = 2;

	test_DLNA_files("O-MP2TS_SKT-13a.mpg", &expected);

} /* test_MPEG_TS_SD_KO_T() */

void test_MPEG_TS_SD_KO_ISO()
{
    ExpectedResultType expected;


	memset( &expected, 0, sizeof(MediaScanResult) );
	expected.type = TYPE_VIDEO;
	strcpy(expected.mime_type, "video/mpeg");
	strcpy(expected.dlna_profile, "MPEG_TS_SD_KO_ISO");
	expected.video_width = 704;
	expected.video_height = 480;
	strcpy(expected.video_fps, "30");

	// Audio part of the check
	strcpy(expected.audio_codec, "ac3");
	expected.audio_bitrate = 192000;
	expected.audio_samplerate = 48000;
	expected.audio_channels = 2;

	test_DLNA_files("O-MP2TS_SK_I-4.mpg", &expected);


	memset( &expected, 0, sizeof(MediaScanResult) );
	expected.type = TYPE_VIDEO;
	strcpy(expected.mime_type, "video/mpeg");
	strcpy(expected.dlna_profile, "MPEG_TS_SD_KO_ISO");
	expected.video_width = 704;
	expected.video_height = 480;
	strcpy(expected.video_fps, "30");

	// Audio part of the check
	strcpy(expected.audio_codec, "ac3");
	expected.audio_bitrate = 192000;
	expected.audio_samplerate = 48000;
	expected.audio_channels = 2;

	test_DLNA_files("O-MP2TS_SK_I-5.mpg", &expected);


	memset( &expected, 0, sizeof(MediaScanResult) );
	expected.type = TYPE_VIDEO;
	strcpy(expected.mime_type, "video/mpeg");
	strcpy(expected.dlna_profile, "MPEG_TS_SD_KO_ISO");
	expected.video_width = 704;
	expected.video_height = 480;
	strcpy(expected.video_fps, "30");

	// Audio part of the check
	strcpy(expected.audio_codec, "ac3");
	expected.audio_bitrate = 192000;
	expected.audio_samplerate = 48000;
	expected.audio_channels = 2;

	test_DLNA_files("O-MP2TS_SK_I-6.mpg", &expected);

	memset( &expected, 0, sizeof(MediaScanResult) );
	expected.type = TYPE_VIDEO;
	strcpy(expected.mime_type, "video/mpeg");
	strcpy(expected.dlna_profile, "MPEG_TS_SD_KO_ISO");
	expected.video_width = 704;
	expected.video_height = 480;
	strcpy(expected.video_fps, "30");

	// Audio part of the check
	strcpy(expected.audio_codec, "ac3");
	expected.audio_bitrate = 128000;
	expected.audio_samplerate = 48000;
	expected.audio_channels = 1;

	test_DLNA_files("O-MP2TS_SK_I-7.mpg", &expected);

	memset( &expected, 0, sizeof(MediaScanResult) );
	expected.type = TYPE_VIDEO;
	strcpy(expected.mime_type, "video/mpeg");
	strcpy(expected.dlna_profile, "MPEG_TS_SD_KO_ISO");
	expected.video_width = 704;
	expected.video_height = 480;
	strcpy(expected.video_fps, "30");

	// Audio part of the check
	strcpy(expected.audio_codec, "ac3");
	expected.audio_bitrate = 192000;
	expected.audio_samplerate = 48000;
	expected.audio_channels = 2;

	test_DLNA_files("O-MP2TS_SK_I-8.mpg", &expected);

	memset( &expected, 0, sizeof(MediaScanResult) );
	expected.type = TYPE_VIDEO;
	strcpy(expected.mime_type, "video/mpeg");
	strcpy(expected.dlna_profile, "MPEG_TS_SD_KO_ISO");
	expected.video_width = 704;
	expected.video_height = 480;
	strcpy(expected.video_fps, "29.97");

	// Audio part of the check
	strcpy(expected.audio_codec, "ac3");
	expected.audio_bitrate = 192000;
	expected.audio_samplerate = 48000;
	expected.audio_channels = 2;

	test_DLNA_files("O-MP2TS_SK_I-9.mpg", &expected);

	memset( &expected, 0, sizeof(MediaScanResult) );
	expected.type = TYPE_VIDEO;
	strcpy(expected.mime_type, "video/mpeg");
	strcpy(expected.dlna_profile, "MPEG_TS_SD_KO_ISO");
	expected.video_width = 704;
	expected.video_height = 480;
	strcpy(expected.video_fps, "29.97");

	// Audio part of the check
	strcpy(expected.audio_codec, "ac3");
	expected.audio_bitrate = 192000;
	expected.audio_samplerate = 48000;
	expected.audio_channels = 2;

	test_DLNA_files("O-MP2TS_SK-9.mpg", &expected);

	memset( &expected, 0, sizeof(MediaScanResult) );
	expected.type = TYPE_VIDEO;
	strcpy(expected.mime_type, "video/mpeg");
	strcpy(expected.dlna_profile, "MPEG_TS_SD_KO_ISO");
	expected.video_width = 704;
	expected.video_height = 480;
	strcpy(expected.video_fps, "24");

	// Audio part of the check
	strcpy(expected.audio_codec, "ac3");
	expected.audio_bitrate = 192000;
	expected.audio_samplerate = 48000;
	expected.audio_channels = 2;

	test_DLNA_files("O-MP2TS_SK_I-11.mpg", &expected);

	memset( &expected, 0, sizeof(MediaScanResult) );
	expected.type = TYPE_VIDEO;
	strcpy(expected.mime_type, "video/mpeg");
	strcpy(expected.dlna_profile, "MPEG_TS_SD_KO_ISO");
	expected.video_width = 704;
	expected.video_height = 480;
	strcpy(expected.video_fps, "23.98");

	// Audio part of the check
	strcpy(expected.audio_codec, "ac3");
	expected.audio_bitrate = 192000;
	expected.audio_samplerate = 48000;
	expected.audio_channels = 2;

	test_DLNA_files("O-MP2TS_SK_I-13.mpg", &expected);

	memset( &expected, 0, sizeof(MediaScanResult) );
	expected.type = TYPE_VIDEO;
	strcpy(expected.mime_type, "video/mpeg");
	strcpy(expected.dlna_profile, "MPEG_TS_SD_KO_ISO");
	expected.video_width = 704;
	expected.video_height = 480;
	strcpy(expected.video_fps, "23.98");

	// Audio part of the check
	strcpy(expected.audio_codec, "ac3");
	expected.audio_bitrate = 192000;
	expected.audio_samplerate = 48000;
	expected.audio_channels = 2;

	test_DLNA_files("O-MP2TS_SK_I-13a.mpg", &expected);
} /* test_MPEG_TS_SD_KO_ISO() */

void test_DLNA_mp3s()
{
	ExpectedResultType expected;

	memset( &expected, 0, sizeof(MediaScanResult) );
	expected.type = TYPE_AUDIO;
	strcpy(expected.mime_type, "video/mpeg");
	strcpy(expected.dlna_profile, "MPEG_TS_SD_KO_ISO");

	//expected.video_width = 704;
	//expected.video_height = 480;
	//strcpy(expected.video_fps, "24");

	// Audio part of the check
	strcpy(expected.audio_codec, "aac");
	expected.audio_bitrate = 96000;
	expected.audio_samplerate = 44100;
	expected.audio_channels = 2;

	test_DLNA_audio_files("O-AAC_ADTS_320-stereo-44.1kHz-96k.adts", &expected);
}

void test_DLNA_scanning(char *argv[])
{
#ifndef WIN32
  char *bin = NULL;
#endif

  char *dir = NULL;
  MediaScan *s = NULL;

#ifdef WIN32
//  dir = _abspath(bin, "data\\video\\dlna"); // because binary is in .libs dir
  dir = strdup("data\\video\\dlna");
#else
  // Get path to this binary
  bin = _findbin(argv[0]);
  dir = _abspath(bin, "../data/video/dlna"); // because binary is in .libs dir
#endif

  s = ms_create();
  ms_add_path(s, dir);
  ms_set_result_callback(s, my_result_callback);
  ms_set_error_callback(s, my_error_callback);
  ms_scan(s);
  ms_destroy(s);

  free(dir);

#ifndef WIN32
  free(bin);
#endif

}

int
main(int argc, char *argv[])
{

  ExpectedResultType expected;

  plan(TEST_COUNT);

 // test_DLNA_scanning(argv);

// Test US profiles
  test_MPEG_TS_SD_NA();
  test_MPEG_TS_SD_NA_T();
  test_MPEG_TS_SD_NA_ISO();
  test_MPEG_TS_HD_NA();
  test_MPEG_TS_HD_NA_T();
  test_MPEG_TS_HD_NA_ISO();

// Test EU profiles
  test_MPEG_TS_SD_EU();
  test_MPEG_TS_SD_EU_T();
  test_MPEG_TS_SD_EU_ISO();

// Test KO Profiles
// Note: DLNA library will identify all of these streams as NA streams and therefore the
// tests will fail
//  test_MPEG_TS_SD_KO();
//  test_MPEG_TS_SD_KO_T();
//  test_MPEG_TS_SD_KO_ISO();

// Test MPEG
  test_MPEG_PS_PAL();
  test_MPEG_PS_NTSC();

  test_DLNA_large_files();

// Test MP3
//	test_DLNA_mp3s();

  return exit_status();
} /* main() */
