///-------------------------------------------------------------------------------------------------
// file:	libmediascan\src\progress.c
//
// summary:	MediaScanProgress class
///-------------------------------------------------------------------------------------------------

#ifdef WIN32
#include "win32/include/win32config.h"
#endif

#include <stdlib.h>

#include <libmediascan.h>
#include "common.h"

///-------------------------------------------------------------------------------------------------
///  Create a new MediaScanProgress instance.
///
/// @author Henry Bennett
/// @date 03/15/2011
///
/// @return null if it fails, else.
///
/// ### remarks .
///-------------------------------------------------------------------------------------------------

MediaScanProgress *progress_create(void)
{
  MediaScanProgress *p = (MediaScanProgress *)calloc(sizeof(MediaScanProgress), 1);
  if (p == NULL) {
	ms_errno = MSENO_MEMERROR;
    FATAL("Out of memory for new MediaScanProgress object\n");
    return NULL;
  }
  
  LOG_MEM("new MediaScanProgress @ %p\n", p);
  
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
///  Destroy a MediaScanProgress instance.
///
/// @author Henry Bennett
/// @date 03/15/2011
///
/// @param [in,out] p If non-null, the.
///
/// ### remarks .
///-------------------------------------------------------------------------------------------------

void progress_destroy(MediaScanProgress *p)
{
  LOG_MEM("destroy MediaScanProgress @ %p\n", p);
  
  free(p);
}
