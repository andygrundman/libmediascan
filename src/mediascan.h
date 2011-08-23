
#ifndef MEDIASCAN_H
#define MEDIASCAN_H

#include "queue.h"

// File/dir queue struct definitions
struct fileq_entry {
  char *file;
  enum media_type type;
    SIMPLEQ_ENTRY(fileq_entry) entries;
};
SIMPLEQ_HEAD(fileq, fileq_entry);

struct dirq_entry {
  char *dir;
  struct fileq *files;
    SIMPLEQ_ENTRY(dirq_entry) entries;
};
SIMPLEQ_HEAD(dirq, dirq_entry);

struct thread_data {
  MediaScan *s;
  char *lpDir;
};

typedef struct thread_data thread_data_type;

#ifndef bool
#define bool int
#endif

#ifndef TRUE
#define TRUE (int)1
#endif

#ifndef FALSE
#define FALSE (int)0
#endif

///-------------------------------------------------------------------------------------------------
///  Determine if we should scan a path.
///
/// @author Henry Bennett
/// @date 03/15/2011
///
/// @param [in,out] s If non-null, the.
/// @param path     Full pathname of the file.
///
/// @return .
///
/// ### remarks .
///-------------------------------------------------------------------------------------------------

int _should_scan(MediaScan *s, const char *path);

///-------------------------------------------------------------------------------------------------
///  Determine if we should scan a path.
///
/// @author Andy Grundman
/// @date 03/15/2011
///
/// @param [in,out] s If non-null, the.
/// @param path     Full pathname of the file.
///
/// @return .
///
/// ### remarks .
///-------------------------------------------------------------------------------------------------

int _should_scan_dir(MediaScan *s, const char *path);

///-------------------------------------------------------------------------------------------------
/// <summary> Query if 'path' is absolute path. </summary>
///
/// <remarks> Henry Bennett, 03/16/2011. </remarks>
///
/// <param name="path"> Pathname to check </param>
///
/// <returns> true if absolute path, false if not. </returns>
///-------------------------------------------------------------------------------------------------

bool is_absolute_path(const char *path);

///-------------------------------------------------------------------------------------------------
/// Recursively walk a directory struction.
///
/// @author Henry Bennett
/// @date 03/15/2011
///
/// @param [in,out] s Scan instance.
/// @param path Full pathname of the directory.
///-------------------------------------------------------------------------------------------------

void recurse_dir(MediaScan *s, const char *path, int recurse_count);

///-------------------------------------------------------------------------------------------------
/// Add a thumbnail to the internal list of result thumbnails. Up to MAX_THUMBS (8) can be added.
///
/// @author Andy Grundman
/// @date 04/13/2011
///
/// @param r Result instance.
/// @param thumb Image instance containing generated thumbnail.
///-------------------------------------------------------------------------------------------------

void result_add_thumbnail(MediaScanResult *r, MediaScanImage *thumb);

///-------------------------------------------------------------------------------------------------
/// Send a progress callback, or notify about it if async.
///
/// @author Andy Grundman
/// @date 04/14/2011
///
/// @param s Scan instance.
///-------------------------------------------------------------------------------------------------
void send_progress(MediaScan *s);

///-------------------------------------------------------------------------------------------------
/// Send an error callback, or notify about it if async.
///
/// @author Andy Grundman
/// @date 04/18/2011
///
/// @param s Scan instance.
/// @param e Error instance.
///-------------------------------------------------------------------------------------------------
void send_error(MediaScan *s, MediaScanError *e);

///-------------------------------------------------------------------------------------------------
/// Send a result callback, or notify about it if async.
///
/// @author Andy Grundman
/// @date 04/18/2011
///
/// @param s Scan instance.
/// @param e Result instance.
///-------------------------------------------------------------------------------------------------
void send_result(MediaScan *s, MediaScanResult *r);

void send_finish(MediaScan *s);

#ifdef WIN32

///-------------------------------------------------------------------------------------------------
///  Watch directory.
///
/// @author Henry Bennett
/// @date 03/22/2011
///
/// @param lpDir String describing the path to be watched
///-------------------------------------------------------------------------------------------------

void WatchDirectory(LPVOID inData);

///-------------------------------------------------------------------------------------------------
///  Code to refresh the directory listing, but not the subtree because it would not be necessary.
///
/// @author Henry Bennett
/// @date 03/22/2011
///
/// @param lpDir The pointer to a dir.
///-------------------------------------------------------------------------------------------------

void RefreshDirectory(MediaScan *s, LPTSTR lpDir);

int parse_lnk(const char *path, LPTSTR szTarget, SIZE_T cchTarget);

#endif // WIN32

#endif // MEDIASCAN_H
