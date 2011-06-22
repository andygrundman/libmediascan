#ifndef _TAG_ITEM_H
#define _TAG_ITEM_H

MediaScanTagItem *tag_item_create(const char *key, const char *value);
void tag_item_destroy(MediaScanTagItem * ti);

#endif // _TAG_ITEM_H
