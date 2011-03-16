
#ifndef MEDIASCAN_H
#define MEDIASCAN_H

#include "queue.h"

// File/dir queue struct definitions
struct fileq_entry {
  char *file;
  SIMPLEQ_ENTRY(fileq_entry) entries;
};
SIMPLEQ_HEAD(fileq, fileq_entry);

struct dirq_entry {
  char *dir;
  struct fileq *files;
  SIMPLEQ_ENTRY(dirq_entry) entries;
};
SIMPLEQ_HEAD(dirq, dirq_entry);



///-------------------------------------------------------------------------------------------------
///  Determine if we should scan a path.
///
/// @author Henry Bennett
/// @date 03/15/2011
///
/// @param [in,out] s If non-null, the.
/// @param path		  Full pathname of the file.
///
/// @return .
///
/// ### remarks .
///-------------------------------------------------------------------------------------------------

int _should_scan(MediaScan *s, const char *path);


#endif