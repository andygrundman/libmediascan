#ifndef _THUMB_H
#define _THUMB_H

typedef uint32_t pix;

MediaScanImage * thumb_create_from_image(MediaScanImage *i, MediaScanThumbSpec *spec);
int thumb_resize(MediaScanImage *src, MediaScanImage *dst, MediaScanThumbSpec *spec);
void thumb_bgcolor_fill(pix *buf, int size, pix bgcolor);
void thumb_resize_gd_fixed(MediaScanImage *src, MediaScanImage *dst, MediaScanThumbSpec *spec);

#endif // _THUMB_H
