

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
#include "audio.h"
#include "error.h"

MediaScanAudio *
audio_create(void)
{
  MediaScanAudio *a = (MediaScanAudio *)calloc(sizeof(MediaScanAudio), 1);
  if (a == NULL) {
    FATAL("Out of memory for new MediaScanAudio object\n");
    return NULL;
  }
  
  LOG_MEM("new MediaScanAudio @ %p\n", a);
  
  return a;
}

void
audio_destroy(MediaScanAudio *a)
{
  LOG_MEM("destroy MediaScanAudio @ %p\n", a);
  free(a);
}
