#include <stdlib.h>
#include <string.h>

#include <libmediascan.h>

#include "common.h"
#include "tag_item.h"

MediaScanTagItem *tag_item_create(const char *key, const char *value) {
  MediaScanTagItem *ti = (MediaScanTagItem *) calloc(sizeof(MediaScanTagItem), 1);
  if (ti == NULL) {
    ms_errno = MSENO_MEMERROR;
    FATAL("Out of memory for new MediaScanTagItem object\n");
    return NULL;
  }

  ti->key = strdup(key);
  ti->value = strdup(value);

  LOG_MEM("new MediaScanTagItem @ %p\n", ti);
  return ti;
}

void tag_item_destroy(MediaScanTagItem * ti) {
  free(ti->key);
  free(ti->value);

  LOG_MEM("destroy MediaScanTagItem @ %p\n", ti);
  free(ti);
}
