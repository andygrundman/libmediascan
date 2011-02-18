#include <libmediascan.h>
#include "common.h"

/**
 * Create a new MediaScanProgress instance
 */
MediaScanProgress *
progress_create(void)
{
  MediaScanProgress *p = (MediaScanProgress *)calloc(sizeof(MediaScanProgress), 1);
  if (p == NULL) {
    LOG_ERROR("Out of memory for new MediaScanProgress object\n");
    return NULL;
  }
  
  LOG_LEVEL(9, "new MediaScanProgress @ %p\n", p);
  
  p->dir_total = 0;
  p->dir_done = 0;
  p->file_total = 0;
  p->file_done = 0;
  p->eta = 0;
  p->rate = 0.0;
  
  return p;
}

void
progress_destroy(MediaScanProgress *p)
{
  LOG_LEVEL(9, "destroy MediaScanProgress @ %p\n", p);
  
  free(p);
}
