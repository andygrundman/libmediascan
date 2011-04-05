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
#include "Cunit/CUnit/Headers/Basic.h"

static FILE* temp_file = NULL;

#ifndef MAX_PATH
#define MAX_PATH 1024
#endif

#ifndef WIN32
void Sleep(int ms)
{
	usleep(ms * 1000); // Convert to usec
}


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
//        else if (WIFSIGNALED(status)) /* killed */
//        {
//        }
//        else if (WIFSTOPPED(status)) /* stopped */
//        {
//        }
    }
    return TRUE;
}

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
//        else if (WIFSIGNALED(status)) /* killed */
//        {
//        }
//        else if (WIFSTOPPED(status)) /* stopped */
//        {
//        }
    }
    return TRUE;
}
#endif

///-------------------------------------------------------------------------------------------------
///  ------------------------------------------------------------------------------------------
/// 	  The suite initialization function. Opens the temporary file used by the tests. Returns
/// 	  zero on success, non-zero otherwise.
///
/// @author Henry Bennett
/// @date 03/16/2011
///
/// @return .
///-------------------------------------------------------------------------------------------------

static int init_suite(void)
{
	temp_file = fopen("./temp2.txt", "w+");

   if ( temp_file == NULL) {
      return -1;
   }
   else {
      return 0;
   }
}

///-------------------------------------------------------------------------------------------------
///  ------------------------------------------------------------------------------------------
/// 	  The suite cleanup function. Closes the temporary file used by the tests. Returns zero
/// 	  on success, non-zero otherwise.
///
/// @author Henry Bennett
/// @date 03/16/2011
///
/// @return .
///-------------------------------------------------------------------------------------------------

static int clean_suite(void)
{

   if (0 != fclose(temp_file)) {
      return -1;
   }
   else {
      temp_file = NULL;
      return 0;
   }


}

static int result_called = FALSE;
static MediaScanResult result;

static void my_result_callback(MediaScan *s, MediaScanResult *r, void *userdata) {

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

static void my_error_callback(MediaScan *s, MediaScanError *error, void *userdata) { 

} /* my_error_callback() */

///-------------------------------------------------------------------------------------------------
///  Test background api
///
/// @author Henry Bennett
/// @date 03/18/2011
///-------------------------------------------------------------------------------------------------

void test_background_api(void)	{
	const char *test_path = "D:\\Siojej3";
	const char *data_path = "data\\video";
	const char *data_file1 = "bars-mpeg1video-mp2.mpg";
	char src[MAX_PATH];
	char dest[MAX_PATH];

	MediaScan *s = ms_create();


	// Do some set for the test
	CU_ASSERT( _mkdir(test_path) != -1 );
	result_called = FALSE;
	
	CU_ASSERT(s->on_result == NULL);
	ms_set_result_callback(s, my_result_callback);
	CU_ASSERT(s->on_result == my_result_callback);

	CU_ASSERT(s->on_error == NULL);
	ms_set_error_callback(s, my_error_callback); 
	CU_ASSERT(s->on_error == my_error_callback);

	ms_watch_directory(s, test_path, my_result_callback);
	CU_ASSERT( result_called == FALSE );
	Sleep(1000); // Sleep 1 second
	CU_ASSERT( result_called == FALSE );
	
	// Now copy a small video file to the test directory
	strcpy(src, data_path);
	strcat(src, "\\");
	strcat(src, data_file1);

	strcpy(dest, test_path);
	strcat(dest, "\\");
	strcat(dest, data_file1);

	CU_ASSERT( CopyFile(src, dest, FALSE) == TRUE );
	Sleep(1000); // Sleep 1 second
	CU_ASSERT( result_called == TRUE );

	ms_destroy(s);

	// Clean up the test
	CU_ASSERT( DeleteFile(dest) == TRUE);
	CU_ASSERT( _rmdir(test_path) != -1 );

} /* test_background_api() */

///-------------------------------------------------------------------------------------------------
///  Setup background tests.
///
/// @author Henry Bennett
/// @date 03/22/2011
///-------------------------------------------------------------------------------------------------

int setupbackground_tests() {
	CU_pSuite pSuite = NULL;


   /* add a suite to the registry */
   pSuite = CU_add_suite("Background Scanning", init_suite, clean_suite);
   if (NULL == pSuite) {
      CU_cleanup_registry();
      return CU_get_error();
   }

   /* add the tests to the background scanning suite */
   if (
	   NULL == CU_add_test(pSuite, "Test background scanning API", test_background_api)
	   )
   {
      CU_cleanup_registry();
      return CU_get_error();
   }

   return 0;

} /* setupbackground_tests() */