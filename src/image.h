#ifndef _IMAGE_H
#define _IMAGE_H

#include "buffer.h"

MediaScanImage * image_create(void);
void image_destroy(MediaScanImage *i);
int image_read_header(MediaScanImage *i, MediaScanResult *r);

#endif // _IMAGE_H