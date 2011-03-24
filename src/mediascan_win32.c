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

void recurse_dir(MediaScan *s, const char *path, struct dirq_entry *curdir)
{
  char *dir = NULL;
  char *p = NULL;
  struct dirq *subdirq;
  char *tmp_full_path;

  // Windows directory browsing variables
  HANDLE hFind = INVALID_HANDLE_VALUE;
  WIN32_FIND_DATA ffd;
  DWORD dwError=0;
  TCHAR  findDir[MAX_PATH];

  if(s == NULL) {
	ms_errno = MSENO_NULLSCANOBJ;
    LOG_ERROR("MediaScan = NULL, aborting scan\n");
    return;
  }

  if ( !is_absolute_path(path) ) { 
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
	if (INVALID_HANDLE_VALUE == hFind) 
	{
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
        // Construct full path
        *tmp_full_path = 0;
        strcat_s(tmp_full_path, MAX_PATH, dir);
        strcat_s(tmp_full_path, MAX_PATH, "\\");
        strcat_s(tmp_full_path, MAX_PATH, name);
        
        if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
        // Entry for complete list of dirs
        // XXX somewhat inefficient, we create this for every directory
        // even those that don't end up having any scannable files
		struct dirq_entry *subdir_entry;
        struct dirq_entry *entry = malloc(sizeof(struct dirq_entry));
        entry->dir = _strdup(tmp_full_path);
        entry->files = malloc(sizeof(struct fileq));
        SIMPLEQ_INIT(entry->files);
        
        // Temporary list of subdirs of the current directory
        subdir_entry = malloc(sizeof(struct dirq_entry));
        
        // Copy entry to subdir_entry, dir will be freed by ms_destroy()
        memcpy(subdir_entry, entry, sizeof(struct dirq_entry));
        SIMPLEQ_INSERT_TAIL(subdirq, subdir_entry, entries);
        
        SIMPLEQ_INSERT_TAIL((struct dirq *)s->_dirq, entry, entries);
        
        s->progress->dir_total++;
        
        LOG_LEVEL(2, "  [%5d] subdir: %s\n", s->progress->dir_total, entry->dir);
      }
      else {
		enum media_type type = _should_scan(s, name);

        if ( type ) {
          // To save memory by not storing the full path to every file,
          // each dir has a list of files in that dir
          struct fileq_entry *entry = malloc(sizeof(struct fileq_entry));
          entry->file = _strdup(name);
          SIMPLEQ_INSERT_TAIL(curdir->files, entry, entries);
          
          s->progress->file_total++;
          
          LOG_LEVEL(2, "  [%5d] file: %s\n", s->progress->file_total, entry->file);
          
          // Scan the file
          ms_scan_file(s, tmp_full_path, type);
        }
      }
    }
  } while (FindNextFile(hFind, &ffd) != 0); 

   dwError = GetLastError();
   if (dwError != ERROR_NO_MORE_FILES) 
   {
      LOG_ERROR("Error searching files");
   }


   FindClose(hFind);

  
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
  }

  // process subdirs
  while (!SIMPLEQ_EMPTY(subdirq)) {
    struct dirq_entry *subdir_entry = SIMPLEQ_FIRST(subdirq);
    SIMPLEQ_REMOVE_HEAD(subdirq, entries);
    recurse_dir(s, subdir_entry->dir, subdir_entry);
    free(subdir_entry);
  }
  
  free(subdirq);
  free(tmp_full_path);

out:
  free(dir);
} /* recurse_dir() */

