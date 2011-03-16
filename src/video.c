#include <libavformat/avformat.h>

#include <libmediascan.h>
#include "common.h"
#include "video.h"
#include "error.h"

MediaScanVideo *
video_create(void)
{
  MediaScanVideo *v = (MediaScanVideo *)calloc(sizeof(MediaScanVideo), 1);
  if (v == NULL) {
    LOG_ERROR("Out of memory for new MediaScanVideo object\n");
    return NULL;
  }
  
  LOG_MEM("new MediaScanVideo @ %p\n", v);
  
  v->codec = NULL;
  v->width = 0;
  v->height = 0;
  
  v->streams = NULL;
  v->thumbnails = NULL;
  v->tags = NULL;
  
  return v;
}

void
video_destroy(MediaScanVideo *v)
{
  LOG_MEM("destroy MediaScanVideo @ %p\n", v);
  free(v);
}