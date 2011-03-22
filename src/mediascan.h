
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

struct thread_data {
   MediaScan *s;
   LPTSTR lpDir;
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
/// @param path		  Full pathname of the file.
///
/// @return .
///
/// ### remarks .
///-------------------------------------------------------------------------------------------------

int _should_scan(MediaScan *s, const char *path);


///-------------------------------------------------------------------------------------------------
/// <summary>	Query if 'path' is absolute path. </summary>
///
/// <remarks>	Henry Bennett, 03/16/2011. </remarks>
///
/// <param name="path"> Pathname to check </param>
///
/// <returns>	true if absolute path, false if not. </returns>
///-------------------------------------------------------------------------------------------------

bool is_absolute_path(const char *path);

///-------------------------------------------------------------------------------------------------
///  Recursively walk a directory struction using Win32 style directory commands
///
/// @author Henry Bennett
/// @date 03/15/2011
///
/// @param [in,out] s	   If non-null, the.
/// @param path			   Full pathname of the file.
/// @param [in,out] curdir If non-null, the curdir.
///
/// ### remarks .
///-------------------------------------------------------------------------------------------------

void recurse_dir(MediaScan *s, const char *path, struct dirq_entry *curdir);

#ifdef WIN32

///-------------------------------------------------------------------------------------------------
///  Watch directory.
///
/// @author Henry Bennett
/// @date 03/22/2011
///
/// @param lpDir String describing the path to be watched
///-------------------------------------------------------------------------------------------------

DWORD WINAPI WatchDirectory(LPVOID inData);

///-------------------------------------------------------------------------------------------------
///  Code to refresh the directory listing, but not the subtree because it would not be necessary.
///
/// @author Henry Bennett
/// @date 03/22/2011
///
/// @param lpDir The pointer to a dir.
///-------------------------------------------------------------------------------------------------

void RefreshDirectory(MediaScan *s, LPTSTR lpDir);

#endif

#endif