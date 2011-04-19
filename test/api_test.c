///-------------------------------------------------------------------------------------------------
// file:	api_test.c
//
// summary:	API test functionality
///-------------------------------------------------------------------------------------------------

#include <libmediascan.h>

#include "tap.h"
#include "common.h"

#define TEST_COUNT 21

static int rcount = 0;


///-------------------------------------------------------------------------------------------------
///  Called with a result.
///
/// @author Henry Bennett
/// @date 03/15/2011
///
/// @param [in,out] s   If non-null, the.
/// @param [out] result if non-null, the result.
///
/// ### remarks .
///-------------------------------------------------------------------------------------------------

static void my_result_callback(MediaScan *s, MediaScanResult *result, void *userdata) {
  //ms_dump_result(result);
  rcount++;
}

///-------------------------------------------------------------------------------------------------
///  Called with progress update.
///
/// @author Henry Bennett
/// @date 03/15/2011
///
/// @param [in,out] s		 If non-null, the.
/// @param [in,out] progress If non-null, the progress.
///
/// ### remarks .
///-------------------------------------------------------------------------------------------------

static void my_error_callback(MediaScan *s, MediaScanError *error, void *userdata) {
  LOG_WARN("[Error] %s (%s)\n", error->error_string, error->path);
}

static void my_progress_callback(MediaScan *s, MediaScanProgress *progress, void *userdata) {
  // Do tests on final progress callback only
  if (!progress->cur_item) {
    ok(progress->total == 35, "final progress callback total is %d", progress->total);
    ok(rcount == 23, "final result callback count is %d", rcount);
  }
} /* my_progress_callback() */

static void my_result_callback2(MediaScan *s, MediaScanResult *result, void *userdata) {

	if(!result->mime_type)
		ms_dump_result(result);
}

void check_mimetypes() {
	long start_count = 0,
		end_count = 0;
	#ifndef WIN32
  char *bin = NULL;
#endif

  char *dir = NULL;
  MediaScan *s = NULL;

	start_count = GetTickCount();

#ifdef WIN32
//  dir = _abspath(bin, "data\\video\\dlna"); // because binary is in .libs dir
  dir = strdup("G:\\Movies");		// Movies: avi. wmv
//  dir = strdup("D:\\Music");	// Music: mp3, flac, wma
//  dir = strdup("F:\\");				// TV Shows: avi, mkv, avi
//  dir = strdup("D:\\Anime");	// Anime: mkv, avi (subtitles)
#else
  // Get path to this binary
  bin = _findbin(argv[0]);
  dir = _abspath(bin, "../data/video/dlna"); // because binary is in .libs dir
#endif
  
  s = ms_create();
  ms_add_path(s, dir);
  ms_set_result_callback(s, my_result_callback2);
  ms_set_error_callback(s, my_error_callback);
  ms_scan(s);    
  ms_destroy(s);
  
  free(dir);

#ifndef WIN32
  free(bin);
#endif

	end_count = GetTickCount();

	printf("------------------------------------------------------\nTotal MS %d\n", end_count - start_count);

}

///-------------------------------------------------------------------------------------------------
///  Main entry-point for this application.
///
/// @author Henry Bennett
/// @date 03/15/2011
///
/// @param argc Number of command-line arguments.
/// @param argv Array of command-line argument strings.
///
/// @return Exit-code for the process - 0 for success, else an error code.
///
/// ### remarks .
///-------------------------------------------------------------------------------------------------

int main(int argc, char *argv[])
{
	/*
  char *bin;
  char *dir;

  plan(TEST_COUNT);  
  ms_set_log_level(ERR);

  // Get path to this binary
  bin = _findbin(argv[0]);
  dir = _abspath(bin, "../data"); // because binary is in .libs dir

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
    ok(s->progress->interval == 60, "ms_set_progress_interval s->progress_interval ok");
    ok(s->on_progress == my_progress_callback, "ms_set_progress_callback s->on_progress ok");
    
    ms_scan(s);
    
    ms_destroy(s);
  }
  
  free(dir);
  free(bin);
  */
 
  ms_set_log_level(INFO);
  run_unit_tests();
  // 
  //check_mimetypes();



  return exit_status();
} /* main() */
