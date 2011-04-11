#ifndef _VIDEO_H
#define _VIDEO_H

MediaScanVideo * video_create(void);
MediaScanImage * video_create_image_from_frame(MediaScanVideo *v, MediaScanResult *r);
void video_add_thumbnail(MediaScanVideo *v, MediaScanImage *i);
uint8_t * video_get_thumbnail(MediaScanVideo *v, int index, int *length);
void video_destroy(MediaScanVideo *v);

#endif // _VIDEO_H