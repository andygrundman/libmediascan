#include <libmediascan.h>

#include "tap.h"
#include "common.h"
#include <unistd.h>

#define TEST_COUNT 22

static int rcount = 0;

static void my_result_callback(MediaScan *s, MediaScanResult *result, void *userdata) {
  //ms_dump_result(result);
  rcount++;
}

static void my_error_callback(MediaScan *s, MediaScanError *error, void *userdata) {
  LOG_WARN("[Error] %s (%s)\n", error->error_string, error->path);
}

static void my_progress_callback(MediaScan *s, MediaScanProgress *progress, void *userdata) {
  // Do tests on final progress callback only
  if (!progress->cur_item) {
    ok(progress->dir_total == 13, "final progress callback dir_total is %d", progress->dir_total);
    ok(progress->file_total == 30, "final progress callback file_total is %d", progress->file_total);
    ok(rcount == 18, "final result callback count is %d", rcount);
  }
}
  
int
main(int argc, char *argv[])
{
  plan(TEST_COUNT);
  
  ms_set_log_level(ERROR);
  
  // Get path to this binary
  char *bin = _findbin(argv[0]);
  char *dir = _abspath(bin, "../data"); // because binary is in .libs dir

  // Test all API functions
  {
    MediaScan *s = ms_create();
    ok(s->npaths == 0, "ms_create s->npaths == 0");
    ok(s->paths[0] == NULL, "ms_create s->paths == NULL");
    ok(s->nignore_exts == 0, "ms_create s->nignore_exts == 0");
    ok(s->ignore_exts[0] == NULL, "ms_create s->ignore_exts == NULL");
    ok(s->async == 0, "ms_create s->async == 0");
    ok(s->on_result == NULL, "ms_create s->on_result == NULL");
    ok(s->on_error == NULL, "ms_create s->on_error == NULL");
    ok(s->on_progress == NULL, "ms_create s->on_progress == NULL");
    
    ms_add_path(s, dir);
    ok(s->npaths == 1, "ms_add_path s->npaths == 1");
    is(s->paths[0], dir, "ms_add_path s->paths[0] is %s", dir);
    
    ms_add_ignore_extension(s, "mp3");
    ms_add_ignore_extension(s, "mp4");
    ok(s->nignore_exts == 2, "ms_add_ignore_extension s->nignore_exts == 2");
    is(s->ignore_exts[0], "mp3", "ms_add_ignore_extension s->ignore_exts[0] is mp3");
    is(s->ignore_exts[1], "mp4", "ms_add_ignore_extension s->ignore_exts[1] is mp4");
    
    ms_set_async(s, 1);
    ok(s->async == 1, "ms_set_async s->async == 1");
    
    ms_set_async(s, 0);
    ok(s->async == 0, "ms_set_async s->async == 0");
    
    ms_set_result_callback(s, my_result_callback);
    ok(s->on_result == my_result_callback, "ms_set_result_callback s->on_result ok");
    
    ms_set_error_callback(s, my_error_callback);
    ok(s->on_error == my_error_callback, "ms_set_error_callback s->on_error ok");
    
    ms_set_progress_callback(s, my_progress_callback);
    ms_set_progress_interval(s, 60);
    ok(s->progress_interval == 60, "ms_set_progress_interval s->progress_interval ok");
    ok(s->on_progress == my_progress_callback, "ms_set_progress_callback s->on_progress ok");
    
    ms_scan(s);
    
    ms_destroy(s);
  }
  
  free(dir);
  free(bin);
  
  return exit_status();
}
