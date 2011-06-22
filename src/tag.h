#ifndef _TAG_H
#define _TAG_H

MediaScanTag *tag_create(const char *type);
void tag_add_item(MediaScanTag * t, const char *key, const char *value);
void tag_destroy(MediaScanTag * t);

#endif // _TAG_H
