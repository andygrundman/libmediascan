#ifndef _ERROR_H
#define _ERROR_H

MediaScanError * error_create(const char *path, enum media_error error_code, const char *error_string);
void error_destroy(MediaScanError *e);

#endif
