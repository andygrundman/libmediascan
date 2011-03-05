#ifndef _SCANDATA_H
#define _SCANDATA_H

// File type and extensions for the type
typedef struct {
  char *type;
  char *ext[15];
} type_ext;

// Handler functions for each file type
typedef struct {
  char*	type;
  int (*scan)(MediaScan s);
} type_handler;

MediaScanResult * result_create(void);
void result_destroy(MediaScanResult *r);

#endif
