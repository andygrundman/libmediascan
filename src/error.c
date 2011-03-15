#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/stat.h>

#include <libmediascan.h>
#include "common.h"

MediaScanError *
error_create(const char *path, enum media_error error_code, const char *error_string)
{
  MediaScanError *e = (MediaScanError *)calloc(sizeof(MediaScanError), 1);
  if (e == NULL) {
    LOG_ERROR("Out of memory for new MediaScanError object\n");
    return NULL;
  }
  
  LOG_LEVEL(9, "new MediaScanError @ %p\n", e);
  
  e->error_code = error_code;
  e->path = path;
  e->error_string = error_string;
  
  return e;
}

void
error_destroy(MediaScanError *e)
{
  LOG_LEVEL(9, "destroy MediaScanError @ %p\n", e);
  
  free(e);
}
