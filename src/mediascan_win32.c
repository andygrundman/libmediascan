
#include <Windows.h>
#include <tchar.h>
#include <stdio.h>
#include <stdlib.h>
#include <strsafe.h>
#include <direct.h>
#include <libmediascan.h>
#include "common.h"
#include "queue.h"
#include "mediascan.h"


#define bool int
#ifndef TRUE
#define TRUE (int)1
#endif

#ifndef FALSE
#define FALSE (int)0
#endif

///-------------------------------------------------------------------------------------------------
/// @file libmediascan\src\mediascan_win32.c
///
///  mediascan window 32 class.
///-------------------------------------------------------------------------------------------------

///-------------------------------------------------------------------------------------------------
///  Win32 specific MediaScan initalization.
///
/// @author Henry Bennett
/// @date 03/15/2011
///-------------------------------------------------------------------------------------------------

void win32_init(void)
{

}

///-------------------------------------------------------------------------------------------------
/// <summary>	Query if 'path' is absolute path. </summary>
///
/// <remarks>	Henry Bennett, 03/16/2011. </remarks>
///
/// <param name="path"> Pathname to check </param>
///
/// <returns>	true if absolute path, false if not. </returns>
///-------------------------------------------------------------------------------------------------

static bool is_absolute_path(const char *path) {

	if(path == NULL)
		return FALSE;

	// \workspace, /workspace, etc
	if( strlen(path) > 1 && ( path[0] == '/' || path[0] == '\\') ) 
		return TRUE;

	// C:\, D:\, etc
	if( strlen(path) > 2 && path[1] == ':' )
		return TRUE;

	return FALSE;
} /* is_absolute_path() */

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

static void recurse_dir_win32(MediaScan *s, const char *path, struct dirq_entry *curdir)
{
  char *dir = NULL;
  char *p = NULL;
  struct dirq *subdirq;
  char *tmp_full_path;
  struct dirent *dp;

  // Windows directory browsing variables
  HANDLE hFind = INVALID_HANDLE_VALUE;
  WIN32_FIND_DATA ffd;
  DWORD dwError=0;
  TCHAR  findDir[MAX_PATH];

  if ( !is_absolute_path(path) ) { 
    // Get full path
    char *buf = (char *)malloc(MAX_PATH);
    if (buf == NULL) {
      LOG_ERROR("Out of memory for directory scan\n");
      return;
    }

    dir = _getcwd(buf, MAX_PATH);

    strcat(dir, "\\");
    strcat(dir, path);
  }
  else {
    dir = strdup(path);
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
    LOG_ERROR("Unable to open directory %s: %s\n", dir, strerror(errno));
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
        strcat(tmp_full_path, dir);
        strcat(tmp_full_path, "\\");
        strcat(tmp_full_path, name);
        
        if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
        // Entry for complete list of dirs
        // XXX somewhat inefficient, we create this for every directory
        // even those that don't end up having any scannable files
		struct dirq_entry *subdir_entry;
        struct dirq_entry *entry = malloc(sizeof(struct dirq_entry));
        entry->dir = strdup(tmp_full_path);
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
        if ( _should_scan(s, name) ) {
          // To save memory by not storing the full path to every file,
          // each dir has a list of files in that dir
          struct fileq_entry *entry = malloc(sizeof(struct fileq_entry));
          entry->file = strdup(name);
          SIMPLEQ_INSERT_TAIL(curdir->files, entry, entries);
          
          s->progress->file_total++;
          
          LOG_LEVEL(2, "  [%5d] file: %s\n", s->progress->file_total, entry->file);
          
          // Scan the file
          ms_scan_file(s, tmp_full_path);
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
    struct timeval now;
    gettimeofday(&now, NULL);
    
    if (now.tv_sec - s->progress->_last_callback >= s->progress_interval) {
      s->progress->cur_item = dir;
      s->progress->_last_callback = now.tv_sec;
      s->on_progress(s, s->progress);
    }
  }

  // process subdirs
  while (!SIMPLEQ_EMPTY(subdirq)) {
    struct dirq_entry *subdir_entry = SIMPLEQ_FIRST(subdirq);
    SIMPLEQ_REMOVE_HEAD(subdirq, entries);
    recurse_dir_win32(s, subdir_entry->dir, subdir_entry);
    free(subdir_entry);
  }
  
  free(subdirq);
  free(tmp_full_path);

out:
  free(dir);
} /* recurse_dir_win32() */


///-------------------------------------------------------------------------------------------------
///  Begin a recursive scan of all paths previously provided to ms_add_path(). If async mode
/// 	is enabled, this call will return immediately. You must obtain the file descriptor using
/// 	ms_async_fd and this must be checked using an event loop or select(). When the fd becomes
/// 	readable you must call ms_async_process to trigger any necessary callbacks.
///
/// @author Henry Bennett
/// @date 03/15/2011
///
/// @param [in,out] s If non-null, the.
///
/// ### remarks .
///-------------------------------------------------------------------------------------------------

void ms_scan(MediaScan *s)
{
  int i = 0;  

  PVOID OldValue = NULL;
  char *phase = NULL;

  if (s->on_result == NULL) {
    LOG_ERROR("Result callback not set, aborting scan\n");
    return;
  }
  
  if (s->async) {
    LOG_ERROR("async mode not yet supported\n");
    // XXX TODO
  }

  for (i = 0; i < s->npaths; i++) {
	char *phase = NULL;
    struct dirq_entry *entry = malloc(sizeof(struct dirq_entry));
    entry->dir = _strdup("/"); // so free doesn't choke on this item later
    entry->files = malloc(sizeof(struct fileq));
    SIMPLEQ_INIT(entry->files);
    SIMPLEQ_INSERT_TAIL((struct dirq *)s->_dirq, entry, entries);
    
    phase = (char *)malloc(MAX_PATH);
    sprintf(phase, "Discovering files in %s", s->paths[i]);
    s->progress->phase = phase;
    
    LOG_LEVEL(1, "Scanning %s\n", s->paths[i]);
    recurse_dir_win32(s, s->paths[i], entry);
    
    // Send final progress callback
    if (s->on_progress) {
      s->progress->cur_item = NULL;
      s->on_progress(s, s->progress);
    }
    
    free(phase);
  }
} /* ms_scan() */

