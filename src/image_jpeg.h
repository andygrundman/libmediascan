#ifndef _IMAGE_JPEG_H
#define _IMAGE_JPEG_H

int image_jpeg_read_header(MediaScanImage *i, MediaScanResult *r);
void image_jpeg_destroy(MediaScanImage *i);

#endif // _IMAGE_JPEG_H
