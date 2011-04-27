#ifndef _SCANDATA_H
#define _SCANDATA_H

// File type and extensions for the type
typedef struct {
  char *type;
  char *ext[15];
} type_ext;

// Handler functions for each file type
typedef struct {
  char *type;
  int (*scan) (MediaScan s);
} type_handler;

MediaScanResult *result_create(MediaScan *s);

/**
 * Fill out the MediaScanResult struct by performing any necessary scan
 * operations. Requires r->type and r->path to be set before calling.
 * Returns 1 on success.
 * Returns 0 on error, and fills r->error with a MediaScanError.
 */
int result_scan(MediaScanResult *r);

void result_destroy(MediaScanResult *r);

#endif
