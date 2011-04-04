#include <libmediascan.h>

#include <jpeglib.h>
#include <libexif/exif-data.h>

#include <setjmp.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "buffer.h"
#include "image_jpeg.h"

#include "libdlna/dlna_internals.h"
#include "libdlna/profiles.h"

#define DEFAULT_JPEG_QUALITY 90

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
  { &jpeg_sm_ico,    48,   48 },
  { &jpeg_lrg_ico,  120,  120 },
  { &jpeg_tn,       160,  160 },
  { &jpeg_sm,       640,  480 },
  { &jpeg_med,     1024,  768 },
  { &jpeg_lrg,     4096, 4096 },
  { NULL, 0, 0 }
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

// Source manager to read JPEG from buffer
static void
buf_src_init(j_decompress_ptr cinfo)
{
  // Nothing
}

static boolean
buf_src_fill_input_buffer(j_decompress_ptr cinfo)
{
  static JOCTET mybuffer[4];
  buf_src_mgr *src = (buf_src_mgr *)cinfo->src;
  
  LOG_DEBUG("JPEG fill_input_buffer\n");
  
  // Consume the entire buffer, even if bytes are still in bytes_in_buffer
  buffer_consume(src->buf, buffer_len(src->buf));
  
  if ( !buffer_check_load(src->buf, src->fp, 1, BUF_SIZE) )
    goto eof;
  
  cinfo->src->next_input_byte = (JOCTET *)buffer_ptr(src->buf);
  cinfo->src->bytes_in_buffer = buffer_len(src->buf);
  
  goto ok;
  
eof:
  // Insert a fake EOI marker if we can't read enough data
  LOG_DEBUG("  EOF, returning EOI marker\n");

  mybuffer[0] = (JOCTET) 0xFF;
  mybuffer[1] = (JOCTET) JPEG_EOI;
  
  cinfo->src->next_input_byte = mybuffer;
  cinfo->src->bytes_in_buffer = 2;

ok:
  return TRUE;
}

static void
buf_src_skip_input_data(j_decompress_ptr cinfo, long num_bytes)
{
  buf_src_mgr *src = (buf_src_mgr *)cinfo->src;
  
  if (num_bytes > 0) {
    LOG_DEBUG("JPEG skip requested: %ld bytes\n", num_bytes);

    while (num_bytes > cinfo->src->bytes_in_buffer) {
      num_bytes -= (long)cinfo->src->bytes_in_buffer;

      // fill_input_buffer will discard the data in the current buffer
      (void) (*cinfo->src->fill_input_buffer)(cinfo);
    }
    
    // Discard the remaining bytes, taking into account the amount libjpeg has already read
    LOG_DEBUG("  JPEG buffer consume %ld bytes\n", (buffer_len(src->buf) - cinfo->src->bytes_in_buffer) + num_bytes);
    buffer_consume(src->buf, (buffer_len(src->buf) - cinfo->src->bytes_in_buffer) + num_bytes);
    
    cinfo->src->next_input_byte = (JOCTET *)buffer_ptr(src->buf);
    cinfo->src->bytes_in_buffer = buffer_len(src->buf);
  }
}

static void
buf_src_term_source(j_decompress_ptr cinfo)
{
  // Nothing
}

static void
image_jpeg_buf_src(MediaScanImage *i, MediaScanResult *r)
{
  JPEGData *j = (JPEGData *)i->_jpeg;
  j_decompress_ptr cinfo = (j_decompress_ptr)j->cinfo;
  buf_src_mgr *src;
  
  if (cinfo->src == NULL) {
    cinfo->src = (struct jpeg_source_mgr *)
      (*cinfo->mem->alloc_small)((j_common_ptr)cinfo, JPOOL_PERMANENT, sizeof(struct buf_src_mgr));
  }
  
  src = (buf_src_mgr *)cinfo->src;
  
  src->buf = (Buffer *)r->_buf;
  src->fp  = r->_fp;
  
  src->jsrc.init_source       = buf_src_init;
  src->jsrc.fill_input_buffer = buf_src_fill_input_buffer;
  src->jsrc.skip_input_data   = buf_src_skip_input_data;
  src->jsrc.resync_to_restart = jpeg_resync_to_restart; // use default
  src->jsrc.term_source       = buf_src_term_source;
  src->jsrc.bytes_in_buffer   = buffer_len(src->buf);
  src->jsrc.next_input_byte   = (JOCTET *)buffer_ptr(src->buf);
  
  LOG_DEBUG("Init JPEG buffer src, %ld bytes in buffer\n", src->jsrc.bytes_in_buffer);
}

static void
libjpeg_error_handler(j_common_ptr cinfo)
{
  cinfo->err->output_message(cinfo);
  longjmp(setjmp_buffer, 1);
  return;
}

static void
libjpeg_output_message(j_common_ptr cinfo)
{
  char buffer[JMSG_LENGTH_MAX];

  /* Create the message */
  (*cinfo->err->format_message) (cinfo, buffer);
  
  warn("libjpeg error: %s (%s)\n", buffer, Filename);
}

int
image_jpeg_read_header(MediaScanImage *i, MediaScanResult *r)
{
  int ret = 1;
  int x;
  
  JPEGData *j = malloc(sizeof(JPEGData));
  i->_jpeg = (void *)j;
  LOG_MEM("new JPEGData @ %p\n", i->_jpeg);
  
  j->cinfo = malloc(sizeof(struct jpeg_decompress_struct));
  j->jpeg_error_pub = malloc(sizeof(struct jpeg_error_mgr));
  
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
  
  i->width    = j->cinfo->image_width;
  i->height   = j->cinfo->image_height;
  i->channels = j->cinfo->num_components;
  
  // Match with DLNA profile
  for (x = 0; jpeg_profiles_mapping[x].profile; x++) {
    if (i->width  <= jpeg_profiles_mapping[x].max_width &&
        i->height <= jpeg_profiles_mapping[x].max_height) {
          r->dlna_profile = jpeg_profiles_mapping[x].profile->id;
          r->mime_type    = jpeg_profiles_mapping[x].profile->mime;
          break;
    }
  }
  
  // Process Exif tag
  if (j->cinfo->marker_list != NULL) {
    jpeg_saved_marker_ptr marker = j->cinfo->marker_list;

    while (marker != NULL) {      
      if (marker->marker == 0xE1 
        && marker->data[0] == 'E' && marker->data[1] == 'x'
        && marker->data[2] == 'i' && marker->data[3] == 'f'
      ) {
        ExifData *exif;
        
        LOG_DEBUG("Parsing EXIF tag of size %d\n", marker->data_length);
        exif = exif_data_new_from_data(marker->data, marker->data_length);
        if (exif != NULL) {
          // XXX get all tags or only important ones?  
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

void
image_jpeg_destroy(MediaScanImage *i)
{
  if (i->_jpeg) {
    JPEGData *j = (JPEGData *)i->_jpeg;

    jpeg_destroy_decompress(j->cinfo);
    free(j->cinfo);    
    free(j->jpeg_error_pub);
  
    LOG_MEM("destroy JPEGData @ %p\n", i->_jpeg);
    free(i->_jpeg);
    i->_jpeg = NULL;
  }
}
