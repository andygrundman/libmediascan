#ifndef _VIDEO_H
#define _VIDEO_H

MediaScanVideo *video_create(void);
MediaScanImage *video_create_image_from_frame(MediaScanVideo *v, MediaScanResult *r);
void video_destroy(MediaScanVideo *v);

#endif // _VIDEO_H
