/*
 *  Simple example of a CUnit unit test.
 *
 *  This program (crudely) demonstrates a very simple "black box"
 *  test of the standard library functions fprintf() and fread().
 *  It uses suite initialization and cleanup functions to open
 *  and close a common temporary file used by the test functions.
 *  The test functions then write to and read from the temporary
 *  file in the course of testing the library functions.
 *
 *  The 2 test functions are added to a single CUnit suite, and
 *  then run using the CUnit Basic interface.  The output of the
 *  program (on CUnit version 2.0-2) is:
 *
 *           CUnit : A Unit testing framework for C.
 *           http://cunit.sourceforge.net/
 *
 *       Suite: Suite_1
 *         Test: test of fprintf() ... passed
 *         Test: test of fread() ... passed
 *
 *       --Run Summary: Type      Total     Ran  Passed  Failed
 *                      suites        1       1     n/a       0
 *                      tests         2       2       2       0
 *                      asserts       5       5       5       0
 */

#include <stdio.h>
#include <string.h>


#ifdef WIN32
#include <Windows.h>
#include <direct.h>
#endif

#include <limits.h>
#include <libmediascan.h>

#include "../src/mediascan.h"
#include "Cunit/CUnit/Headers/Basic.h"

#ifndef MAX_PATH
#define MAX_PATH 1024
#endif

/* Pointer to the file used by the tests. */
static FILE* temp_file = NULL;

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

int init_suite1(void)
{
	temp_file = fopen("./temp.txt", "w+");

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

int clean_suite1(void)
{

   if (0 != fclose(temp_file)) {
      return -1;
   }
   else {
      temp_file = NULL;
      return 0;
   }


}

static	void my_result_callback(MediaScan *s, MediaScanResult *result) {

}

static void my_error_callback(MediaScan *s, MediaScanError *error) { 

} /* my_error_callback() */

///-------------------------------------------------------------------------------------------------
///  Unit Test Scan Function.
///
/// @author Henry Bennett
/// @date 03/16/2011
///
/// ### remarks Henry Bennett, 03/15/2011.
///-------------------------------------------------------------------------------------------------

void test_ms_scan(void){
	
	char dir[MAX_PATH] = "data";
	MediaScan *s = ms_create();

	CU_ASSERT(s->npaths == 0);
	ms_add_path(s, dir);
	CU_ASSERT(s->npaths == 1);
	CU_ASSERT_STRING_EQUAL(s->paths[0], dir);


	CU_ASSERT(s->on_result == NULL);
	ms_set_result_callback(s, my_result_callback);
	CU_ASSERT(s->on_result == my_result_callback);

	CU_ASSERT(s->on_error == NULL);
	ms_set_error_callback(s, my_error_callback); 
	CU_ASSERT(s->on_error == my_error_callback);

	ms_scan(s);

	ms_destroy(s);
}

///-------------------------------------------------------------------------------------------------
///  This test will check the scanning functionality without adding a directory to the scan
/// 	list.
///
/// @author Henry Bennett
/// @date 03/16/2011
///
/// ### remarks Henry Bennett, 03/16/2011.
///-------------------------------------------------------------------------------------------------

void test_ms_scan_2(void)
{
	MediaScan *s = ms_create();

	// Do not add a path and then attempt to scan
	CU_ASSERT(s->npaths == 0);

	CU_ASSERT(s->on_result == NULL);
	ms_set_result_callback(s, my_result_callback);
	CU_ASSERT(s->on_result == my_result_callback);

	CU_ASSERT(s->on_error == NULL);
	ms_set_error_callback(s, my_error_callback); 
	CU_ASSERT(s->on_error == my_error_callback);

	ms_scan(s);

	ms_destroy(s);
}

///-------------------------------------------------------------------------------------------------
///  Robustness test for ms_scan. Try to add more than 128 directories to scan.
///
/// @author Henry Bennett
/// @date 03/16/2011
///
/// ### remarks Henry Bennett, 03/16/2011.
///-------------------------------------------------------------------------------------------------

void test_ms_scan_3(void)	{

	char dir[MAX_PATH];
	char dir2[MAX_PATH];
	int i = 0;

	MediaScan *s = ms_create();

	CU_ASSERT(s->npaths == 0);

	// Add 128 Paths to the list, which should work fine
	for(i = 0; i < 128; i++)
	{
		sprintf(dir, "testdir%04d", i);
		CU_ASSERT(s->npaths == (i) );
		ms_add_path(s, dir);
		CU_ASSERT(s->npaths == (i+1) );
		CU_ASSERT_STRING_EQUAL(s->paths[i], dir);
	}

	CU_ASSERT(s->npaths == 128 );
	CU_ASSERT_STRING_EQUAL(s->paths[127], dir);

	strcpy(dir2, dir);
	sprintf(dir, "toomany");
	ms_add_path(s, dir); // This will fail

	// Make sure number of paths hasn't gone up and the last entry is the same
	CU_ASSERT(s->npaths == 128 );
	CU_ASSERT_STRING_EQUAL(s->paths[127], dir2);

	ms_destroy(s);
}

///-------------------------------------------------------------------------------------------------
///  Test ms_scan with a non-exsistent directory structure.
///
/// @author Henry Bennett
/// @date 03/16/2011
///-------------------------------------------------------------------------------------------------

void test_ms_scan_4(void)	{
	char dir[MAX_PATH] = "notadirectory";
	MediaScan *s = ms_create();

	CU_ASSERT(s->npaths == 0);
	ms_add_path(s, dir);
	CU_ASSERT(s->npaths == 1);
	CU_ASSERT_STRING_EQUAL(s->paths[0], dir);

	// Won't run without the call backs in place
	CU_ASSERT(s->on_result == NULL);
	ms_set_result_callback(s, my_result_callback);
	CU_ASSERT(s->on_result == my_result_callback);

	CU_ASSERT(s->on_error == NULL);
	ms_set_error_callback(s, my_error_callback); 
	CU_ASSERT(s->on_error == my_error_callback);

	// Okay this is the test, try to scan a non-exsistent directory structure
	ms_scan(s);
	CU_ASSERT(ms_errno == MSENO_DIRECTORYFAIL); // If we got this errno, then we got the failure we wanted

	ms_destroy(s);
} /* test_ms_scan_4 */

///-------------------------------------------------------------------------------------------------
///  Test ms_scan's path stripping functionality
///
/// @author Henry Bennett
/// @date 03/16/2011
///-------------------------------------------------------------------------------------------------

void test_ms_scan_5(void)	{
		#define NUM_STRINGS 3
	
	int i = 0;
	char in_dir[NUM_STRINGS][MAX_PATH] = 
		{ "data", "data/", "data\\" };
	char out_dir[NUM_STRINGS][MAX_PATH] = 
		{ "data", "data/", "data\\" };

	MediaScan *s = ms_create();

	for(i = 0; i < NUM_STRINGS; i++)
	{
	ms_add_path(s, in_dir[i]);
	CU_ASSERT(s->npaths == i + 1);
	CU_ASSERT_STRING_EQUAL(s->paths[i], in_dir[i]);
	}

	// Won't run without the call backs in place
	CU_ASSERT(s->on_result == NULL);
	ms_set_result_callback(s, my_result_callback);
	CU_ASSERT(s->on_result == my_result_callback);

	CU_ASSERT(s->on_error == NULL);
	ms_set_error_callback(s, my_error_callback); 
	CU_ASSERT(s->on_error == my_error_callback);

	// Run the scan
	ms_errno = 0;
	ms_scan(s);
	CU_ASSERT(ms_errno != MSENO_DIRECTORYFAIL); // Should get no error if the stripping worked correctly

	ms_destroy(s);
} /* test_ms_scan_5 */

static int progress_called = FALSE;

static void my_progress_callback_6(MediaScan *s, MediaScanProgress *progress) {
  // Check final progress callback only
//  if (!progress->cur_item) {
//    ok(progress->dir_total == 3, "final progress callback dir_total is %d", progress->dir_total);
//    ok(progress->file_total == 22, "final progress callback file_total is %d", progress->file_total);
//  }
//  
	if(s->progress->cur_item != NULL)
		progress_called = TRUE;

} /* my_progress_callback() */

///-------------------------------------------------------------------------------------------------
///  Test ms_scan's on progress notification
///
/// @author Henry Bennett
/// @date 03/16/2011
///-------------------------------------------------------------------------------------------------

void test_ms_scan_6(void)	{
	char dir[MAX_PATH] = "data/audio/mp3";
	MediaScan *s = ms_create();

	CU_ASSERT(s->npaths == 0);
	ms_add_path(s, dir);
	CU_ASSERT(s->npaths == 1);
	CU_ASSERT_STRING_EQUAL(s->paths[0], dir);


	CU_ASSERT(s->on_result == NULL);
	ms_set_result_callback(s, my_result_callback);
	CU_ASSERT(s->on_result == my_result_callback);

	CU_ASSERT(s->on_error == NULL);
	ms_set_error_callback(s, my_error_callback); 
	CU_ASSERT(s->on_error == my_error_callback);

	CU_ASSERT(s->on_progress == NULL);
	ms_set_progress_callback(s, my_progress_callback_6); 
	CU_ASSERT(s->on_progress == my_progress_callback_6);
	
	
	CU_ASSERT(s->progress_interval == 1); // Verify that the progress interval is set to 1

	ms_set_progress_interval(s, 60);
	CU_ASSERT(s->progress_interval == 60);

	// Reset the callback time
	progress_called = FALSE;
	s->progress->_last_callback = 0;
	ms_scan(s);
	CU_ASSERT(progress_called == FALSE);
	#ifdef WIN32
	Sleep(40); // wait 40ms
	#else
	sleep(40);
	#endif
	ms_scan(s);
	CU_ASSERT(progress_called == FALSE);

	// Reset the callback time
	s->progress->_last_callback = 0;
	ms_scan(s);
	CU_ASSERT(progress_called == FALSE);
	
	#ifdef WIN32
	Sleep(61); // wait 61ms
	#else
	sleep(40);
	#endif;
	
	ms_scan(s);
	CU_ASSERT(progress_called == TRUE);

	ms_destroy(s);

} /* test_ms_scan_6 */

static	void my_result_callback_1(MediaScan *s, MediaScanResult *result) {

}

static int error_called = FALSE;
static MediaScanError error;

static void my_error_callback_1(MediaScan *s, MediaScanError *error) { 
	error_called = TRUE;
	memcpy(&error, error, sizeof(MediaScanError));
} /* my_error_callback() */

///-------------------------------------------------------------------------------------------------
///  Test test_ms_file_scan_1'
///
/// @author Henry Bennett
/// @date 03/18/2011
///-------------------------------------------------------------------------------------------------

void test_ms_file_scan_1(void)	{
	char valid_file[MAX_PATH] = "data\video\bars-mpeg4-aac.m4v";
	char invalid_file[MAX_PATH] = "data\video\notafile.m4v";
	
	MediaScan *s = ms_create();
	
	// Check null mediascan oject
	ms_errno = 0;
	ms_scan_file(NULL, invalid_file, TYPE_VIDEO);
	CU_ASSERT(ms_errno == MSENO_NULLSCANOBJ);

	// Check for no callback set
	ms_errno = 0;
	ms_scan_file(s, invalid_file, TYPE_VIDEO);
	CU_ASSERT(ms_errno == MSENO_NORESULTCALLBACK);

	CU_ASSERT(s->on_result == NULL);
	ms_set_result_callback(s, my_result_callback_1);
	CU_ASSERT(s->on_result == my_result_callback_1);

	// Now scan an invalid file, without an error handler
	ms_errno = 0;
	ms_scan_file(s, invalid_file, TYPE_VIDEO);
	CU_ASSERT(ms_errno == MSENO_NOERRORCALLBACK);

	// Set up an error callback
	CU_ASSERT(s->on_error == NULL);
	ms_set_error_callback(s, my_error_callback); 
	CU_ASSERT(s->on_error == my_error_callback);

	// Now scan an invalid file, with an error handler
	ms_errno = 0;
	error_called = FALSE;
	ms_scan_file(s, invalid_file, TYPE_VIDEO);
	CU_ASSERT(ms_errno != MSENO_NOERRORCALLBACK);
	CU_ASSERT(error_called == TRUE);

	// Now scan a valid video file, with an error handler
	ms_errno = 0;
	error_called = FALSE;
	ms_scan_file(s, valid_file, TYPE_VIDEO);
	// Need more info to run this test

	ms_destroy(s);
} /* test_ms_file_scan_1 */

///-------------------------------------------------------------------------------------------------
///  Test ms_set_async and ms_set_log_level
///
/// @author Henry Bennett
/// @date 03/18/2011
///-------------------------------------------------------------------------------------------------

void test_ms_misc_functions(void)	{
	MediaScan *s = ms_create();
	

	ms_destroy(s);
} /* test_ms_misc_functions() */

///-------------------------------------------------------------------------------------------------
///  ------------------------------------------------------------------------------------------
/// 	  The main() function for setting up and running the tests. Returns a CUE_SUCCESS on
/// 	  successful running, another CUnit error code on failure.
///
/// @author Henry Bennett
/// @date 03/16/2011
///
/// @return .
///-------------------------------------------------------------------------------------------------

int run_unit_tests()
{
   CU_pSuite pSuite = NULL;

   /* initialize the CUnit test registry */
   if (CUE_SUCCESS != CU_initialize_registry())
      return CU_get_error();

   /* add a suite to the registry */
   pSuite = CU_add_suite("File Scan", init_suite1, clean_suite1);
   if (NULL == pSuite) {
      CU_cleanup_registry();
      return CU_get_error();
   }

   /* add the tests to the suite */
   if (
	   NULL == CU_add_test(pSuite, "Simple test of ms_scan()", test_ms_scan) ||
	   NULL == CU_add_test(pSuite, "Test ms_scan() with no paths", test_ms_scan_2) ||
	   NULL == CU_add_test(pSuite, "Test of ms_scan() with too many paths", test_ms_scan_3) ||
	   NULL == CU_add_test(pSuite, "Test of ms_scan() with a bad directory", test_ms_scan_4) ||
//	   NULL == CU_add_test(pSuite, "Test of ms_scan() with strange path slashes", test_ms_scan_5) ||
	   NULL == CU_add_test(pSuite, "Test of ms_scan()'s progress notifications", test_ms_scan_6) ||
	   NULL == CU_add_test(pSuite, "Test of ms_file_scan()", test_ms_file_scan_1) ||
	   NULL == CU_add_test(pSuite, "Test of misc functions", test_ms_misc_functions)
	   )
   {
      CU_cleanup_registry();
      return CU_get_error();
   }

   /* Run all tests using the CUnit Basic interface */
   CU_basic_set_mode(CU_BRM_VERBOSE);
   CU_basic_run_tests();
   CU_cleanup_registry();
   return CU_get_error();
} /* run_unit_tests() */


