#ifndef _IMAGE_PNG_H
#define _IMAGE_PNG_H

int image_png_read_header(MediaScanImage *i, MediaScanResult *r);
int image_png_load(MediaScanImage *i);
void image_png_compress(MediaScanImage *i, MediaScanThumbSpec *spec);
void image_png_destroy(MediaScanImage *i);

#endif // _IMAGE_PNG_H
