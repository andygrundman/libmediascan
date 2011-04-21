#ifndef _IMAGE_GIF_H
#define _IMAGE_GIF_H

int image_gif_read_header(MediaScanImage *i, MediaScanResult *r, int is_gif89);
int image_gif_load(MediaScanImage *i);
void image_gif_destroy(MediaScanImage *i);

#endif // _IMAGE_GIF_H
