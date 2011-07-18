#include <stdio.h>
#include <string.h>


#ifdef WIN32
#include <Windows.h>
#include <direct.h>
#else
#include <fcntl.h>
#include <sys/wait.h>
#define _rmdir rmdir
#define _mkdir mkdir
#define CopyFile copyfile
#define DeleteFile deletefile
#endif

#include <limits.h>
#include <libmediascan.h>

#include "../src/mediascan.h"
#include "../src/database.h"
#include "CUnit/CUnit/Headers/Basic.h"

#ifndef MAX_PATH
#define MAX_PATH 1024
#endif

#ifndef WIN32

void Sleep(int ms)
{
	usleep(ms * 1000); // Convert to usec
} /* Sleep() */

int copyfile(char *source, char *dest, int not_used)
{
    int childExitStatus;
    pid_t pid;

    if (!source || !dest) {
        return FALSE;
    }

    pid = fork();

    if (pid == 0) { /* child */
        execl("/bin/cp", "/bin/cp", source, dest, (char *)0);
    }
    else if (pid < 0) {
        return FALSE;
    }
    else {
        /* parent - wait for child - this has all error handling, you
         * could just call wait() as long as you are only expecting to
         * have one child process at a time.
         */
        pid_t ws = waitpid( pid, &childExitStatus, WNOHANG);
        if (ws == -1)
        { 
        return FALSE;
        }

        if( WIFEXITED(childExitStatus)) /* exit code in childExitStatus */
        {
            int status = WEXITSTATUS(childExitStatus); /* zero is normal exit */
            /* handle non-zero as you wish */
        }
    }
    return TRUE;
} /* copyfile() */

int deletefile(char *source)
{
    int childExitStatus;
    pid_t pid;

    if (!source) {
        return FALSE;
    }

    pid = fork();

    if (pid == 0) { /* child */
        execl("/bin/rm", "/bin/rm", source, (char *)0);
    }
    else if (pid < 0) {
        return FALSE;
    }
    else {
        /* parent - wait for child - this has all error handling, you
         * could just call wait() as long as you are only expecting to
         * have one child process at a time.
         */
        pid_t ws = waitpid( pid, &childExitStatus, WNOHANG);
        if (ws == -1)
        { 
        return FALSE;
        }

        if( WIFEXITED(childExitStatus)) /* exit code in childExitStatus */
        {
            int status = WEXITSTATUS(childExitStatus); /* zero is normal exit */
            /* handle non-zero as you wish */
        }
    }
    return TRUE;
} /* deletefile() */

#endif

static int result_called = 0;
static int finish_called = 0;
static MediaScanResult result;

static void my_result_callback(MediaScan *s, MediaScanResult *r, void *userdata) {

	result.type = r->type;
	result.path = strdup(r->path);
	result.flags = r->flags;

	fprintf(stderr, "my_result_callback for %s\n", result.path);

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

	result_called++;
} /* my_result_callback() */

static void my_error_callback(MediaScan *s, MediaScanError *error, void *userdata) { 

} /* my_error_callback() */

static void my_finish_callback(MediaScan *s, void *userdata) { 
  fprintf(stderr, "finish_callback\n");
	finish_called = TRUE;
} /* my_finish_callback() */




///-------------------------------------------------------------------------------------------------
///  Test background api
///
/// @author Henry Bennett
/// @date 03/18/2011
///-------------------------------------------------------------------------------------------------
#define MAKE_PATH(str, path, file)  	{ strcpy((str), (path)); strcat((str), "\\"); strcat((str), (file)); }

static void PathCopyFile(const char *file, const char *src_path, const char *dest_path) 
{
	char src[MAX_PATH];
	char dest[MAX_PATH];

	MAKE_PATH(src, src_path, file);
	MAKE_PATH(dest, dest_path, file);

	printf("Copying %s to %s\n", src, dest);
	CopyFile(src, dest, FALSE);
}

static void test_background_api(void)	{
	const char *test_path = "C:\\Siojej3";
	const char *data_path = "data\\video";
	const char *data_file1 = "bars-mpeg1video-mp2.mpg";
	const char *data_file2 = "bars-msmpeg4-mp2.asf";
	const char *data_file3 = "bars-msmpeg4v2-mp2.avi";
	const char *data_file4 = "bars-vp8-vorbis.webm";
	const char *data_file5 = "wmv92-with-audio.wmv";
//	char src[MAX_PATH];
	char dest[MAX_PATH];

	MediaScan *s = ms_create();

	CU_ASSERT_FATAL(s != NULL);

	// Do some setup for the test
	CU_ASSERT( _mkdir(test_path) != -1 );
	result_called = 0;
	
	CU_ASSERT(s->on_result == NULL);
	ms_set_result_callback(s, my_result_callback);
	CU_ASSERT(s->on_result == my_result_callback);

	CU_ASSERT(s->on_error == NULL);
	ms_set_error_callback(s, my_error_callback); 
	CU_ASSERT(s->on_error == my_error_callback);

	ms_watch_directory(s, test_path);
	CU_ASSERT( result_called == 0 );
	Sleep(1000); // Sleep 1 second
	CU_ASSERT( result_called == 0 );
	
	// Now copy a small video file to the test directory
	PathCopyFile(data_file1, data_path, test_path );

	CU_ASSERT( result_called == 0 );
	Sleep(1000); // Sleep 1 second

	// Now process the callbacks
	ms_async_process(s);
	CU_ASSERT( result_called == 1 );
	
	result_called = 0;

	PathCopyFile(data_file2, data_path, test_path );
	Sleep(2000); // Sleep 1 second

	// Now process the callbacks
	ms_async_process(s);
	CU_ASSERT( result_called == 1 );
	
	reset_bdb(s);
	result_called = 0;

	MAKE_PATH(dest, test_path, data_file1);
	printf("Deleting %s\n", dest);
	CU_ASSERT( DeleteFile(dest) == TRUE);
	Sleep(1500); // Sleep 500 milliseconds
	MAKE_PATH(dest, test_path, data_file2);
	printf("Deleting %s\n", dest);
	CU_ASSERT( DeleteFile(dest) == TRUE);
	Sleep(1500); // Sleep 500 milliseconds

	PathCopyFile(data_file1, data_path, test_path );
	Sleep(500); // Sleep 500 milliseconds
	PathCopyFile(data_file2, data_path, test_path );
	Sleep(1500); // Sleep 500 milliseconds
	PathCopyFile(data_file3, data_path, test_path );
	Sleep(500); // Sleep 500 milliseconds
	PathCopyFile(data_file4, data_path, test_path );
	Sleep(100); // Sleep 500 milliseconds
	PathCopyFile(data_file5, data_path, test_path );
	Sleep(500); // Sleep 500 milliseconds

	// Now process the callbacks
	ms_async_process(s);
	CU_ASSERT( result_called == 5 );

	ms_destroy(s);

	// Clean up the test
	MAKE_PATH(dest, test_path, data_file1);
	DeleteFile(dest);
	MAKE_PATH(dest, test_path, data_file2);
	DeleteFile(dest);
	MAKE_PATH(dest, test_path, data_file3);
	DeleteFile(dest);
	MAKE_PATH(dest, test_path, data_file4);
	DeleteFile(dest);
	MAKE_PATH(dest, test_path, data_file5);
	DeleteFile(dest);

	CU_ASSERT( _rmdir(test_path) != -1 );

} /* test_background_api() */

static void test_background_api2(void)	{
	const char *test_path = "C:\\4oij3";
	const char *data_path = "data\\video";
	const char *data_file1 = "bars-mpeg1video-mp2.mpg";
	const char *data_file2 = "bars-msmpeg4-mp2.asf";
	const char *data_file3 = "bars-msmpeg4v2-mp2.avi";
	const char *data_file4 = "bars-vp8-vorbis.webm";
	const char *data_file5 = "wmv92-with-audio.wmv";
//	char src[MAX_PATH];
	char dest[MAX_PATH];

	MediaScan *s = ms_create();

	CU_ASSERT_FATAL(s != NULL);

	// Do some setup for the test
	CU_ASSERT( _mkdir(test_path) != -1 );
	result_called = 0;
	
	CU_ASSERT(s->on_result == NULL);
	ms_set_result_callback(s, my_result_callback);
	CU_ASSERT(s->on_result == my_result_callback);

	CU_ASSERT(s->on_error == NULL);
	ms_set_error_callback(s, my_error_callback); 
	CU_ASSERT(s->on_error == my_error_callback);

	ms_watch_directory(s, test_path);
	Sleep(1000); // Sleep 1 second

	// Now copy a small video file to the test directory
	PathCopyFile(data_file1, data_path, test_path );
	CU_ASSERT( result_called == 0 );


	// Now process the callbacks
	ms_async_process(s);
	CU_ASSERT( result_called == 1 );

	MAKE_PATH(dest, test_path, data_file1);
	CU_ASSERT( DeleteFile(dest) == TRUE);


	ms_destroy(s);

	// Clean up the test
	CU_ASSERT( _rmdir(test_path) != -1 );

} /* test_background_api2() */


static void test_background_api3(void)	{
	const char *test_path = "\\\\magento\\share";
	const char *test_path2 = "C:\\4o34ij3";
	const char *test_path3 = "Z:\\";
	const char *test_path4 = "C:\\data";

	MediaScan *s = ms_create();

	CU_ASSERT_FATAL(s != NULL);

	// Do some setup for the test
	result_called = 0;
	ms_errno = 0;
	CU_ASSERT( _mkdir(test_path2) != -1 );

	CU_ASSERT(s->on_result == NULL);
	ms_set_result_callback(s, my_result_callback);
	CU_ASSERT(s->on_result == my_result_callback);

	CU_ASSERT(s->on_error == NULL);
	ms_set_error_callback(s, my_error_callback); 
	CU_ASSERT(s->on_error == my_error_callback);

	ms_watch_directory(s, test_path);
	CU_ASSERT(ms_errno == MSENO_ILLEGALPARAMETER); // If we got this errno, then we got the failure we wanted

	// Test a directory that looks like a mapped network drive but isn't
	ms_errno = 0;
	ms_watch_directory(s, test_path2);
	CU_ASSERT(ms_errno == 0); 

	// Now test a mapped network drive
	ms_errno = 0;
	ms_watch_directory(s, test_path3);
	CU_ASSERT(ms_errno == MSENO_ILLEGALPARAMETER); 

	// Now test a NTFS mounted folder
//	ms_errno = 0;
//	ms_watch_directory(s, test_path4);
//	CU_ASSERT(ms_errno == 0); 


	ms_destroy(s);

	// Clean up the test
	CU_ASSERT( _rmdir(test_path2) != -1 );
} /* test_background_api3() */

static void test_win32_shortcuts(void)	{
	const char *test_path = "data\\video\\shortcuts";

	MediaScan *s = ms_create();

	CU_ASSERT_FATAL(s != NULL);

	// Do some setup for the test
	result_called = 0;
	ms_errno = 0;

	CU_ASSERT(s->on_result == NULL);
	ms_set_result_callback(s, my_result_callback);
	CU_ASSERT(s->on_result == my_result_callback);

	CU_ASSERT(s->on_error == NULL);
	ms_set_error_callback(s, my_error_callback); 
	CU_ASSERT(s->on_error == my_error_callback);

	CU_ASSERT(s->npaths == 0);
	ms_add_path(s, test_path);
	CU_ASSERT(s->npaths == 1);

	ms_scan(s);
	CU_ASSERT( result_called == 1 );

	ms_destroy(s);

} /* test_win32_shortcuts() */

static void test_win32_shortcut_recursion(void)	{
	const char *test_path1 = "data\\recursion";
	const char *test_path2 = "data\\recursion2";
	const char *test_path3 = "data\\recursion3";
	MediaScan *s;

	s = ms_create();

	CU_ASSERT_FATAL(s != NULL);

	/* Test #1 of the initial scan functionality */
	ms_set_flags(s, MS_RESCAN | MS_CLEARDB);

	// Do some setup for the test
	result_called = 0;
	ms_errno = 0;

	CU_ASSERT(s->on_result == NULL);
	ms_set_result_callback(s, my_result_callback);
	CU_ASSERT(s->on_result == my_result_callback);

	CU_ASSERT(s->on_error == NULL);
	ms_set_error_callback(s, my_error_callback); 
	CU_ASSERT(s->on_error == my_error_callback);

	CU_ASSERT(s->npaths == 0);
	ms_add_path(s, test_path1);
	CU_ASSERT(s->npaths == 1);

	ms_scan(s);
	CU_ASSERT( result_called == 1 );

	ms_destroy(s);

	/* Test #2 of the rescan functionality */

	s = ms_create();
	CU_ASSERT_FATAL(s != NULL);

	ms_set_flags(s, MS_FULL_SCAN);

	// Do some setup for the test
	result_called = 0;
	ms_errno = 0;

	CU_ASSERT(s->on_result == NULL);
	ms_set_result_callback(s, my_result_callback);
	CU_ASSERT(s->on_result == my_result_callback);

	CU_ASSERT(s->on_error == NULL);
	ms_set_error_callback(s, my_error_callback); 
	CU_ASSERT(s->on_error == my_error_callback);

	CU_ASSERT(s->npaths == 0);
	ms_add_path(s, test_path1);
	CU_ASSERT(s->npaths == 1);

	ms_scan(s);
	CU_ASSERT( result_called == 1 );

	ms_destroy(s);

	/* Test #3 of a different directory structure */

	s = ms_create();
	CU_ASSERT_FATAL(s != NULL);

	ms_set_flags(s, MS_FULL_SCAN);

	// Do some setup for the test
	result_called = 0;
	ms_errno = 0;

	CU_ASSERT(s->on_result == NULL);
	ms_set_result_callback(s, my_result_callback);
	CU_ASSERT(s->on_result == my_result_callback);

	CU_ASSERT(s->on_error == NULL);
	ms_set_error_callback(s, my_error_callback); 
	CU_ASSERT(s->on_error == my_error_callback);

	CU_ASSERT(s->npaths == 0);
	ms_add_path(s, test_path2);
	CU_ASSERT(s->npaths == 1);

	ms_scan(s);
	CU_ASSERT( result_called == 3 );

	ms_destroy(s);

	/* Test #4 of a different directory structure */

	s = ms_create();
	CU_ASSERT_FATAL(s != NULL);

	ms_set_flags(s, MS_FULL_SCAN);

	// Do some setup for the test
	result_called = 0;
	ms_errno = 0;

	CU_ASSERT(s->on_result == NULL);
	ms_set_result_callback(s, my_result_callback);
	CU_ASSERT(s->on_result == my_result_callback);

	CU_ASSERT(s->on_error == NULL);
	ms_set_error_callback(s, my_error_callback); 
	CU_ASSERT(s->on_error == my_error_callback);

	CU_ASSERT(s->npaths == 0);
	ms_add_path(s, test_path3);
	CU_ASSERT(s->npaths == 1);

	ms_scan(s);
	CU_ASSERT( result_called == 2 );

	ms_destroy(s);

} /* test_win32_shortcut_recursion() */

static void test_juke(void)	{
	const char *test_path1 = "E:\\workspace\\JUKE Test Media\\Music";
	const char *test_path2 = "E:\\workspace\\JUKE Test Media\\Pictures";
	const char *test_path3 = "E:\\workspace\\JUKE Test Media\\Videos";
	MediaScan *s;

	s = ms_create();

	CU_ASSERT_FATAL(s != NULL);

	/* Test #1 of the initial scan functionality */
	ms_set_flags(s, MS_RESCAN | MS_CLEARDB);
	ms_set_log_level(DEBUG);
	// Do some setup for the test
	result_called = 0;
	ms_errno = 0;

	CU_ASSERT(s->on_result == NULL);
	ms_set_result_callback(s, my_result_callback);
	CU_ASSERT(s->on_result == my_result_callback);

	CU_ASSERT(s->on_error == NULL);
	ms_set_error_callback(s, my_error_callback); 
	CU_ASSERT(s->on_error == my_error_callback);

	CU_ASSERT(s->npaths == 0);
	ms_add_path(s, test_path1);
	ms_add_path(s, test_path2);
	ms_add_path(s, test_path3);
	CU_ASSERT(s->npaths == 3);

	ms_scan(s);
	CU_ASSERT( result_called == 1 );

	ms_destroy(s);


} /* test_juke() */

static void test_juke_bad_file(void)	{
//	const char *test_path1 = "E:\\workspace\\JUKE Test Media\\Videos\\MP4 Test Videos\\TChaseMPEG4.mp4";
	const char *test_path1 = "data\\video\\TChaseMPEG4.mp4";
	MediaScan *s;

	s = ms_create();

	CU_ASSERT_FATAL(s != NULL);

	/* Test #1 of the initial scan functionality */
	ms_set_flags(s, MS_RESCAN | MS_CLEARDB);
	ms_set_log_level(DEBUG);
	// Do some setup for the test
	result_called = 0;
	ms_errno = 0;

	CU_ASSERT(s->on_result == NULL);
	ms_set_result_callback(s, my_result_callback);
	CU_ASSERT(s->on_result == my_result_callback);

	CU_ASSERT(s->on_error == NULL);
	ms_set_error_callback(s, my_error_callback); 
	CU_ASSERT(s->on_error == my_error_callback);

	ms_scan_file(s, test_path1, TYPE_VIDEO); 
	CU_ASSERT( result_called == 1 );

	ms_destroy(s);


} /* test_juke_bad_file() */


static void test_linux_shortcuts(void)	{
	const char *test_path = "data/video/linuxshortcuts";
	
	// file is bars-mpeg4-aac.m4v
	const char *test_file1 = "data/video/linuxshortcuts/dlna_abs.symlink";
	const char *test_file2 = "data/video/linuxshortcuts/dlna_rel.symlink";
	const char *test_file3 = "data/video/linuxshortcuts/MPEG_POS_NTSC-ac3_abs.symlink";
	const char *test_file4 = "data/video/linuxshortcuts/MPEG_POS_NTSC-ac3_rel.symlink";
	const char *dest_test_file1 = "data/video/dnla/MPEG_PS_NTSC-ac3.mpg";
	char out_path[MAX_PATH];

	MediaScan *s = ms_create();

	CU_ASSERT_FATAL(s != NULL);

	// Do some setup for the test
	result_called = 0;
	ms_errno = 0;

	CU_ASSERT(s->on_result == NULL);
	ms_set_result_callback(s, my_result_callback);
	CU_ASSERT(s->on_result == my_result_callback);

	CU_ASSERT(s->on_error == NULL);
	ms_set_error_callback(s, my_error_callback); 
	CU_ASSERT(s->on_error == my_error_callback);

	CU_ASSERT(s->npaths == 0);
	//ms_add_path(s, test_file1);
	CU_ASSERT(s->npaths == 1);

	ms_scan(s);
	CU_ASSERT( result_called == 5 );
	
	ms_destroy(s);
	s = ms_create();

	CU_ASSERT_FATAL(s != NULL);

	// Do some setup for the test
	result_called = 0;
	ms_errno = 0;

	CU_ASSERT(s->on_result == NULL);
	ms_set_result_callback(s, my_result_callback);
	CU_ASSERT(s->on_result == my_result_callback);

	CU_ASSERT(s->on_error == NULL);
	ms_set_error_callback(s, my_error_callback); 
	CU_ASSERT(s->on_error == my_error_callback);
/*
	CU_ASSERT(s->npaths == 0);
	ms_add_path(s, test_file3);
	CU_ASSERT(s->npaths == 1);

	ms_scan(s);
	CU_ASSERT( result_called == 5 );	
*/
//	if(isAlias(test_file1))
//		printf("%s is a link\n", test_file1);
//	if(isAlias(test_file2))
//		printf("%s is a link\n", test_file2);
//	if(isAlias(test_file3))
//		printf("%s is a link\n", test_file3);
//	if(isAlias(test_file4))
//		printf("%s is a link\n", test_file4);
//	CheckMacAlias(test_file4, out_path);
//	printf("Points to %s\n", out_path);

	result_called = 0;
	ms_scan_file(s, test_file3, TYPE_UNKNOWN);
	CU_ASSERT( result_called == 1 );

	//reset_bdb(s);
	//result_called = 0;
	//ms_scan_file(s, test_file4, TYPE_UNKNOWN);
	//CU_ASSERT( result_called == 1 );

	ms_destroy(s);

} /* test_linux_shortcuts() */

static void test_mac_shortcuts(void)	{
	const char *test_path = "data/video/macshortcuts";
	
	const char *test_file1 = "data/video/macshortcuts/avi_alias";
	const char *test_file2 = "data/video/macshortcuts/dlna";
	const char *test_file3 = "data/video/macshortcuts/dnla_link1";
	const char *test_file4 = "data/video/macshortcuts/bars-mpeg4-mp2.avi";
	const char *test_file5 = "data/video/macshortcuts/dnla_link2";
	const char *test_file6 = "data/video/macshortcuts/bars-mpeg4-mp2.avi";
	const char *test_file7 = "data/video/macshortcuts/avi_link";
	const char *dest_test_file4 = "/Users/Fox/workspace/libmediascan/test/data/video/bars-mpeg4-mp2.avi";
	char out_path[MAX_PATH];

	MediaScan *s = ms_create();
		ms_set_log_level(DEBUG);
	CU_ASSERT_FATAL(s != NULL);

	// Do some setup for the test
	result_called = 0;
	ms_errno = 0;

	CU_ASSERT(s->on_result == NULL);
	ms_set_result_callback(s, my_result_callback);
	CU_ASSERT(s->on_result == my_result_callback);

	CU_ASSERT(s->on_error == NULL);
	ms_set_error_callback(s, my_error_callback); 
	CU_ASSERT(s->on_error == my_error_callback);

	CU_ASSERT(s->npaths == 0);
	ms_add_path(s, test_file2);
	CU_ASSERT(s->npaths == 1);

	ms_scan(s);
	CU_ASSERT( result_called == 5 );
	
	ms_destroy(s);
	s = ms_create();

	CU_ASSERT_FATAL(s != NULL);

	// Do some setup for the test
	result_called = 0;
	ms_errno = 0;

	CU_ASSERT(s->on_result == NULL);
	ms_set_result_callback(s, my_result_callback);
	CU_ASSERT(s->on_result == my_result_callback);

	CU_ASSERT(s->on_error == NULL);
	ms_set_error_callback(s, my_error_callback); 
	CU_ASSERT(s->on_error == my_error_callback);

	CU_ASSERT(s->npaths == 0);
	ms_add_path(s, test_file5);
	CU_ASSERT(s->npaths == 1);

	ms_scan(s);
	CU_ASSERT( result_called == 5 );	

	//if(isAlias(test_file6))
	//	printf("%s is a link\n", test_file6);
	//ms_scan_file(s, test_file6, TYPE_UNKNOWN);

	if(isAlias(test_file1))
		printf("%s is a link\n", test_file1);
	ms_scan_file(s, test_file1, TYPE_UNKNOWN);

	if(isAlias(test_file7))
		printf("%s is a link\n", test_file7);
	ms_scan_file(s, test_file7, TYPE_UNKNOWN);
//	if(isAlias(test_file2))
//		printf("%s is a link\n", test_file2);
//	if(isAlias(test_file3))
//		printf("%s is a link\n", test_file3);
//	if(isAlias(test_file4))
//		printf("%s is a link\n", test_file4);
//	CheckMacAlias(test_file4, out_path);
//	printf("Points to %s\n", out_path);

//	ms_scan_file(s, test_file4, TYPE_UNKNOWN);

	ms_destroy(s);

} /* test_mac_shortcuts() */

static void test_async_api(void)	{

  long time1, time2;

	#ifdef WIN32
	const char dir[MAX_PATH] = "data\\video\\dlna";
	#else
	const char dir[MAX_PATH] = "data/video/dlna";
  	struct timeval now;
	#endif

	MediaScan *s = ms_create();

	CU_ASSERT(s->npaths == 0);
	ms_add_path(s, dir);
	CU_ASSERT(s->npaths == 1);

	CU_ASSERT( s->async == FALSE );
	ms_set_async(s, FALSE);
	CU_ASSERT( s->async == FALSE );

	CU_ASSERT(s->on_result == NULL);
	ms_set_result_callback(s, my_result_callback);
	CU_ASSERT(s->on_result == my_result_callback);

	CU_ASSERT(s->on_error == NULL);
	ms_set_error_callback(s, my_error_callback); 
	CU_ASSERT(s->on_error == my_error_callback);

	ms_scan(s);
	CU_ASSERT( result_called == 5 );

	result_called = 0;
	reset_bdb(s);


	CU_ASSERT( s->async == FALSE );
	ms_set_async(s, TRUE);
	CU_ASSERT( s->async == TRUE );

#ifdef WIN32
  time1 = GetTickCount();
#else
  gettimeofday(&now, NULL);
  time1 = now.tv_sec;
#endif

	ms_scan(s);
	CU_ASSERT( result_called == 0 );

#ifdef WIN32
  time2 = GetTickCount();
#else
  gettimeofday(&now, NULL);
  time2 = now.tv_sec;
#endif

	// Verify that the function returns almost immediately
	CU_ASSERT( time2 - time1 < 20 );

	Sleep(1000); // Sleep 1 second

	// Now process the callbacks
	ms_async_process(s);
	CU_ASSERT( result_called == 5 );

	ms_destroy(s);
} /* test_async_api() */

#ifdef WIN32

/* (adapted from Perl) select contributed by Vincent R. Slyngstad (vrs@ibeam.intel.com) */
#define SOCKET_TEST(x, y) \
    do {					\
	if((x) == (y))					\
	    errno = WSAGetLastError();			\
    } while (0)

#define SOCKET_TEST_ERROR(x) SOCKET_TEST(x, SOCKET_ERROR)
#define TO_SOCKET(x) _get_osfhandle(x)

static int
win32_select(int nfds, fd_set* rd, fd_set* wr, fd_set* ex, const struct timeval* timeout)
{
    int r;
    int i, fd, save_errno = errno;
    FD_SET nrd, nwr, nex;
    bool just_sleep = TRUE;

    FD_ZERO(&nrd);
    FD_ZERO(&nwr);
    FD_ZERO(&nex);
    for (i = 0; i < nfds; i++) {
		if (rd && FD_ISSET(i,rd)) {
			fd = TO_SOCKET(i);
			FD_SET((unsigned)fd, &nrd);
			just_sleep = FALSE;
		}
		if (wr && FD_ISSET(i,wr)) {
			fd = TO_SOCKET(i);
			FD_SET((unsigned)fd, &nwr);
			just_sleep = FALSE;
		}
		if (ex && FD_ISSET(i,ex)) {
			fd = TO_SOCKET(i);
			FD_SET((unsigned)fd, &nex);
			just_sleep = FALSE;
		}
    }

    /* winsock seems incapable of dealing with all three fd_sets being empty,
     * so do the (millisecond) sleep as a special case
     */
    if (just_sleep) {
		if (timeout)
			Sleep(timeout->tv_sec  * 1000 +
			  timeout->tv_usec / 1000);	/* do the best we can */
		else
			Sleep(UINT_MAX);
		return 0;
    }

    errno = save_errno;
    SOCKET_TEST_ERROR(r = select(nfds, &nrd, &nwr, &nex, timeout));
    save_errno = errno;

    for (i = 0; i < nfds; i++) {
		if (rd && FD_ISSET(i,rd)) {
			fd = TO_SOCKET(i);
			if (!FD_ISSET(fd, &nrd)) 
				FD_CLR(i,rd);
		}
		if (wr && FD_ISSET(i,wr)) {
			fd = TO_SOCKET(i);
			if (!FD_ISSET(fd, &nwr))
				FD_CLR(i,wr);
		}
		if (ex && FD_ISSET(i,ex)) {
			fd = TO_SOCKET(i);
			if (!FD_ISSET(fd, &nex))
				FD_CLR(i,ex);
		}
    }
    errno = save_errno;
    return r;
}

static void test_async_api2(void)	{

  long time1, time2;
  int fd;
  FD_SET rd;
  struct timeval timeout;

#ifdef WIN32
	const char dir[MAX_PATH] = "C:\\Documents and Settings\\Administrator\\My Documents\\My Pictures";
#else
	const char dir[MAX_PATH] = "/Users/andy/Pictures";
#endif

	MediaScan *s = ms_create();
	ms_set_log_level(DEBUG);
	CU_ASSERT(s->npaths == 0);
	ms_add_path(s, dir);
	CU_ASSERT(s->npaths == 1);

	CU_ASSERT(s->on_result == NULL);
	ms_set_result_callback(s, my_result_callback);
	CU_ASSERT(s->on_result == my_result_callback);

	CU_ASSERT(s->on_error == NULL);
	ms_set_error_callback(s, my_error_callback); 
	CU_ASSERT(s->on_error == my_error_callback);

	CU_ASSERT(s->on_finish == NULL);
	ms_set_finish_callback(s, my_finish_callback); 
	CU_ASSERT(s->on_finish == my_finish_callback);


	CU_ASSERT( s->async == FALSE );
	ms_set_async(s, TRUE);
	CU_ASSERT( s->async == TRUE );

	//ms_set_cachedir(s, "\\tmp\\libmediascan");
	ms_set_flags(s, MS_USE_EXTENSION);

	finish_called = 0;
	ms_scan(s);
	CU_ASSERT( result_called == 0 );

	while (!finish_called) {
    // Use select() from Perl's win32 code
	  fd = ms_async_fd(s);
	  FD_ZERO(&rd);
	  FD_SET(fd, &rd);
	  timeout.tv_sec = 10;
	  timeout.tv_usec = 0;

		fprintf(stderr, "** waiting in win32_select for fd %d\n", fd);
		win32_select(fd + 1, &rd, NULL, NULL, &timeout);
		fprintf(stderr, "** select returned, fd is set? %d\n", FD_ISSET(fd, &rd) ? 1 : 0);
		if (FD_ISSET(fd, &rd)) {
			ms_async_process(s);
      fprintf(stderr, "** async_process returned\n");
		}
	}

	CU_ASSERT( result_called == 8 );

	ms_destroy(s);
} /* test_async_api2() */

#endif

///-------------------------------------------------------------------------------------------------
///  Setup background tests.
///
/// @author Henry Bennett
/// @date 03/22/2011
///-------------------------------------------------------------------------------------------------

int setupbackground_tests() {
	CU_pSuite pSuite = NULL;


   /* add a suite to the registry */
   pSuite = CU_add_suite("Background Scanning", NULL, NULL);
   if (NULL == pSuite) {
      CU_cleanup_registry();
      return CU_get_error();
   }

   /* add the tests to the background scanning suite */
   if (
//   NULL == CU_add_test(pSuite, "Test background scanning API", test_background_api) 
//   NULL == CU_add_test(pSuite, "Test background scanning Deletion", test_background_api2) 
#if defined(WIN32)
    NULL == CU_add_test(pSuite, "Test Async scanning API 2", test_async_api2) 
#endif
//   NULL == CU_add_test(pSuite, "Test edge cases of background scanning API", test_background_api3) 
//   NULL == CU_add_test(pSuite, "Test Juke DB", test_juke) 
//   NULL == CU_add_test(pSuite, "Test a bad file in Juke", test_juke_bad_file)    
#if defined(WIN32)
//   NULL == CU_add_test(pSuite, "Test Win32 shortcuts", test_win32_shortcuts) 
   //NULL == CU_add_test(pSuite, "Test Win32 shortcut infinite recursion", test_win32_shortcut_recursion) 
#elif defined(__linux__)
   NULL == CU_add_test(pSuite, "Test Linux shortcuts", test_linux_shortcuts) 
#else
   NULL == CU_add_test(pSuite, "Test Mac shortcuts", test_mac_shortcuts) 
#endif
	   )
   {
      CU_cleanup_registry();
      return CU_get_error();
   }

   return 0;

} /* setupbackground_tests() */
