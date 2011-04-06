#ifndef _IMAGE_JPEG_H
#define _IMAGE_JPEG_H

int image_jpeg_read_header(MediaScanImage *i, MediaScanResult *r);
int image_jpeg_load(MediaScanImage *i, MediaScanThumbSpec *spec_hint);
void image_jpeg_compress(MediaScanImage *i, MediaScanThumbSpec *spec);
void image_jpeg_destroy(MediaScanImage *i);

#endif // _IMAGE_JPEG_H
