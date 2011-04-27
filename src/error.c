#include <fcntl.h>
#include <stdlib.h>
#include <string.h>

#ifndef WIN32
#include <strings.h>
#endif

#include <sys/stat.h>

#include <libmediascan.h>
#include "common.h"

MediaScanError *error_create(const char *tmp_full_path, enum media_error error_code, const char *tmp_error_string) {
  MediaScanError *e = (MediaScanError *)calloc(sizeof(MediaScanError), 1);
  if (e == NULL) {
    FATAL("Out of memory for new MediaScanError object\n");
    return NULL;
  }

  LOG_MEM("new MediaScanError @ %p\n", e);

  e->error_code = error_code;
  e->averror = 0;
  e->path = strdup(tmp_full_path);
  e->error_string = strdup(tmp_error_string);

  return e;
}

// Copy an error instance. Pass the copy to error_destroy
// when done.
MediaScanError *error_copy(MediaScanError *e) {
  MediaScanError *ecopy = malloc(sizeof(MediaScanError));
  memcpy(ecopy, e, sizeof(MediaScanError));

  ecopy->path = strdup(e->path);
  ecopy->error_string = strdup(e->error_string);

  LOG_MEM("copy MediaScanError @ %p -> %p\n", e, ecopy);

  return ecopy;
}

void error_destroy(MediaScanError *e) {
  LOG_MEM("destroy MediaScanError @ %p\n", e);

  free(e->error_string);
  free(e->path);
  free(e);
}
