///-------------------------------------------------------------------------------------------------
/// @file libmediascan\src\mediascan_win32.c
///
///  mediascan window 32 class.
///-------------------------------------------------------------------------------------------------

#include <Windows.h>
#include <tchar.h>
#include <stdio.h>
#include <stdlib.h>
#include <strsafe.h>
#include <direct.h>
#include <tchar.h>
#include <libmediascan.h>
#include "common.h"
#include "queue.h"
#include "mediascan.h"
#include "progress.h"

#ifdef _MSC_VER
#pragma warning( disable: 4127 )
#endif

///-------------------------------------------------------------------------------------------------
///  Recursively walk a directory struction using Win32 style directory commands
///
/// @author Henry Bennett
/// @date 03/15/2011
///
/// @param [in,out] s    If non-null, the.
/// @param path        Full pathname of the file.
/// @param [in,out] curdir If non-null, the curdir.
///
/// ### remarks .
///-------------------------------------------------------------------------------------------------

void recurse_dir(MediaScan *s, const char *path) {
  char *dir = NULL;
  char *p = NULL;
  char *tmp_full_path;
  struct dirq_entry *parent_entry = NULL; // entry for current dir in s->_dirq
  struct dirq *subdirq;         // list of subdirs of the current directory


  // Windows directory browsing variables
  HANDLE hFind = INVALID_HANDLE_VALUE;
  WIN32_FIND_DATA ffd;
  DWORD dwError = 0;
  TCHAR findDir[MAX_PATH];

  if (s == NULL) {
    ms_errno = MSENO_NULLSCANOBJ;
    LOG_ERROR("MediaScan = NULL, aborting scan\n");
    return;
  }

  if (!is_absolute_path(path)) {
    // Get full path
    char *buf = (char *)malloc(MAX_PATH);
    if (buf == NULL) {
      LOG_ERROR("Out of memory for directory scan\n");
      return;
    }

    dir = _getcwd(buf, MAX_PATH);

    strcat_s(dir, MAX_PATH, "\\");
    strcat_s(dir, MAX_PATH, path);
  }
  else {
    dir = _strdup(path);
  }


  // Strip trailing slash if any
  p = &dir[0];
  while (*p != 0) {
    if (p[1] == 0 && (*p == '/' || *p == '\\'))
      *p = 0;
    p++;
  }

  LOG_LEVEL(2, "Recursed into %s\n", dir);


  // Prepare string for use with FindFile functions.  First, copy the
  // string to a buffer, then append '\*' to the directory name.
  StringCchCopy(findDir, MAX_PATH, dir);
  StringCchCat(findDir, MAX_PATH, TEXT("\\*"));


  // Find the first file in the directory.
  hFind = FindFirstFile(findDir, &ffd);
  if (INVALID_HANDLE_VALUE == hFind) {
    LOG_ERROR("Unable to open directory %s errno %d\n", dir, MSENO_DIRECTORYFAIL);
    ms_errno = MSENO_DIRECTORYFAIL;
    goto out;
  }


  subdirq = malloc(sizeof(struct dirq));
  SIMPLEQ_INIT(subdirq);

  tmp_full_path = malloc(MAX_PATH);


  do {

    char *name = ffd.cFileName;

    // skip all dot files
    if (name[0] != '.') {
      if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
        struct dirq_entry *subdir_entry = malloc(sizeof(struct dirq_entry));

        // Construct full path
        *tmp_full_path = 0;
        strcat_s(tmp_full_path, MAX_PATH, dir);
        strcat_s(tmp_full_path, MAX_PATH, "\\");
        strcat_s(tmp_full_path, MAX_PATH, name);


        subdir_entry->dir = _strdup(tmp_full_path);
        SIMPLEQ_INSERT_TAIL(subdirq, subdir_entry, entries);

        LOG_INFO(" subdir: %s\n", tmp_full_path);
      }
      else {
        enum media_type type = _should_scan(s, name);

        if (type) {
          struct fileq_entry *entry;

          if (parent_entry == NULL) {
            // Add parent directory to list of dirs with files
            parent_entry = malloc(sizeof(struct dirq_entry));
            parent_entry->dir = _strdup(dir);
            parent_entry->files = malloc(sizeof(struct fileq));
            SIMPLEQ_INIT(parent_entry->files);
            SIMPLEQ_INSERT_TAIL((struct dirq *)s->_dirq, parent_entry, entries);
          }

          // Add scannable file to this directory list
          entry = malloc(sizeof(struct fileq_entry));
          entry->file = _strdup(name);
          entry->type = type;
          SIMPLEQ_INSERT_TAIL(parent_entry->files, entry, entries);

          s->progress->total++;

          LOG_INFO(" [%5d] file: %s\n", s->progress->total, entry->file);
        }
      }
    }
  } while (FindNextFile(hFind, &ffd) != 0);

  dwError = GetLastError();
  if (dwError != ERROR_NO_MORE_FILES) {
    LOG_ERROR("Error searching files.");
  }

  FindClose(hFind);

  // Send progress update
  if (s->on_progress)
    if (progress_update(s->progress, dir))
      s->on_progress(s, s->progress, s->userdata);

/* Old Progress Code
  // Send progress update
  if (s->on_progress) {
	long tick_ms = GetTickCount();

	// Check to see if _last_callback needs to be set to a reasonable time
	if( s->progress->_last_callback == 0 )
	{
		s->progress->_last_callback = tick_ms;
	}

    if (tick_ms - s->progress->_last_callback >= s->progress_interval) {
      s->progress->cur_item = dir;
      s->progress->_last_callback = tick_ms;
      s->on_progress(s, s->progress);
    }
  } */


  // process subdirs
  while (!SIMPLEQ_EMPTY(subdirq)) {
    struct dirq_entry *subdir_entry = SIMPLEQ_FIRST(subdirq);
    SIMPLEQ_REMOVE_HEAD(subdirq, entries);
    recurse_dir(s, subdir_entry->dir);
    free(subdir_entry);
  }

  free(subdirq);
  free(tmp_full_path);

out:
  free(dir);
}                               /* recurse_dir() */
