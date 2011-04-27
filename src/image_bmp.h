#ifndef _IMAGE_BMP_H
#define _IMAGE_BMP_H

// BMP Compression methods
enum bmp_compression {
  BMP_BI_RGB = 0,
  BMP_BI_RLE8,
  BMP_BI_RLE4,
  BMP_BI_BITFIELDS,
  BMP_BI_JPEG,
  BMP_BI_PNG
};

int image_bmp_read_header(MediaScanImage *i, MediaScanResult *r);
int image_bmp_load(MediaScanImage *i);
void image_bmp_destroy(MediaScanImage *i);

#endif // _IMAGE_BMP_H
