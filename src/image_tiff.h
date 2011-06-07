#ifndef _IMAGE_TIFF_H
#define _IMAGE_TIFF_H

int image_tiff_read_header(MediaScanImage *i, MediaScanResult *r);
int image_tiff_load(MediaScanImage *i);
void image_tiff_destroy(MediaScanImage *i);

#endif // _IMAGE_TIFF_H
