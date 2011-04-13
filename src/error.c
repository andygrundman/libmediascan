#include <fcntl.h>
#include <stdlib.h>
#include <string.h>

#ifndef WIN32
#include <strings.h>
#endif

#include <sys/stat.h>

#include <libmediascan.h>
#include "common.h"

MediaScanError *error_create(const char *path, enum media_error error_code, const char *error_string) {
  MediaScanError *e = (MediaScanError *)calloc(sizeof(MediaScanError), 1);
  if (e == NULL) {
    FATAL("Out of memory for new MediaScanError object\n");
    return NULL;
  }

  LOG_MEM("new MediaScanError @ %p\n", e);

  e->error_code = error_code;
  e->averror = 0;
  e->path = path;
  e->error_string = error_string;

  return e;
}

void error_destroy(MediaScanError *e) {
  LOG_MEM("destroy MediaScanError @ %p\n", e);

  free(e);
}
