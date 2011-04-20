#include <libmediascan.h>

#include "tap.h"
#include "common.h"

#define TEST_COUNT 21

static int rcount = 0;
static int done = 0;

static void my_result_callback(MediaScan *s, MediaScanResult *result, void *userdata) {
  //ms_dump_result(result);
  rcount++;
}

static void my_error_callback(MediaScan *s, MediaScanError *error, void *userdata) {
  LOG_WARN("[Error] %s (%s)\n", error->error_string, error->path);
}

static void my_progress_callback(MediaScan *s, MediaScanProgress *progress, void *userdata) {
  LOG_WARN("[Progress] %s (%s) %d/%d, ETA %d secs\n", progress->phase, progress->cur_item, progress->done, progress->total, progress->eta);

  // Do tests on final progress callback only
  if (!progress->cur_item) {
    ok(progress->total == 42, "final progress callback total is %d", progress->total);
    ok(rcount == 23, "final result callback count is %d", rcount);
  }
}

static void my_finish_callback(MediaScan *s, void *userdata) {
  done = 1;
}

int main(int argc, char *argv[])
{
  char *bin;
  char *dir;

  plan(TEST_COUNT);  
  ms_set_log_level(WARN);

  // Get path to this binary
  bin = _findbin(argv[0]);
  //dir = _abspath(bin, "../data"); // because binary is in .libs dir
  dir = "/Users/andy/QA/DHGMedia/Pictures/GIF";

  // Scan all image files
  {
    MediaScan *s = ms_create();
    ms_add_path(s, dir);    
    ms_add_ignore_extension(s, "AUDIO");
    ms_add_ignore_extension(s, "VIDEO");
    ms_add_thumbnail_spec(s, THUMB_AUTO, 300, 0, 1, 0, 0);
    ms_set_result_callback(s, my_result_callback);
    ms_set_error_callback(s, my_error_callback);
    ms_set_progress_callback(s, my_progress_callback);
    ms_set_finish_callback(s, my_finish_callback);
    //ms_set_async(s, 1);
    ms_scan(s);

    // XXX Watch fd instead
    do {
      ms_async_process(s);
    } while (!done);

    ms_destroy(s);
  }
  
  //free(dir);
  free(bin);
 
  return exit_status();
}
