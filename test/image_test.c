#include <libmediascan.h>

#include "tap.h"
#include "common.h"
#include <unistd.h>

#define TEST_COUNT 21

static int rcount = 0;

static void my_result_callback(MediaScan *s, MediaScanResult *result, void *userdata) {
  ms_dump_result(result);
  rcount++;
}

static void my_error_callback(MediaScan *s, MediaScanError *error, void *userdata) {
  LOG_WARN("[Error] %s (%s)\n", error->error_string, error->path);
}

static void my_progress_callback(MediaScan *s, MediaScanProgress *progress, void *userdata) {
  // Do tests on final progress callback only
  if (!progress->cur_item) {
    ok(progress->total == 42, "final progress callback total is %d", progress->total);
    ok(rcount == 23, "final result callback count is %d", rcount);
  }
}

int main(int argc, char *argv[])
{
  char *bin;
  char *dir;

  plan(TEST_COUNT);  
  ms_set_log_level(INFO);

  // Get path to this binary
  bin = _findbin(argv[0]);
  dir = _abspath(bin, "../data"); // because binary is in .libs dir

  // Scan all image files
  {
    MediaScan *s = ms_create();
    ms_add_path(s, dir);    
    ms_add_ignore_extension(s, "VIDEO");
    ms_add_ignore_extension(s, "AUDIO");
    ms_set_result_callback(s, my_result_callback);
    ms_set_error_callback(s, my_error_callback);
    ms_set_progress_callback(s, my_progress_callback);
    ms_scan(s);
    ms_destroy(s);
  }
  
  free(dir);
  free(bin);
 
  return exit_status();
}