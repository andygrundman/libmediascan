// Test all DLNA video profiles

#include <libmediascan.h>

#include "tap.h"
#include "common.h"
#include <unistd.h>

#define TEST_COUNT 8


static void my_result_callback(MediaScan *s, MediaScanResult *r) {
  char *file;
  if ((file = strrchr(r->path, '/')) == NULL)
    if ((file = strrchr(r->path, '\\')) == NULL)
      return;  
  file++;
  
  if (!strcmp(file, "MPEG1.mpg")) {
    ok(r->type == TYPE_VIDEO, "MPEG1.mpg type is video ok");
    is(r->mime_type, "video/mpeg", "MPEG1.mpg MIME type video/mpeg ok");
    is(r->dlna_profile, "MPEG1", "MPEG1.mpg DLNA profile MPEG1 ok");
    ok(r->size == 51200, "MPEG1.mpg file size is 51200 ok");
    ok(r->bitrate == 1363969, "MPEG1.mpg bitrate is 1363969bps ok");
    ok(r->duration_ms == 300, "MPEG1.mpg duration is 0.3s ok");
    ok(r->type_data.video->width == 352, "MPEG1.mpg video width 352 ok");
    ok(r->type_data.video->height == 240, "MPEG1.mpg video height 240 ok");
  }
}

static void my_error_callback(MediaScan *s, MediaScanError *error) {
  LOG_ERROR("[Error] %s (%s)\n", error->error_string, error->path);
}

int
main(int argc, char *argv[])
{
  char *bin;
  char *dir;
  MediaScan *s;
  plan(TEST_COUNT);
  
  ms_set_log_level(ERR);
  
  // Get path to this binary
  bin = _findbin(argv[0]);

#ifdef WIN32
  dir = _abspath(bin, "data/video/dlna"); // because binary is in .libs dir
#else
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
