
#include <libmediascan.h>
#include <libexif/exif-data.h>
#include <stdlib.h>
#include <setjmp.h>
#include <string.h>

#ifdef WIN32
# define HAVE_BOOLEAN
#endif
#include <jpeglib.h>


#include "common.h"
#include "buffer.h"
#include "image.h"
#include "image_jpeg.h"

#include "libdlna/dlna_internals.h"
#include "libdlna/profiles.h"

#define DEFAULT_JPEG_QUALITY 90

// Forward declarations
static void parse_exif_ifd(ExifContent * content, void *data);
static void parse_exif_entry(ExifEntry * e, void *data);

/* Profile for image media class content of small resolution */
static dlna_profile_t jpeg_sm = {
  "JPEG_SM",
  MIME_IMAGE_JPEG,
  LABEL_IMAGE_PICTURE
};

/* Profile for image media class content of medium resolution */
static dlna_profile_t jpeg_med = {
  "JPEG_MED",
  MIME_IMAGE_JPEG,
  LABEL_IMAGE_PICTURE
};

/* Profile for image media class content of high resolution */
static dlna_profile_t jpeg_lrg = {
  "JPEG_LRG",
  MIME_IMAGE_JPEG,
  LABEL_IMAGE_PICTURE
};

/* Profile for image thumbnails */
static dlna_profile_t jpeg_tn = {
  "JPEG_TN",
  MIME_IMAGE_JPEG,
  LABEL_IMAGE_ICON
};

/* Profile for small icons */
static dlna_profile_t jpeg_sm_ico = {
  "JPEG_SM_ICO",
  MIME_IMAGE_JPEG,
  LABEL_IMAGE_ICON
};

/* Profile for large icons */
static dlna_profile_t jpeg_lrg_ico = {
  "JPEG_LRG_ICO",
  MIME_IMAGE_JPEG,
  LABEL_IMAGE_ICON
};

static const struct {
  dlna_profile_t *profile;
  int max_width;
  int max_height;
} jpeg_profiles_mapping[] = {
  {
  &jpeg_sm_ico, 48, 48}, {
  &jpeg_lrg_ico, 120, 120}, {
  &jpeg_tn, 160, 160}, {
  &jpeg_sm, 640, 480}, {
  &jpeg_med, 1024, 768}, {
  &jpeg_lrg, 4096, 4096}, {
  NULL, 0, 0}
};

// Unfortunately we need a global variable in order to display the filename
// during libjpeg output messages
#define FILENAME_LEN 1024
char Filename[FILENAME_LEN + 1];

typedef struct JPEGData {
  struct jpeg_decompress_struct *cinfo;
  struct jpeg_error_mgr *jpeg_error_pub;
} JPEGData;

jmp_buf setjmp_buffer;

typedef struct buf_src_mgr {
  struct jpeg_source_mgr jsrc;
  Buffer *buf;
  FILE *fp;
} buf_src_mgr;

struct buf_dst_mgr {
  struct jpeg_destination_mgr jdst;
  Buffer *dbuf;
  JOCTET *buf;
  JOCTET *off;
};

// Source manager to read JPEG from buffer
static void buf_src_init(j_decompress_ptr cinfo) {
  // Nothing
}

static boolean buf_src_fill_input_buffer(j_decompress_ptr cinfo) {
  static JOCTET mybuffer[4];
  buf_src_mgr *src = (buf_src_mgr *)cinfo->src;

  // Consume the entire buffer, even if bytes are still in bytes_in_buffer
  buffer_consume(src->buf, buffer_len(src->buf));

  if (!buffer_check_load(src->buf, src->fp, 1, BUF_SIZE))
    goto eof;

  cinfo->src->next_input_byte = (JOCTET *)buffer_ptr(src->buf);
  cinfo->src->bytes_in_buffer = buffer_len(src->buf);

  goto ok;

eof:
  // Insert a fake EOI marker if we can't read enough data
  LOG_DEBUG("  EOF filling input buffer, returning EOI marker\n");

  mybuffer[0] = (JOCTET)0xFF;
  mybuffer[1] = (JOCTET)JPEG_EOI;

  cinfo->src->next_input_byte = mybuffer;
  cinfo->src->bytes_in_buffer = 2;

ok:
  return TRUE;
}

static void buf_src_skip_input_data(j_decompress_ptr cinfo, long num_bytes) {
  buf_src_mgr *src = (buf_src_mgr *)cinfo->src;

  if (num_bytes > 0) {
    LOG_DEBUG("JPEG skip requested: %ld bytes\n", num_bytes);

    while (num_bytes > cinfo->src->bytes_in_buffer) {
      num_bytes -= (long)cinfo->src->bytes_in_buffer;

      // fill_input_buffer will discard the data in the current buffer
      (void)(*cinfo->src->fill_input_buffer) (cinfo);
    }

    // Discard the remaining bytes, taking into account the amount libjpeg has already read
    LOG_DEBUG("  JPEG buffer consume %ld bytes\n", (buffer_len(src->buf) - cinfo->src->bytes_in_buffer) + num_bytes);
    buffer_consume(src->buf, (buffer_len(src->buf) - cinfo->src->bytes_in_buffer) + num_bytes);

    cinfo->src->next_input_byte = (JOCTET *)buffer_ptr(src->buf);
    cinfo->src->bytes_in_buffer = buffer_len(src->buf);
  }
}

static void buf_src_term_source(j_decompress_ptr cinfo) {
  // Nothing
}

static void image_jpeg_buf_src(MediaScanImage *i, MediaScanResult *r) {
  JPEGData *j = (JPEGData *)i->_jpeg;
  j_decompress_ptr cinfo = (j_decompress_ptr)j->cinfo;
  buf_src_mgr *src;

  if (cinfo->src == NULL) {
    cinfo->src = (struct jpeg_source_mgr *)
      (*cinfo->mem->alloc_small) ((j_common_ptr)cinfo, JPOOL_PERMANENT, sizeof(struct buf_src_mgr));
  }

  src = (buf_src_mgr *)cinfo->src;

  src->buf = (Buffer *)r->_buf;
  src->fp = r->_fp;

  src->jsrc.init_source = buf_src_init;
  src->jsrc.fill_input_buffer = buf_src_fill_input_buffer;
  src->jsrc.skip_input_data = buf_src_skip_input_data;
  src->jsrc.resync_to_restart = jpeg_resync_to_restart; // use default
  src->jsrc.term_source = buf_src_term_source;
  src->jsrc.bytes_in_buffer = buffer_len(src->buf);
  src->jsrc.next_input_byte = (JOCTET *)buffer_ptr(src->buf);

  LOG_DEBUG("Init JPEG buffer src, %ld bytes in buffer\n", src->jsrc.bytes_in_buffer);
}

// Destination manager to copy compressed data to a buffer
static void buf_dst_mgr_init(j_compress_ptr cinfo) {
  struct buf_dst_mgr *dst = (void *)cinfo->dest;

  // Temporary internal buffer
  dst->buf = (JOCTET *)malloc(BUF_SIZE);
  LOG_MEM("new JPEG buf @ %p\n", dst->buf);

  // Storage for full compressed data
  dst->dbuf = (Buffer *)malloc(sizeof(Buffer));
  LOG_MEM("new JPEG dbuf @ %p\n", dst->dbuf);
  buffer_init(dst->dbuf, BUF_SIZE);

  dst->off = dst->buf;
  dst->jdst.next_output_byte = dst->off;
  dst->jdst.free_in_buffer = BUF_SIZE;
}

static boolean buf_dst_mgr_empty(j_compress_ptr cinfo) {
  struct buf_dst_mgr *dst = (void *)cinfo->dest;
  void *tmp;

  // Copy tmp buffer to image buffer
  buffer_append(dst->dbuf, dst->buf, BUF_SIZE);

  // Reuse the tmp buffer for the next chunk
  dst->off = dst->buf;
  dst->jdst.next_output_byte = dst->off;
  dst->jdst.free_in_buffer = BUF_SIZE;

  LOG_MEM("buf_dst_mgr_empty, copied %d bytes (total now %d)\n", BUF_SIZE, buffer_len(dst->dbuf));

  return TRUE;
}

static void buf_dst_mgr_term(j_compress_ptr cinfo) {
  struct buf_dst_mgr *dst = (void *)cinfo->dest;

  size_t sz = BUF_SIZE - dst->jdst.free_in_buffer;

  if (sz > 0) {
    // Copy final buffer to image data
    buffer_append(dst->dbuf, dst->buf, sz);
  }

  LOG_MEM("destroy JPEG buf @ %p\n", dst->buf);
  free(dst->buf);

  LOG_MEM("buf_dst_mgr_term, copied final %ld bytes (total bytes %d)\n", sz, buffer_len(dst->dbuf));
}

static void image_jpeg_buf_dest(j_compress_ptr cinfo, struct buf_dst_mgr *dst) {
  memset(dst, 0, sizeof(struct buf_dst_mgr));
  dst->jdst.init_destination = buf_dst_mgr_init;
  dst->jdst.empty_output_buffer = buf_dst_mgr_empty;
  dst->jdst.term_destination = buf_dst_mgr_term;
  cinfo->dest = (void *)dst;
}

static void libjpeg_error_handler(j_common_ptr cinfo) {
  cinfo->err->output_message(cinfo);
  longjmp(setjmp_buffer, 1);
  return;
}

static void libjpeg_output_message(j_common_ptr cinfo) {
  char buffer[JMSG_LENGTH_MAX];

  /* Create the message */
  (*cinfo->err->format_message) (cinfo, buffer);

  LOG_WARN("libjpeg error: %s (%s)\n", buffer, Filename);
}

int image_jpeg_read_header(MediaScanImage *i, MediaScanResult *r) {
  int ret = 1;
  int x;

  JPEGData *j = malloc(sizeof(JPEGData));
  i->_jpeg = (void *)j;
  LOG_MEM("new JPEGData @ %p\n", i->_jpeg);

  j->cinfo = malloc(sizeof(struct jpeg_decompress_struct));
  j->jpeg_error_pub = malloc(sizeof(struct jpeg_error_mgr));
  LOG_MEM("new JPEG cinfo @ %p\n", j->cinfo);
  LOG_MEM("new JPEG error_pub @ %p\n", j->jpeg_error_pub);

  j->cinfo->err = jpeg_std_error(j->jpeg_error_pub);
  j->jpeg_error_pub->error_exit = libjpeg_error_handler;
  j->jpeg_error_pub->output_message = libjpeg_output_message;

  if (setjmp(setjmp_buffer)) {
    image_jpeg_destroy(i);
    return 0;
  }

  // Save filename in case any warnings/errors occur
  strncpy(Filename, r->path, FILENAME_LEN);
  if (strlen(r->path) > FILENAME_LEN)
    Filename[FILENAME_LEN] = 0;

  jpeg_create_decompress(j->cinfo);

  // Init custom source manager to read from existing buffer
  image_jpeg_buf_src(i, r);

  // Save APP1 marker for EXIF
  jpeg_save_markers(j->cinfo, 0xE1, 1024 * 64);

  jpeg_read_header(j->cinfo, TRUE);

  i->width = j->cinfo->image_width;
  i->height = j->cinfo->image_height;
  i->channels = j->cinfo->num_components;
  r->mime_type = MIME_IMAGE_JPEG;

  // Match with DLNA profile
  for (x = 0; jpeg_profiles_mapping[x].profile; x++) {
    if (i->width <= jpeg_profiles_mapping[x].max_width && i->height <= jpeg_profiles_mapping[x].max_height) {
      r->dlna_profile = jpeg_profiles_mapping[x].profile->id;
      break;
    }
  }

  // Process Exif tag
  if (j->cinfo->marker_list != NULL) {
    jpeg_saved_marker_ptr marker = j->cinfo->marker_list;

    while (marker != NULL) {
      if (marker->marker == 0xE1
          && marker->data[0] == 'E' && marker->data[1] == 'x' && marker->data[2] == 'i' && marker->data[3] == 'f') {
        ExifData *exif;

        LOG_DEBUG("Parsing EXIF tag of size %d\n", marker->data_length);
        exif = exif_data_new_from_data(marker->data, marker->data_length);
        LOG_MEM("new EXIF data @ %p\n", exif);
        if (exif != NULL) {
          exif_data_foreach_content(exif, parse_exif_ifd, (void *)i);
          LOG_MEM("destroy EXIF data @ %p\n", exif);
          exif_data_free(exif);
        }

        break;
      }

      marker = marker->next;
    }
  }

out:
  return ret;
}

int image_jpeg_load(MediaScanImage *i, MediaScanThumbSpec *spec_hint) {
  float scale_factor;
  int x, w, h, ofs;
  unsigned char *line[1], *ptr = NULL;

  JPEGData *j = (JPEGData *)i->_jpeg;

  if (setjmp(setjmp_buffer)) {
    // See if we have partially decoded an image and hit a fatal error, but still have a usable image
    if (ptr != NULL) {
      LOG_MEM("destroy JPEG load ptr @ %p\n", ptr);
      free(ptr);
      ptr = NULL;
    }

    if (j->cinfo->output_scanline > 0) {
      LOG_DEBUG("Fatal error but already processed %d scanlines, continuing...\n", j->cinfo->output_scanline);
      return 1;
    }

    image_jpeg_destroy(i);
    return 0;
  }

  // Abort on progressive JPEGs if memory_limit is in use,
  // as progressive JPEGs can use many MBs of memory and there
  // is no other easy way to alter libjpeg's memory use
  /* XXX
     if (i->memory_limit && j->cinfo->progressive_mode) {
     LOG_WARN("libmediascan will not decode progressive JPEGs when memory_limit is in use (%s)\n", i->path);
     image_jpeg_destroy(i);
     return 0;
     }
   */

  // XXX If reusing the object a second time, we need to read the header again

  j->cinfo->do_fancy_upsampling = FALSE;
  j->cinfo->do_block_smoothing = FALSE;

  // Choose optimal scaling factor
  jpeg_calc_output_dimensions(j->cinfo);
  scale_factor = (float)j->cinfo->output_width / spec_hint->width;
  if (scale_factor > ((float)j->cinfo->output_height / spec_hint->height))
    scale_factor = (float)j->cinfo->output_height / spec_hint->height;
  j->cinfo->scale_denom *= (unsigned int)scale_factor;
  jpeg_calc_output_dimensions(j->cinfo);

  w = j->cinfo->output_width;
  h = j->cinfo->output_height;

  // Change the original values to the scaled size
  i->width = w;
  i->height = h;

  LOG_DEBUG("Using JPEG scale factor %d/%d, new source dimensions %d x %d\n",
            j->cinfo->scale_num, j->cinfo->scale_denom, w, h);

  // Save filename in case any warnings/errors occur
  strncpy(Filename, i->path, FILENAME_LEN);
  if (strlen(i->path) > FILENAME_LEN)
    Filename[FILENAME_LEN] = 0;

  // Note: I tested libjpeg-turbo's JCS_EXT_XBGR but it writes zeros
  // instead of FF for alpha, doesn't support CMYK, etc

  jpeg_start_decompress(j->cinfo);

  // Allocate storage for decompressed image
  image_alloc_pixbuf(i, w, h);

  ofs = 0;

  ptr = (unsigned char *)malloc(w * j->cinfo->output_components);
  line[0] = ptr;
  LOG_MEM("new JPEG load ptr @ %p\n", ptr);

  if (j->cinfo->output_components == 3) { // RGB
    while (j->cinfo->output_scanline < j->cinfo->output_height) {
      jpeg_read_scanlines(j->cinfo, line, 1);
      for (x = 0; x < w; x++) {
        i->_pixbuf[ofs++] = COL(ptr[x + x + x], ptr[x + x + x + 1], ptr[x + x + x + 2]);
      }
    }
  }
  else if (j->cinfo->output_components == 4) {  // CMYK inverted (Photoshop)
    while (j->cinfo->output_scanline < j->cinfo->output_height) {
      JSAMPROW row = *line;
      jpeg_read_scanlines(j->cinfo, line, 1);
      for (x = 0; x < w; x++) {
        int c = *row++;
        int m = *row++;
        int y = *row++;
        int k = *row++;

        i->_pixbuf[ofs++] = COL((c * k) / MAXJSAMPLE, (m * k) / MAXJSAMPLE, (y * k) / MAXJSAMPLE);
      }
    }
  }
  else {                        // grayscale
    while (j->cinfo->output_scanline < j->cinfo->output_height) {
      jpeg_read_scanlines(j->cinfo, line, 1);
      for (x = 0; x < w; x++) {
        i->_pixbuf[ofs++] = COL(ptr[x], ptr[x], ptr[x]);
      }
    }
  }

  LOG_MEM("destroy JPEG load ptr @ %p\n", ptr);
  free(ptr);

  jpeg_finish_decompress(j->cinfo);

  return 1;
}

// Compress the data from i->_pixbuf to i->data.
// Uses libjpeg-turbo if available (JCS_EXTENSIONS) for better performance
int image_jpeg_compress(MediaScanImage *i, MediaScanThumbSpec *spec) {
  struct jpeg_compress_struct cinfo;
  struct jpeg_error_mgr jerr;
  struct buf_dst_mgr dst;
  int quality = spec->jpeg_quality;
  int x;
#ifdef JCS_EXTENSIONS
  JSAMPROW *data = NULL;
#else
  volatile unsigned char *data = NULL;  // volatile = won't be rolled back if longjmp is called
  JSAMPROW row_pointer[1];
  int y, row_stride;
#endif

  if (!i->_pixbuf_size) {
    LOG_WARN("JPEG compression requires pixbuf data\n");
    return 0;
  }

  if (!quality)
    quality = DEFAULT_JPEG_QUALITY;

  cinfo.err = jpeg_std_error(&jerr);
  jpeg_create_compress(&cinfo);
  image_jpeg_buf_dest(&cinfo, &dst);

  cinfo.image_width = i->width;
  cinfo.image_height = i->height;
  cinfo.input_components = 3;
  cinfo.in_color_space = JCS_RGB; // output is always RGB even if source was grayscale

  if (setjmp(setjmp_buffer)) {
    if (data != NULL) {
      LOG_MEM("destroy JPEG data row @ %p\n", data);
      free((void *)data);
    }
    return 0;
  }

#ifdef JCS_EXTENSIONS
  // Use libjpeg-turbo support for direct reading from source buffer
  cinfo.input_components = 4;
  cinfo.in_color_space = JCS_EXT_XBGR;
#endif

  jpeg_set_defaults(&cinfo);
  jpeg_set_quality(&cinfo, quality, TRUE);
  jpeg_start_compress(&cinfo, TRUE);

#ifdef JCS_EXTENSIONS
  data = (JSAMPROW *)malloc(i->height * sizeof(JSAMPROW));
  LOG_MEM("new JPEG data row @ %p\n", data);

  for (x = 0; x < i->height; x++)
    data[x] = (JSAMPROW)&i->_pixbuf[x * i->width];

  while (cinfo.next_scanline < cinfo.image_height) {
    jpeg_write_scanlines(&cinfo, &data[cinfo.next_scanline], cinfo.image_height - cinfo.next_scanline);
  }

#else
  // Normal libjpeg
  row_stride = cinfo.image_width * 3;
  data = (unsigned char *)malloc(row_stride);
  LOG_MEM("new JPEG data row @ %p\n", data);

  y = 0;
  while (cinfo.next_scanline < cinfo.image_height) {
    for (x = 0; x < cinfo.image_width; x++) {
      data[x + x + x] = COL_RED(i->_pixbuf[y]);
      data[x + x + x + 1] = COL_GREEN(i->_pixbuf[y]);
      data[x + x + x + 2] = COL_BLUE(i->_pixbuf[y]);
      y++;
    }
    row_pointer[0] = (unsigned char *)data;
    jpeg_write_scanlines(&cinfo, row_pointer, 1);
  }
#endif

  jpeg_finish_compress(&cinfo);

  LOG_MEM("destroy JPEG data row @ %p\n", data);
  free((void *)data);

  jpeg_destroy_compress(&cinfo);

  // Attach compressed buffer to image
  i->_dbuf = (void *)dst.dbuf;

  return 1;
}

void image_jpeg_destroy(MediaScanImage *i) {
  if (i->_jpeg) {
    JPEGData *j = (JPEGData *)i->_jpeg;

    jpeg_destroy_decompress(j->cinfo);
    LOG_MEM("destroy JPEG cinfo @ %p\n", j->cinfo);
    free(j->cinfo);
    LOG_MEM("destroy JPEG error_pub @ %p\n", j->jpeg_error_pub);
    free(j->jpeg_error_pub);

    LOG_MEM("destroy JPEGData @ %p\n", i->_jpeg);
    free(i->_jpeg);
    i->_jpeg = NULL;
  }
}

/// libexif callbacks

static void parse_exif_ifd(ExifContent * content, void *data) {
  exif_content_foreach_entry(content, parse_exif_entry, data);
}

static void parse_exif_entry(ExifEntry * e, void *data) {
  MediaScanImage *i = (MediaScanImage *)data;
  char val[1024];

  // Get orientation
  if (e->tag == 0x112) {
    ExifByteOrder o = exif_data_get_byte_order(e->parent->parent);
    i->orientation = exif_get_short(e->data, o);
    LOG_DEBUG("Exif orientation: %d\n", i->orientation);
  }

  // XXX store other tags
  LOG_DEBUG("Exif entry: %x (%d bytes) %s: %s\n", e->tag, e->size,
            exif_tag_get_name_in_ifd(e->tag, exif_entry_get_ifd(e)), exif_entry_get_value(e, val, sizeof(val)));
}
