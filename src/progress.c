#include <stdarg.h>
#include <stdlib.h>
#include <sys/time.h>

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
    FATAL("Out of memory for new MediaScanProgress object\n");
    return NULL;
  }
  
  p->interval = 1;
  p->rate = -1;
  p->eta = -1;
  
  LOG_MEM("new MediaScanProgress @ %p\n", p);
  
  return p;
}

void
progress_start_phase(MediaScanProgress *p, const char *fmt, ...)
{
  char *phase = (char *)malloc((size_t)PathMax);
  struct timeval now;
  va_list ap;
  
  if (p->phase)
    free(p->phase);
  
  va_start(ap, fmt);  
  vsprintf(phase, fmt, ap);
  va_end(ap);
  
  p->phase = phase;
  
  gettimeofday(&now, NULL);
  p->_start_ts = now.tv_sec;
}

// Returns 1 if progress was updated and callback should be called
int
progress_update(MediaScanProgress *p, const char *cur_item)
{
  struct timeval now;
  
  LOG_DEBUG("progress_update %s\n", cur_item);
  
  gettimeofday(&now, NULL);
  
  if (now.tv_sec - p->_last_update_ts >= p->interval) {
    int elapsed = now.tv_sec - p->_start_ts;
    
    if (elapsed > 0) {
      p->rate = (int)((p->done / elapsed) + 0.5);
      if (p->total && p->rate > 0)
        p->eta = (int)(((p->total - p->done) / p->rate) + 0.5);
    }
    
    p->cur_item = cur_item;
    p->_last_update_ts = now.tv_sec;
    
    return 1;
  }
  
  return 0;
}

void
progress_destroy(MediaScanProgress *p)
{
  if (p->phase)
    free(p->phase);
  
  LOG_MEM("destroy MediaScanProgress @ %p\n", p);  
  free(p);
}
