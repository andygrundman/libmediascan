#ifndef _PROGRESS_H
#define _PROGRESS_H

MediaScanProgress *progress_create(void);
void progress_start_phase(MediaScanProgress *p, const char *fmt, ...);
int progress_update(MediaScanProgress *p, const char *cur_item);
void progress_destroy(MediaScanProgress *p);

#endif // _PROGRESS_H
