#include <libmediascan.h>

#include <png.h>
#include <setjmp.h>

#include "common.h"
#include "buffer.h"
#include "image.h"
#include "image_png.h"

#include "libdlna/dlna_internals.h"
#include "libdlna/profiles.h"

typedef struct PNGData {
  png_structp png_ptr;
  png_infop info_ptr;
  Buffer *buf;
  FILE *fp;
  const char *path;
} PNGData;

/* Profile for image thumbnails */
static dlna_profile_t png_tn = {
  "PNG_TN", MIME_IMAGE_PNG, LABEL_IMAGE_ICON
};

/* Profile for small icons */
static dlna_profile_t png_sm_ico = {
  "PNG_SM_ICO",  MIME_IMAGE_PNG, LABEL_IMAGE_ICON
};

/* Profile for large icons */
static dlna_profile_t png_lrg_ico = {
  "PNG_LRG_ICO", MIME_IMAGE_PNG, LABEL_IMAGE_ICON
};

/* Profile for image class content of high resolution */
static dlna_profile_t png_lrg = {
  "PNG_LRG", MIME_IMAGE_PNG, LABEL_IMAGE_PICTURE
};

static const struct {
  dlna_profile_t *profile;
  int max_width;
  int max_height;
} png_profiles_mapping[] = {
  { &png_sm_ico,    48,   48 },
  { &png_lrg_ico,  120,  120 },
  { &png_tn,       160,  160 },
  { &png_lrg,     4096, 4096 },
  { NULL, 0, 0 }
};

static void
image_png_error(png_structp png_ptr, png_const_charp error_msg)
{
  PNGData *p = (PNGData *)png_get_error_ptr(png_ptr);
  
  LOG_WARN("libpng error: %s (%s)\n", error_msg, p->path);
  
  longjmp(png_jmpbuf(png_ptr), 1);
}

static void
image_png_warning(png_structp png_ptr, png_const_charp warning_msg)
{
  PNGData *p = (PNGData *)png_get_error_ptr(png_ptr);
  
  LOG_WARN("libpng warning: %s (%s)\n", warning_msg, p->path);
}

static void
image_png_read_buf(png_structp png_ptr, png_bytep data, png_size_t len)
{
  PNGData *p = (PNGData *)png_get_io_ptr(png_ptr);

  LOG_DEBUG("PNG read_buf wants %ld bytes, %d in buffer\n", len, buffer_len(p->buf));
 
  if ( !buffer_check_load(p->buf, p->fp, len, BUF_SIZE) )
    goto eof;

  png_memcpy(data, buffer_ptr(p->buf), len);
  buffer_consume(p->buf, len);
  goto ok;
 
eof:
  png_error(png_ptr, "Not enough PNG data");
 
ok:
  return;
}

int
image_png_read_header(MediaScanImage *i, MediaScanResult *r)
{
  int x;
  PNGData *p = malloc(sizeof(PNGData));
  i->_png = (void *)p;
  LOG_MEM("new PNGData @ %p\n", i->_png);
  
  p->buf = (Buffer *)r->_buf;
  p->fp = r->_fp;
  p->path = r->path;
  
  p->png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, (png_voidp)p, image_png_error, image_png_warning);
  if (!p->png_ptr)
    FATAL("Could not initialize libpng\n");
  
  p->info_ptr = png_create_info_struct(p->png_ptr);
  if (!p->info_ptr) {
    png_destroy_read_struct(&p->png_ptr, (png_infopp)NULL, (png_infopp)NULL);
    FATAL("Could not initialize libpng\n");
  }
  
  if ( setjmp( png_jmpbuf(p->png_ptr) ) ) {
    image_png_destroy(i);
    return 0;
  }
  
  png_set_read_fn(p->png_ptr, p, image_png_read_buf);
  
  png_read_info(p->png_ptr, p->info_ptr);
  
  i->width     = png_get_image_width(p->png_ptr, p->info_ptr);
  i->height    = png_get_image_height(p->png_ptr, p->info_ptr);
  i->channels  = png_get_channels(p->png_ptr, p->info_ptr);
  i->has_alpha = 1;
  
  // Match with DLNA profile
  for (x = 0; png_profiles_mapping[x].profile; x++) {
    if (i->width  <= png_profiles_mapping[x].max_width &&
        i->height <= png_profiles_mapping[x].max_height) {
          r->dlna_profile = png_profiles_mapping[x].profile->id;
          r->mime_type    = png_profiles_mapping[x].profile->mime;
          break;
    }
  }
  
  return 1;
}

static void
image_png_interlace_pass_gray(MediaScanImage *i, unsigned char *ptr, int start_y, int stride_y, int start_x, int stride_x)
{
  int x, y;
  PNGData *p = (PNGData *)i->_png;
  
  for (y = 0; y < i->height; y++) {
    png_read_row(p->png_ptr, ptr, NULL);
    if (start_y == 0) {
      start_y = stride_y;
      for (x = start_x; x < i->width; x += stride_x) {
        i->_pixbuf[y * i->width + x] = COL_FULL(
          ptr[x * 2], ptr[x * 2], ptr[x * 2], ptr[x * 2 + 1]
        );
      }
    }
    start_y--;
  }
}

static void
image_png_interlace_pass(MediaScanImage *i, unsigned char *ptr, int start_y, int stride_y, int start_x, int stride_x)
{
  int x, y;
  PNGData *p = (PNGData *)i->_png;
  
  for (y = 0; y < i->height; y++) {
    png_read_row(p->png_ptr, ptr, NULL);
    if (start_y == 0) {
      start_y = stride_y;
      for (x = start_x; x < i->width; x += stride_x) {
        i->_pixbuf[y * i->width + x] = COL_FULL(
          ptr[x * 4], ptr[x * 4 + 1], ptr[x * 4 + 2], ptr[x * 4 + 3]
        );
      }
    }
    start_y--;
  }
}

int
image_png_load(MediaScanImage *i)
{
  int bit_depth, color_type, num_passes, x, y;
  int ofs;
  volatile unsigned char *ptr = NULL; // volatile = won't be rolled back if longjmp is called
  PNGData *p = (PNGData *)i->_png;
  
  if ( setjmp( png_jmpbuf(p->png_ptr) ) ) {
    if (ptr != NULL)
      free((void *)ptr);
    image_png_destroy(i);
    return 0;
  }
  
  // XXX If reusing the object a second time, we need to completely create a new png struct
  
  bit_depth  = png_get_bit_depth(p->png_ptr, p->info_ptr);
  color_type = png_get_color_type(p->png_ptr, p->info_ptr);
  
  if (color_type == PNG_COLOR_TYPE_PALETTE) {
    png_set_expand(p->png_ptr); // png_set_palette_to_rgb(p->png_ptr);
    i->channels = 4;
  }
  else if (color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8)
    png_set_expand(p->png_ptr); // png_set_expand_gray_1_2_4_to_8(p->png_ptr);
  else if (png_get_valid(p->png_ptr, p->info_ptr, PNG_INFO_tRNS))
    png_set_expand(p->png_ptr); // png_set_tRNS_to_alpha(p->png_ptr);
  
  if (bit_depth == 16)
    png_set_strip_16(p->png_ptr);
  else if (bit_depth < 8)
    png_set_packing(p->png_ptr);
  
  // Make non-alpha RGB/Palette 32-bit and Gray 16-bit for easier handling
  if ( !(color_type & PNG_COLOR_MASK_ALPHA) ) {
    png_set_add_alpha(p->png_ptr, 0xFF, PNG_FILLER_AFTER);
  }
  
  num_passes = png_set_interlace_handling(p->png_ptr);
  
  LOG_DEBUG("png bit_depth %d, color_type %d, channels %d, num_passes %d\n", bit_depth, color_type, i->channels, num_passes);
  
  png_read_update_info(p->png_ptr, p->info_ptr);
  
  image_alloc_pixbuf(i, i->width, i->height);
  
  ptr = (unsigned char *)malloc(png_get_rowbytes(p->png_ptr, p->info_ptr));
  
  ofs = 0;
  
  if (color_type == PNG_COLOR_TYPE_GRAY || color_type == PNG_COLOR_TYPE_GRAY_ALPHA) { // Grayscale (Alpha)
    if (num_passes == 1) { // Non-interlaced
      for (y = 0; y < i->height; y++) {
        png_read_row(p->png_ptr, (unsigned char *)ptr, NULL);
        for (x = 0; x < i->width; x++) {
  			  i->_pixbuf[ofs++] = COL_FULL(ptr[x * 2], ptr[x * 2], ptr[x * 2], ptr[x * 2 + 1]);
  			}
      }
    }
    else if (num_passes == 7) { // Interlaced
      image_png_interlace_pass_gray(i, (unsigned char *)ptr, 0, 8, 0, 8);
      image_png_interlace_pass_gray(i, (unsigned char *)ptr, 0, 8, 4, 8);
      image_png_interlace_pass_gray(i, (unsigned char *)ptr, 4, 8, 0, 4);
      image_png_interlace_pass_gray(i, (unsigned char *)ptr, 0, 4, 2, 4);
      image_png_interlace_pass_gray(i, (unsigned char *)ptr, 2, 4, 0, 2);
      image_png_interlace_pass_gray(i, (unsigned char *)ptr, 0, 2, 1, 2);
      image_png_interlace_pass_gray(i, (unsigned char *)ptr, 1, 2, 0, 1);
    }
  }
  else { // RGB(A)
    if (num_passes == 1) { // Non-interlaced
      for (y = 0; y < i->height; y++) {
        png_read_row(p->png_ptr, (unsigned char *)ptr, NULL);
        for (x = 0; x < i->width; x++) {
  			  i->_pixbuf[ofs++] = COL_FULL(ptr[x * 4], ptr[x * 4 + 1], ptr[x * 4 + 2], ptr[x * 4 + 3]);
  			}
      }
    }
    else if (num_passes == 7) { // Interlaced
      // The first pass will return an image 1/8 as wide as the entire image
      // (every 8th column starting in column 0)
      // and 1/8 as high as the original (every 8th row starting in row 0)
      image_png_interlace_pass(i, (unsigned char *)ptr, 0, 8, 0, 8);
    
      // The second will be 1/8 as wide (starting in column 4)
      // and 1/8 as high (also starting in row 0)
      image_png_interlace_pass(i, (unsigned char *)ptr, 0, 8, 4, 8);
    
      // The third pass will be 1/4 as wide (every 4th pixel starting in column 0)
      // and 1/8 as high (every 8th row starting in row 4)
      image_png_interlace_pass(i, (unsigned char *)ptr, 4, 8, 0, 4);
    
      // The fourth pass will be 1/4 as wide and 1/4 as high
      // (every 4th column starting in column 2, and every 4th row starting in row 0)
      image_png_interlace_pass(i, (unsigned char *)ptr, 0, 4, 2, 4);
    
      // The fifth pass will return an image 1/2 as wide,
      // and 1/4 as high (starting at column 0 and row 2)
      image_png_interlace_pass(i, (unsigned char *)ptr, 2, 4, 0, 2);
    
      // The sixth pass will be 1/2 as wide and 1/2 as high as the original
      // (starting in column 1 and row 0)
      image_png_interlace_pass(i, (unsigned char *)ptr, 0, 2, 1, 2);
    
      // The seventh pass will be as wide as the original, and 1/2 as high,
      // containing all of the odd numbered scanlines.
      image_png_interlace_pass(i, (unsigned char *)ptr, 1, 2, 0, 1);
    }
    else {
      FATAL("Unsupported PNG interlace type (%d passes)\n", num_passes);
    }
  }
  
  free(ptr);
  
  // This is not required, so we can save some time by not reading post-image chunks
  //png_read_end(p->png_ptr, p->info_ptr);
  
  return 1;
}

void
image_png_destroy(MediaScanImage *i)
{
  if (i->_png) {
    PNGData *p = (PNGData *)i->_png;
    
    png_destroy_read_struct(&p->png_ptr, &p->info_ptr, NULL);
      
    LOG_MEM("destroy PNGData @ %p\n", i->_png);
    free(i->_png);
    i->_png = NULL;
  }
}
