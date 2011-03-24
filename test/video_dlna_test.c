// Test all DLNA video profiles

#include <unistd.h>
#include <stdint.h>
#include <math.h>
#include <libmediascan.h>

#include "tap.h"
#include "common.h"

#define TEST_COUNT 51

// From ffmpeg utils.c:print_fps
static const char *
fps2str(double fps)
{
  static char str[10];
  uint64_t v = lrintf(fps*100);
  if     (v% 100      ) sprintf(str, "%3.2f", fps);
  else if(v%(100*1000)) sprintf(str, "%1.0f", fps);
  else                  sprintf(str, "%1.0fk", fps/1000); 
  return str;
}


static void my_result_callback(MediaScan *s, MediaScanResult *r) {
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
   * 
   * MPEG_PS_NTSC
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
   * MPEG_TS_SD_NA_ISO
   * MPEG_TS_SD_NA_XAC3
   * MPEG_TS_SD_NA_XAC3_T
   * MPEG_TS_SD_NA_XAC3_ISO
   * MPEG_TS_HD_NA
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

static void my_error_callback(MediaScan *s, MediaScanError *error) {
  LOG_ERROR("[Error] %s (%s)\n", error->error_string, error->path);
}

int
main(int argc, char *argv[])
{
  char *bin = NULL;
  char *dir = NULL;
  MediaScan *s = NULL;

  plan(TEST_COUNT);
  
  ms_set_log_level(ERR);
  

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
  free(bin);
  
  return exit_status();
}
