#ifndef _IMAGE_H
#define _IMAGE_H

#include "buffer.h"

MediaScanImage * image_create(void);
void image_destroy(MediaScanImage *i);
int image_read_header(MediaScanImage *i, MediaScanResult *r);
void image_create_thumbnail(MediaScanImage *i, MediaScanThumbSpec *spec);

#endif // _IMAGE_H