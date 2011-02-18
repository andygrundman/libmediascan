#include <libmediascan.h>

#include "tap.h"
#include "common.h"

#define TEST_COUNT 18

static void my_result_callback(MediaScan *s, MediaScanResult *result) { }

static void my_error_callback(MediaScan *s, MediaScanError *error) { }

static void my_progress_callback(MediaScan *s, MediaScanProgress *progress) { }
  
int
main(int argc, char *argv[])
{ 
  plan(TEST_COUNT);
  
  ms_set_log_level(9);
  
  // Get path to this binary
  char *bin = _findbin(argv[0]);
  
  // Test all API functions except ms_scan
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
    
    ms_add_path(s, "/foo/bar");
    ms_add_path(s, "/foo/baz");
    ok(s->npaths == 2, "ms_add_path s->npaths == 2");
    is(s->paths[0], "/foo/bar", "ms_add_path s->paths[0] is /foo/bar");
    is(s->paths[1], "/foo/baz", "ms_add_path s->paths[1] is /foo/baz");
    
    ms_add_ignore_extension(s, "wav");
    ms_add_ignore_extension(s, "mp4");
    ok(s->nignore_exts == 2, "ms_add_ignore_extension s->nignore_exts == 2");
    is(s->ignore_exts[0], "wav", "ms_add_ignore_extension s->ignore_exts[0] is wav");
    is(s->ignore_exts[1], "mp4", "ms_add_ignore_extension s->ignore_exts[1] is mp4");
    
    ms_set_async(s, 1);
    ok(s->async == 1, "ms_set_async s->async == 1");
    
    ms_set_result_callback(s, my_result_callback);
    ok(s->on_result == my_result_callback, "ms_set_result_callback s->on_result ok");
    
    ms_set_error_callback(s, my_error_callback);
    ok(s->on_error == my_error_callback, "ms_set_error_callback s->on_error ok");
    
    ms_set_progress_callback(s, my_progress_callback);
    ok(s->on_progress == my_progress_callback, "ms_set_progress_callback s->on_progress ok");
    
    ms_destroy(s);
  }
  
  free(bin);
  
  return exit_status();
}
