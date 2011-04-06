


#include <libmediascan.h>

#ifdef WIN32
#include "mediascan_win32.h"
#endif


#ifdef _MSC_VER 
#pragma warning( disable: 4244 ) 
#endif

#include <libavformat/avformat.h>

#ifdef _MSC_VER
#pragma warning( default: 4244 )
#endif


#include "common.h"
#include "video.h"
#include "error.h"



MediaScanVideo *
video_create(void)
{
  MediaScanVideo *v = (MediaScanVideo *)calloc(sizeof(MediaScanVideo), 1);
  if (v == NULL) {
	ms_errno = MSENO_MEMERROR;
    FATAL("Out of memory for new MediaScanVideo object\n");
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