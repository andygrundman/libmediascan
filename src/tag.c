#include <stdlib.h>

#include <libmediascan.h>

#include "common.h"
#include "tag.h"
#include "tag_item.h"

MediaScanTag *tag_create(const char *type) {
  MediaScanTag *t = (MediaScanTag *)calloc(sizeof(MediaScanTag), 1);
  if (t == NULL) {
    ms_errno = MSENO_MEMERROR;
    FATAL("Out of memory for new MediaScanTag object\n");
    return NULL;
  }

  t->type = type;
  t->nitems = 0;

  LOG_MEM("new MediaScanTag @ %p\n", t);
  return t;
}

void tag_destroy(MediaScanTag *t) {
  int i;

  // free items
  if (t->nitems) {
    for (i = 0; i < t->nitems; i++)
      tag_item_destroy(t->items[i]);
  }

  LOG_MEM("destroy MediaScanTag @ %p\n", t);
  free(t);
}

void tag_add_item(MediaScanTag *t, const char *key, const char *value) {
  MediaScanTagItem *ti = tag_item_create(key, value);

  if (t->nitems < MAX_TAG_ITEMS - 1)
    t->items[t->nitems++] = ti;
}
