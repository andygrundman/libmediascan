#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <libmediascan.h>

#include "../src/mediascan.h"
#include "../src/database.h"
#include "CUnit/CUnit/Headers/Basic.h"

#ifndef MAX_PATH
#define MAX_PATH 1024
#endif

static int result_called = 0;
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

	result_called++;
} /* my_result_callback() */

static void my_error_callback(MediaScan *s, MediaScanError *error, void *userdata) { 

} /* my_error_callback() */

static void test_image_scanning(void)	{

  long time1, time2;

	#ifdef WIN32
	const char file01[MAX_PATH] = "data\\image\\larger\\cat_1.png";
	const char file02[MAX_PATH] = "data\\image\\larger\\hs-1991-05-a-full_jpg.jpg";
	const char file03[MAX_PATH] = "data\\image\\larger\\hs-1991-05-a-full_tif.tif";
	const char file04[MAX_PATH] = "data\\image\\larger\\hs-1995-14-a-full_tif.tif";
	const char file05[MAX_PATH] = "data\\image\\larger\\hs-1995-49-a-full_tif.tif";
	const char file06[MAX_PATH] = "data\\image\\larger\\hs-2005-02-c-full_jpg.jpg";
	const char file07[MAX_PATH] = "data\\image\\larger\\QCpatternA-v2.bmp";
	const char file08[MAX_PATH] = "data\\image\\larger\\star-chart-bars-full-600dpi.png";
	const char file09[MAX_PATH] = "data\\image\\larger\\test pattern.jpg";
	const char file10[MAX_PATH] = "data\\image\\larger\\test_pattern.gif";
	const char file11[MAX_PATH] = "data\\image\\larger\\test-pattern.jpg";
	#else
	const char file01[MAX_PATH] = "data/image/larger/cat_1.png";
	const char file02[MAX_PATH] = "data/image/larger/hs-1991-05-a-full_jpg.jpg";
	const char file03[MAX_PATH] = "data/image/larger/hs-1991-05-a-full_tif.tif";
	const char file04[MAX_PATH] = "data/image/larger/hs-1995-14-a-full_tif.tif";
	const char file05[MAX_PATH] = "data/image/larger/hs-1995-49-a-full_tif.tif";
	const char file06[MAX_PATH] = "data/image/larger/hs-2005-02-c-full_jpg.jpg";
	const char file07[MAX_PATH] = "data/image/larger/QCpatternA-v2.bmp";
	const char file08[MAX_PATH] = "data/image/larger/star-chart-bars-full-600dpi.png";
	const char file09[MAX_PATH] = "data/image/larger/test pattern.jpg";
	const char file10[MAX_PATH] = "data/image/larger/test_pattern.gif";
	const char file11[MAX_PATH] = "data/image/larger/test-pattern.jpg";

	const char dir[MAX_PATH] = "data/video/dlna";
  struct timeval now;
	#endif

	MediaScan *s = ms_create();

//	CU_ASSERT(s->npaths == 0);
//	ms_add_path(s, dir);
	//CU_ASSERT(s->npaths == 1);

	CU_ASSERT( s->async == FALSE );
	ms_set_async(s, FALSE);
	CU_ASSERT( s->async == FALSE );

	CU_ASSERT(s->on_result == NULL);
	ms_set_result_callback(s, my_result_callback);
	CU_ASSERT(s->on_result == my_result_callback);

	CU_ASSERT(s->on_error == NULL);
	ms_set_error_callback(s, my_error_callback); 
	CU_ASSERT(s->on_error == my_error_callback);

	ms_scan_file(s, file01, TYPE_UNKNOWN);
	CU_ASSERT( result_called == 1 );
	CU_ASSERT_STRING_EQUAL(result.mime_type, "image/png");
	CU_ASSERT_STRING_EQUAL(result.image->codec, "PNG");
	CU_ASSERT( result.image->width == 512 );
	CU_ASSERT( result.image->height == 512 );
	CU_ASSERT( result.image->channels == 4 );
	CU_ASSERT( result.image->has_alpha == 1 );

	result_called = 0;
	ms_scan_file(s, file02, TYPE_UNKNOWN);
	CU_ASSERT( result_called == 1 );
	CU_ASSERT_STRING_EQUAL(result.mime_type, "image/jpeg");
	CU_ASSERT_STRING_EQUAL(result.image->codec, "JPEG");
	CU_ASSERT( result.image->width == 3307 );
	CU_ASSERT( result.image->height == 2347 );
	CU_ASSERT( result.image->channels == 3 );
	CU_ASSERT( result.image->has_alpha == 0 );

	result_called = 0;
	ms_scan_file(s, file03, TYPE_UNKNOWN);
	CU_ASSERT( result_called == 1 );
	CU_ASSERT_STRING_EQUAL(result.mime_type, "image/tiff");
	CU_ASSERT_STRING_EQUAL(result.image->codec, "TIF");
	CU_ASSERT( result.image->width == 3307 );
	CU_ASSERT( result.image->height == 2347 );
	CU_ASSERT( result.image->channels == 3 );
	CU_ASSERT( result.image->has_alpha == 0 );

	result_called = 0;
	ms_scan_file(s, file04, TYPE_UNKNOWN);
	CU_ASSERT( result_called == 1 );
	CU_ASSERT_STRING_EQUAL(result.mime_type, "image/tiff");
	CU_ASSERT_STRING_EQUAL(result.image->codec, "TIFF");
	CU_ASSERT( result.image->width == 3307 );
	CU_ASSERT( result.image->height == 2347 );
	CU_ASSERT( result.image->channels == 3 );
	CU_ASSERT( result.image->has_alpha == 0 );

	result_called = 0;
	ms_scan_file(s, file05, TYPE_UNKNOWN);
	CU_ASSERT( result_called == 1 );
	CU_ASSERT_STRING_EQUAL(result.mime_type, "image/tiff");
	CU_ASSERT_STRING_EQUAL(result.image->codec, "TIFF");
	CU_ASSERT( result.image->width == 3307 );
	CU_ASSERT( result.image->height == 2347 );
	CU_ASSERT( result.image->channels == 3 );
	CU_ASSERT( result.image->has_alpha == 0 );

	result_called = 0;
	ms_scan_file(s, file06, TYPE_UNKNOWN);
	CU_ASSERT( result_called == 1 );
	CU_ASSERT_STRING_EQUAL(result.mime_type, "image/jpeg");
	CU_ASSERT_STRING_EQUAL(result.image->codec, "JPEG");
	CU_ASSERT( result.image->width == 1651 );
	CU_ASSERT( result.image->height == 1651 );
	CU_ASSERT( result.image->channels == 3 );
	CU_ASSERT( result.image->has_alpha == 0 );

	result_called = 0;
	ms_scan_file(s, file07, TYPE_UNKNOWN);
	CU_ASSERT( result_called == 1 );
	CU_ASSERT_STRING_EQUAL(result.mime_type, "image/x-ms-bmp");
	CU_ASSERT_STRING_EQUAL(result.image->codec, "BMP");
	CU_ASSERT( result.image->width == 1024 );
	CU_ASSERT( result.image->height == 1024 );
	CU_ASSERT( result.image->channels == 4 );
	CU_ASSERT( result.image->has_alpha == 0 );

	result_called = 0;
	ms_scan_file(s, file08, TYPE_UNKNOWN);
	CU_ASSERT( result_called == 1 );
	CU_ASSERT_STRING_EQUAL(result.mime_type, "image/png");
	CU_ASSERT_STRING_EQUAL(result.image->codec, "PNG");
	CU_ASSERT( result.image->width == 6299 );
	CU_ASSERT( result.image->height == 4725 );
	CU_ASSERT( result.image->channels == 1 );
	CU_ASSERT( result.image->has_alpha == 1 );

	result_called = 0;
	ms_scan_file(s, file09, TYPE_UNKNOWN);
	CU_ASSERT( result_called == 1 );
	CU_ASSERT_STRING_EQUAL(result.mime_type, "image/jpeg");
	CU_ASSERT_STRING_EQUAL(result.image->codec, "JPEG");
	CU_ASSERT( result.image->width == 1924 );
	CU_ASSERT( result.image->height == 1088 );
	CU_ASSERT( result.image->channels == 3 );
	CU_ASSERT( result.image->has_alpha == 0 );

	result_called = 0;
	ms_scan_file(s, file10, TYPE_UNKNOWN);
	CU_ASSERT( result_called == 1 );
	CU_ASSERT_STRING_EQUAL(result.mime_type, "image/gif");
	CU_ASSERT_STRING_EQUAL(result.image->codec, "GIF");
	CU_ASSERT( result.image->width == 640 );
	CU_ASSERT( result.image->height == 480 );
	CU_ASSERT( result.image->channels == 0 );
	CU_ASSERT( result.image->has_alpha == 0 );

	result_called = 0;
	ms_scan_file(s, file11, TYPE_UNKNOWN);
	CU_ASSERT( result_called == 1 );
	CU_ASSERT_STRING_EQUAL(result.mime_type, "image/jpeg");
	CU_ASSERT_STRING_EQUAL(result.image->codec, "JPEG");
	CU_ASSERT( result.image->width == 1440 );
	CU_ASSERT( result.image->height == 1086 );
	CU_ASSERT( result.image->channels == 1 );
	CU_ASSERT( result.image->has_alpha == 0 );

//	ms_scan(s);
//	CU_ASSERT( result_called == 5 );

//	result_called = 0;
//	reset_bdb(s);

	ms_destroy(s);
} /* test_image_scanning() */

///-------------------------------------------------------------------------------------------------
///  Setup background tests.
///
/// @author Henry Bennett
/// @date 03/22/2011
///-------------------------------------------------------------------------------------------------

int setupimage_tests() {
	CU_pSuite pSuite = NULL;


   /* add a suite to the registry */
   pSuite = CU_add_suite("Image Scanning", NULL, NULL);
   if (NULL == pSuite) {
      CU_cleanup_registry();
      return CU_get_error();
   }

   /* add the tests to the background scanning suite */
   if (NULL == CU_add_test(pSuite, "Test Image Scanning", test_image_scanning) 
 	   )
   {
      CU_cleanup_registry();
      return CU_get_error();
   }

   return 0;

} /* setupimage_tests() */
