#ifndef _IMAGE_H
#define _IMAGE_H

#include "buffer.h"

// pixel helpers
#define COL(red, green, blue) (((red) << 24) | ((green) << 16) | ((blue) << 8) | 0xFF)
#define COL_FULL(red, green, blue, alpha) (((red) << 24) | ((green) << 16) | ((blue) << 8) | (alpha))
#define COL_RED(col)   ((col >> 24) & 0xFF)
#define COL_GREEN(col) ((col >> 16) & 0xFF)
#define COL_BLUE(col)  ((col >> 8) & 0xFF)
#define COL_ALPHA(col) (col & 0xFF)

MediaScanImage * image_create(void);
void image_destroy(MediaScanImage *i);
int image_read_header(MediaScanImage *i, MediaScanResult *r);
void image_add_thumbnail(MediaScanImage *i, MediaScanImage *thumb);
int image_load(MediaScanImage *i, MediaScanThumbSpec *spec_hint);
void image_alloc_pixbuf(MediaScanImage *i, int width, int height);
void image_unload(MediaScanImage *i);

#endif // _IMAGE_H