#include <stdio.h>
#include <string.h>


#ifdef WIN32
#include <Windows.h>
#include <direct.h>
#else
#include <fcntl.h>
#include <sys/wait.h>
#endif

#ifdef WIN32
# define HAVE_BOOLEAN
#endif
#include <jpeglib.h>
#include <limits.h>
#include <libmediascan.h>

#define PNG_DEBUG 3
#include <png.h>


#include "../src/mediascan.h"
#include "../src/common.h"
#include "../src/buffer.h"
#include "../src/image.h"
#include "../src/image_png.h"


#include "Cunit/CUnit/Headers/Basic.h"

/*
#ifndef MAX_PATH
#define MAX_PATH 1024
#endif
*/

static int result_called = FALSE;
static int error_called = FALSE;

static MediaScanResult result;

static void abort_(const char * s, ...)
{
        va_list args;
        va_start(args, s);
        vfprintf(stderr, s, args);
        fprintf(stderr, "\n");
        va_end(args);
        abort();
}


static void my_result_callback(MediaScan *s, MediaScanResult *r, void *userdata) {

	int i;

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
	result.nthumbnails = r->nthumbnails;

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

  for (i = 0; i < r->nthumbnails; i++)
	{
//		if(result._thumbs[i])
//			free(result._thumbs[i]);

		result._thumbs[i] = malloc( sizeof(struct _Image) );
		result._thumbs[i]->path = r->_thumbs[i]->path;
		result._thumbs[i]->codec = r->_thumbs[i]->codec;
		result._thumbs[i]->width = r->_thumbs[i]->width;
		result._thumbs[i]->height = r->_thumbs[i]->height;
		result._thumbs[i]->channels = r->_thumbs[i]->channels;
		result._thumbs[i]->has_alpha = r->_thumbs[i]->has_alpha;
		result._thumbs[i]->offset = r->_thumbs[i]->offset;
		result._thumbs[i]->orientation = r->_thumbs[i]->orientation;

		result._thumbs[i]->_pixbuf_size = r->_thumbs[i]->_pixbuf_size;
		result._thumbs[i]->_pixbuf_is_copy = r->_thumbs[i]->_pixbuf_is_copy;

//		if(result._thumbs[i]->_pixbuf)
//			free(result._thumbs[i]->_pixbuf);

		result._thumbs[i]->_pixbuf = malloc( r->_thumbs[i]->_pixbuf_size );
		memcpy( result._thumbs[i]->_pixbuf, r->_thumbs[i]->_pixbuf, r->_thumbs[i]->_pixbuf_size);

//		if(result._thumbs[i]->_dbuf)
//			free(result._thumbs[i]->_dbuf);

		result._thumbs[i]->_dbuf = malloc( buffer_len(r->_thumbs[i]->_dbuf) );
		memcpy( result._thumbs[i]->_dbuf, r->_thumbs[i]->_dbuf, buffer_len(r->_thumbs[i]->_dbuf));


//		if(result._thumbs[i]->_png)
//			free(result._thumbs[i]->_png);

		if(r->_thumbs[i]->_png)
		{
		result._thumbs[i]->_png = malloc( buffer_len(r->_thumbs[i]->_png) );
		memcpy( result._thumbs[i]->_png, r->_thumbs[i]->_png, buffer_len(r->_thumbs[i]->_png));
		}

	}

	result_called = TRUE;
} /* my_result_callback() */

static void my_error_callback(MediaScan *s, MediaScanError *error, void *userdata) { 
	error_called = TRUE;
} /* my_error_callback() */


// Basic image reading tests
static const struct {
  const char *filename;
	const char *mime_type;
	const char *codec;
	int channels;
	int width;
	int height;
	int failure;
	const char *thumb_filename;
} expected_results[] = {

	// Bitmaps
  { "data\\image\\bmp\\1bit.bmp",					"image/x-ms-bmp", "BMP", 4, 127, 64, FALSE, "data\\image\\bmp\\thumb\\1bit"   },
  { "data\\image\\bmp\\4bit.bmp",					"image/x-ms-bmp", "BMP", 4, 127, 64, FALSE, "data\\image\\bmp\\thumb\\4bit"  },
  { "data\\image\\bmp\\4bit_rle.bmp",			"image/x-ms-bmp", "BMP", 4, 127, 64, FALSE, "data\\image\\bmp\\thumb\\4bit_rle"  },
  { "data\\image\\bmp\\8bit.bmp",					"image/x-ms-bmp", "BMP", 4, 127, 64, FALSE, "data\\image\\bmp\\thumb\\8bit"  },
  { "data\\image\\bmp\\8bit_os2.bmp",			"image/x-ms-bmp", "BMP", 4, 127, 64, TRUE,  "data\\image\\bmp\\thumb\\8bit_os2"  },

	// Note: BMP RLE compression is not supported
//  { "data\\image\\bmp\\8bit_rle.bmp",			"image/x-ms-bmp", "BMP", 4, 127, 64, FALSE, "data\\image\\bmp\\thumb\\8bit_rle"   }, 
	{ "data\\image\\bmp\\16bit_555.bmp",		"image/x-ms-bmp", "BMP", 4, 127, 64, FALSE, "data\\image\\bmp\\thumb\\16bit_555"   },
  { "data\\image\\bmp\\16bit_565.bmp",		"image/x-ms-bmp", "BMP", 4, 127, 64, FALSE, "data\\image\\bmp\\thumb\\16bit_565"   },
  { "data\\image\\bmp\\24bit.bmp",				"image/x-ms-bmp", "BMP", 4, 127, 64, FALSE, "data\\image\\bmp\\thumb\\24bit"   },
  { "data\\image\\bmp\\32bit.bmp",				"image/x-ms-bmp", "BMP", 4, 127, 64, FALSE, "data\\image\\bmp\\thumb\\32bit"   },
  { "data\\image\\bmp\\32bit_alpha.bmp",	"image/x-ms-bmp", "BMP", 4, 127, 64, FALSE, "data\\image\\bmp\\thumb\\32bit_alpha"   },

	// GIF
	// Note: GIF is not currently supported
//  { "data\\image\\gif\\corrupt.gif",				"image/gif", "GIF", 4, 160, 120, TRUE, "data\\image\\gif\\thumb\\corrupt"   },
//  { "data\\image\\gif\\interlaced_256.gif",	"image/gif", "GIF", 4, 160, 120, FALSE, "data\\image\\gif\\thumb\\interlaced_256"   },
//  { "data\\image\\gif\\transparent.gif",		"image/gif", "GIF", 4, 160, 120, FALSE, "data\\image\\gif\\thumb\\transparent"   },
//  { "data\\image\\gif\\white.gif",					"image/gif", "GIF", 4, 160, 120, FALSE, "data\\image\\gif\\thumb\\white"   },

	// JPEG
  { "data\\image\\jpg\\cmyk.jpg",												"image/jpeg", "JPEG", 4, 313, 234, FALSE, "data\\image\\jpg\\thumb\\cmyk"   },
  { "data\\image\\jpg\\corrupt.jpg",										"image/jpeg", "JPEG", 4, 313, 234, TRUE,	"data\\image\\jpg\\thumb\\corrupt"   },
  { "data\\image\\jpg\\exif_90_ccw.jpg",								"image/jpeg", "JPEG", 3, 117, 157, FALSE, "data\\image\\jpg\\thumb\\exif_90_ccw"   },
  { "data\\image\\jpg\\exif_180.jpg",										"image/jpeg", "JPEG", 3, 157, 117, FALSE, "data\\image\\jpg\\thumb\\exif_180"   },
  { "data\\image\\jpg\\exif_270_ccw.jpg",								"image/jpeg", "JPEG", 3, 117, 157, FALSE, "data\\image\\jpg\\thumb\\exif_270_ccw"   },
  { "data\\image\\jpg\\exif_mirror_horiz.jpg",					"image/jpeg", "JPEG", 3, 157, 117, FALSE, "data\\image\\jpg\\thumb\\exif_mirror_horiz"   },
  { "data\\image\\jpg\\exif_mirror_horiz_90_ccw.jpg",		"image/jpeg", "JPEG", 3, 117, 157, FALSE, "data\\image\\jpg\\thumb\\exif_mirror_horiz_90_ccw"   },
  { "data\\image\\jpg\\exif_mirror_horiz_270_ccw.jpg",	"image/jpeg", "JPEG", 3, 117, 157, FALSE, "data\\image\\jpg\\thumb\\exif_mirror_horiz_270_ccw"   },
  { "data\\image\\jpg\\exif_mirror_vert.jpg",						"image/jpeg", "JPEG", 3, 157, 117, FALSE, "data\\image\\jpg\\thumb\\exif_mirror_vert"   },
  { "data\\image\\jpg\\gray.jpg",												"image/jpeg", "JPEG", 1, 313, 234, FALSE, "data\\image\\jpg\\thumb\\gray"   },
  { "data\\image\\jpg\\gray_progressive.jpg",						"image/jpeg", "JPEG", 1, 313, 234, FALSE, "data\\image\\jpg\\thumb\\gray_progressive"   },
  { "data\\image\\jpg\\large-exif.jpg",									"image/jpeg", "JPEG", 3, 200, 200, FALSE, "data\\image\\jpg\\thumb\\large-exif"   },
  { "data\\image\\jpg\\rgb.jpg",												"image/jpeg", "JPEG", 3, 313, 234, FALSE, "data\\image\\jpg\\thumb\\rgb"   },
  { "data\\image\\jpg\\rgb_progressive.jpg",						"image/jpeg", "JPEG", 3, 313, 234, FALSE, "data\\image\\jpg\\thumb\\rgb_progressive"   },
  { "data\\image\\jpg\\truncated.jpg",									"image/jpeg", "JPEG", 3, 313, 234, FALSE, "data\\image\\jpg\\thumb\\truncated"   },

	// PNG
  { "data\\image\\png\\gray.png",												"image/png", "PNG", 1, 160, 120, FALSE, "data\\image\\png\\thumb\\gray"   },
  { "data\\image\\png\\gray_alpha.png",									"image/png", "PNG", 2, 160, 120, FALSE, "data\\image\\png\\thumb\\gray_alpha"   },
  { "data\\image\\png\\gray_interlaced.png",						"image/png", "PNG", 1, 160, 120, FALSE, "data\\image\\png\\thumb\\gray_interlaced"   },
  { "data\\image\\png\\palette.png",										"image/png", "PNG", 1, 160, 120, FALSE, "data\\image\\png\\thumb\\palette"   },
  { "data\\image\\png\\palette_alpha.png",							"image/png", "PNG", 1, 160, 120, FALSE, "data\\image\\png\\thumb\\palette_alpha"   },
  { "data\\image\\png\\palette_bkgd.png",								"image/png", "PNG", 1, 98, 31, FALSE,		"data\\image\\png\\thumb\\palette_bkgd"   },
  { "data\\image\\png\\rgb.png",												"image/png", "PNG", 3, 160, 120, FALSE, "data\\image\\png\\thumb\\rgb"   },
  { "data\\image\\png\\rgba.png",												"image/png", "PNG", 4, 160, 120, FALSE, "data\\image\\png\\thumb\\rgba"   },
  { "data\\image\\png\\rgba_interlaced.png",						"image/png", "PNG", 4, 160, 120, FALSE, "data\\image\\png\\thumb\\rgba_interlaced"   },
  { "data\\image\\png\\x00n0g01.png",										"image/png", "PNG", 4, 160, 120, TRUE, "data\\image\\png\\thumb\\x00n0g01"   },
  { "data\\image\\png\\xcrn0g04.png",										"image/png", "PNG", 4, 160, 120, TRUE, "data\\image\\png\\thumb\\xcrn0g04"   },

  { NULL, 0 }
};


///-------------------------------------------------------------------------------------------------
///  Test image reading
///
/// @author Henry Bennett
/// @date 03/18/2011
///-------------------------------------------------------------------------------------------------

void test_image_reading(void)	{
	int i = 0;

	MediaScan *s = ms_create();

	CU_ASSERT_FATAL(s != NULL);

	CU_ASSERT(s->on_result == NULL);
	ms_set_result_callback(s, my_result_callback);
	CU_ASSERT(s->on_result == my_result_callback);

	CU_ASSERT(s->on_error == NULL);
	ms_set_error_callback(s, my_error_callback); 
	CU_ASSERT(s->on_error == my_error_callback);

	for(i = 0; expected_results[i].filename; i++) 
	{
		error_called = FALSE;
		result_called = FALSE;

		ms_scan_file(s, expected_results[i].filename, TYPE_IMAGE);
	
		if(expected_results[i].failure) {
			CU_ASSERT(error_called);
			continue;
		}
		else {
			CU_ASSERT(result_called);
		}

		CU_ASSERT_PTR_NOT_NULL(result.mime_type);
		if(result.mime_type) {
			CU_ASSERT_STRING_EQUAL(result.mime_type, expected_results[i].mime_type); }

		CU_ASSERT_PTR_NOT_NULL(result.image);
		if(result.image) {
			CU_ASSERT_STRING_EQUAL(result.image->codec, expected_results[i].codec);
			CU_ASSERT(result.image->width == expected_results[i].width);
			CU_ASSERT(result.image->height == expected_results[i].height);
			CU_ASSERT(result.image->channels == expected_results[i].channels);

			if(!strcmp(expected_results[i].codec, "GIF") )
			{
				CU_ASSERT(result.image->_jpeg == NULL);
				CU_ASSERT(result.image->_png == NULL);
				CU_ASSERT(result.image->_bmp == NULL);
		//		CU_ASSERT(result.image->_gif != NULL);
			}
			else if(!strcmp(expected_results[i].codec, "JPEG") )
			{
				CU_ASSERT(result.image->_jpeg != NULL);
				CU_ASSERT(result.image->_png == NULL);
				CU_ASSERT(result.image->_bmp == NULL);
		//		CU_ASSERT(result.image->_gif == NULL);
			}
			else if(!strcmp(expected_results[i].codec, "PNG") )
			{
				CU_ASSERT(result.image->_jpeg == NULL);
				CU_ASSERT(result.image->_png != NULL);
				CU_ASSERT(result.image->_bmp == NULL);
		//		CU_ASSERT(result.image->_gif == NULL);
			}
			else if(!strcmp(expected_results[i].codec, "BMP") )
			{
				CU_ASSERT(result.image->_jpeg == NULL);
				CU_ASSERT(result.image->_png == NULL);
				CU_ASSERT(result.image->_bmp != NULL);
		//		CU_ASSERT(result.image->_gif == NULL);
			}
		}

	}

	ms_destroy(s);
} /* test_image_reading() */

void write_png_file(char* file_name) {
  int j, x, y;
  int color_space = PNG_COLOR_TYPE_RGB_ALPHA;
  volatile unsigned char *ptr = NULL;
  png_structp png_ptr;
  png_infop info_ptr;
//  Buffer *buf;
	FILE *fp = fopen(file_name, "wb");

	if (!result._thumbs[0]->_pixbuf_size) {
    LOG_WARN("PNG compression requires pixbuf data\n");
    return;
  }

  png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
  if (!png_ptr) {
    FATAL("Could not initialize libpng\n");
  }

  info_ptr = png_create_info_struct(png_ptr);
  if (!info_ptr) {
    png_destroy_write_struct(&png_ptr, NULL);
    FATAL("Could not initialize libpng\n");
  }

  if (setjmp(png_jmpbuf(png_ptr))) {
    if (ptr != NULL)
      free((void *)ptr);
    return;
  }

  png_init_io(png_ptr, fp);

	/* write header */
	if (setjmp(png_jmpbuf(png_ptr)))
				abort_("[write_png_file] Error during writing header");


  // Match output color space with input file
	switch (result._thumbs[0]->channels) {
    case 4:
    case 3:
      LOG_DEBUG("PNG output color space set to RGBA\n");
      color_space = PNG_COLOR_TYPE_RGB_ALPHA;
      break;
    case 2:
    case 1:
      LOG_DEBUG("PNG output color space set to gray alpha\n");
      color_space = PNG_COLOR_TYPE_GRAY_ALPHA;
      break;
  }

  png_set_IHDR(png_ptr, info_ptr, result._thumbs[0]->width, result._thumbs[0]->height, 8, color_space,
               PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);

  png_write_info(png_ptr, info_ptr);

	/* write bytes */
	if (setjmp(png_jmpbuf(png_ptr)))
				abort_("[write_png_file] Error during writing bytes");


  ptr = (unsigned char *)malloc(png_get_rowbytes(png_ptr, info_ptr));

  j = 0;

  if (color_space == PNG_COLOR_TYPE_GRAY_ALPHA) {
    for (y = 0; y < result._thumbs[0]->height; y++) {
      for (x = 0; x < result._thumbs[0]->width; x++) {
        ptr[x * 2] = COL_BLUE(result._thumbs[0]->_pixbuf[j]);
        ptr[x * 2 + 1] = COL_ALPHA(result._thumbs[0]->_pixbuf[j]);
        j++;
      }
      png_write_row(png_ptr, (png_bytep) ptr);
    }
  }
  else {                        // RGB  
    for (y = 0; y < result._thumbs[0]->height; y++) {
      for (x = 0; x < result._thumbs[0]->width; x++) {
        ptr[x * 4] = COL_RED(result._thumbs[0]->_pixbuf[j]);
				ptr[x * 4 + 1] = COL_GREEN(result._thumbs[0]->_pixbuf[j]);
        ptr[x * 4 + 2] = COL_BLUE(result._thumbs[0]->_pixbuf[j]);
        ptr[x * 4 + 3] = COL_ALPHA(result._thumbs[0]->_pixbuf[j]);
        j++;
      }
      png_write_row(png_ptr, (png_bytep) ptr);
    }
  }

  free((void *)ptr);

	/* end write */
	if (setjmp(png_jmpbuf(png_ptr)))
				abort_("[write_png_file] Error during end of write");


  png_write_end(png_ptr, info_ptr);

  png_destroy_write_struct(&png_ptr, &info_ptr);

	fclose(fp);
}


///-------------------------------------------------------------------------------------------------
///  Test thumbnailing
///
/// @author Henry Bennett
/// @date 03/18/2011
///-------------------------------------------------------------------------------------------------

void test_thumbnailing(void)	{
	int i = 0;

	MediaScan *s = ms_create();

	CU_ASSERT_FATAL(s != NULL);

	CU_ASSERT(s->on_result == NULL);
	ms_set_result_callback(s, my_result_callback);
	CU_ASSERT(s->on_result == my_result_callback);

	CU_ASSERT(s->on_error == NULL);
	ms_set_error_callback(s, my_error_callback); 
	CU_ASSERT(s->on_error == my_error_callback);

	ms_add_thumbnail_spec(s, THUMB_PNG, 32,32, TRUE, 0, 90);

	ms_scan_file(s, expected_results[0].filename, TYPE_IMAGE);

	// TODO: Read and compare presaved and approved thumbnails

	ms_destroy(s);
} /* test_thumbnailing() */

void generate_thumbnails()
{
	FILE *tfp;
	char file[MAX_PATH_STR_LEN];
	int i, n;
	MediaScanImage *thumb;
	Buffer *dbuf;

	MediaScan *s = ms_create();
		ms_set_result_callback(s, my_result_callback);
			ms_set_error_callback(s, my_error_callback); 
			
	ms_add_thumbnail_spec(s, THUMB_PNG, 32,32, TRUE, 0, 90);

	for(i = 0; expected_results[i].filename; i++) 
	{
		ms_scan_file(s, expected_results[i].filename, TYPE_IMAGE);

		if (!strcmp("JPEG", result._thumbs[0]->codec))
			sprintf(file, "%s.jpg", expected_results[i].thumb_filename);
		else
			sprintf(file, "%s.png",expected_results[i].thumb_filename);

		write_png_file(file);
	}


		ms_destroy(s);

}

///-------------------------------------------------------------------------------------------------
///  Setup background tests.
///
/// @author Henry Bennett
/// @date 03/22/2011
///-------------------------------------------------------------------------------------------------

int setup_thumbnail_tests() {
	CU_pSuite pSuite = NULL;

	

	// Note: Uncomment this to generate the thumbnails, they should be examined by eye to make sure they are good 
	// before using them in tests.
	// generate_thumbnails();


   /* add a suite to the registry */
   pSuite = CU_add_suite("Thumbnails", NULL, NULL);
   if (NULL == pSuite) {
      CU_cleanup_registry();
      return CU_get_error();
   }

   /* add the tests to the background scanning suite */
   if (
	   NULL == CU_add_test(pSuite, "Test thumbnail API", test_image_reading) ||
	   NULL == CU_add_test(pSuite, "Test thumbnail creation", test_thumbnailing)
	   )
   {
      CU_cleanup_registry();
      return CU_get_error();
   }

   return 0;

} /* setupbackground_tests() */