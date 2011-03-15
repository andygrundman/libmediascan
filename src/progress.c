///-------------------------------------------------------------------------------------------------
// file:	libmediascan\src\progress.c
//
// summary:	MediaScanProgress class
///-------------------------------------------------------------------------------------------------

#ifdef WIN32
#include "win32/include/config.h"
#endif

#include <stdlib.h>

#include <libmediascan.h>
#include "common.h"

///-------------------------------------------------------------------------------------------------
/// <summary>	Create a new MediaScanProgress instance </summary>
///
/// <remarks>	 </remarks>
///
/// <returns>	null if it fails, else. </returns>
///-------------------------------------------------------------------------------------------------

MediaScanProgress *progress_create(void)
{
  MediaScanProgress *p = (MediaScanProgress *)calloc(sizeof(MediaScanProgress), 1);
  if (p == NULL) {
    LOG_ERROR("Out of memory for new MediaScanProgress object\n");
    return NULL;
  }
  
  LOG_LEVEL(9, "new MediaScanProgress @ %p\n", p);
  
  p->phase = NULL;
  p->cur_item = NULL;
  
  p->dir_total = 0;
  p->dir_done = 0;
  p->file_total = 0;
  p->file_done = 0;
  p->eta = 0;
  p->rate = 0.0;
  
  p->_last_callback = 0;
  
  return p;
}

///-------------------------------------------------------------------------------------------------
/// <summary>	Destroy a MediaScanProgress instance </summary>
///
/// <remarks>	</remarks>
///
/// <param name="p">	[in,out] If non-null, the. </param>
///-------------------------------------------------------------------------------------------------

void progress_destroy(MediaScanProgress *p)
{
  LOG_LEVEL(9, "destroy MediaScanProgress @ %p\n", p);
  
  free(p);
}
