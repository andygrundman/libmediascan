
#include <libmediascan.h>

#ifdef WIN32
#include "mediascan_win32.h"
#endif

#include "common.h"
#include "image.h"
#include "error.h"
#include "image_jpeg.h"

MediaScanImage *
image_create(void)
{
  MediaScanImage *i = (MediaScanImage *)calloc(sizeof(MediaScanImage), 1);
  if (i == NULL) {
    ms_errno = MSENO_MEMERROR;
    FATAL("Out of memory for new MediaScanImage object\n");
    return NULL;
  }
  
  LOG_MEM("new MediaScanImage @ %p\n", i);
  
  return i;
}

void
image_destroy(MediaScanImage *i)
{
  if (i->_jpeg)
    image_jpeg_destroy(i);
  
  LOG_MEM("destroy MediaScanImage @ %p\n", i);
  free(i);
}

int
image_read_header(MediaScanImage *i, MediaScanResult *r)
{
  unsigned char *bptr;
  int ret = 1;
  
  bptr = buffer_ptr((Buffer *)r->_buf);
  
  // Determine image type and basic details
  switch (bptr[0]) {
    case 0xff:
      if (bptr[1] == 0xd8 && bptr[2] == 0xff) {
        i->codec = "JPEG";
        if ( !image_jpeg_read_header(i, r) ) {
          ret = 0;
          goto out;
        }
      }
      break;
    case 0x89:
      if (bptr[1] == 'P' && bptr[2] == 'N' && bptr[3] == 'G'
        && bptr[4] == 0x0d && bptr[5] == 0x0a && bptr[6] == 0x1a && bptr[7] == 0x0a) {
          i->codec = "PNG";
      }
      break;
    case 'G':
      if (bptr[1] == 'I' && bptr[2] == 'F' && bptr[3] == '8'
        && (bptr[4] == '7' || bptr[4] == '9') && bptr[5] == 'a') {
          i->codec = "GIF";
      }
      break;
    case 'B':
      if (bptr[1] == 'M') {
        i->codec = "BMP";
      }
      break;
  }
  
  if ( !i->codec ) {
    ret = 0;
    goto out;
  }
  
  // XXX create thumbnail(s) if requested
  
out:
  return ret;
}
