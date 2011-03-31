///-------------------------------------------------------------------------------------------------
/// @file libmediascan\src\mediascan_macos.c
///
///  mediascan mac OS class.
///-------------------------------------------------------------------------------------------------

#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <errno.h>


#include <libmediascan.h>
#include "common.h"
#include "mediascan.h"

void unix_init(void)
{
  PathMax = pathconf(".", _PC_PATH_MAX); // 1024
}

///-------------------------------------------------------------------------------------------------
///  Recursively walk a directory struction.
///
/// @author Andy Grundman
/// @date 03/15/2011
///
/// @param [in,out] s	   If non-null, the.
/// @param path			   Full pathname of the file.
/// @param [in,out] curdir If non-null, the curdir.
///
/// ### remarks .
///-------------------------------------------------------------------------------------------------

void
recurse_dir(MediaScan *s, const char *path)
{
  char *dir, *p;
  char *tmp_full_path;
  DIR *dirp;
  struct dirent *dp;
  struct dirq *subdirq; // list of subdirs of the current directory
  struct dirq_entry *parent_entry = NULL; // entry for current dir in s->_dirq

  if (path[0] != '/') { // XXX Win32
    // Get full path
    char *buf = (char *)malloc((size_t)PathMax);
    if (buf == NULL) {
      FATAL("Out of memory for directory scan\n");
      return;
    }

    dir = getcwd(buf, (size_t)PathMax);
    strcat(dir, "/");
    strcat(dir, path);
  }
  else {
    dir = strdup(path);
  }

  // Strip trailing slash if any
  p = &dir[0];
  while (*p != 0) {
#ifdef _WIN32
    if (p[1] == 0 && (*p == '/' || *p == '\\'))
#else
    if (p[1] == 0 && *p == '/')
#endif
      *p = 0;
    p++;
  }
  
  LOG_INFO("Recursed into %s\n", dir);
  
  if ((dirp = opendir(dir)) == NULL) {
    LOG_ERROR("Unable to open directory %s: %s\n", dir, strerror(errno));
    goto out;
  }
  
  subdirq = malloc(sizeof(struct dirq));
  SIMPLEQ_INIT(subdirq);

  tmp_full_path = malloc((size_t)PathMax);
  
  while ((dp = readdir(dirp)) != NULL) {
    char *name = dp->d_name;

    // skip all dot files
    if (name[0] != '.') {        
      // XXX some platforms may be missing d_type/DT_DIR
      if (dp->d_type == DT_DIR) {
        // Add to list of subdirectories we need to recurse into
        struct dirq_entry *subdir_entry = malloc(sizeof(struct dirq_entry));
        
        // Construct full path
        *tmp_full_path = 0;
        strcat(tmp_full_path, dir);
        strcat(tmp_full_path, "/");
        strcat(tmp_full_path, name);
        
        subdir_entry->dir = strdup(tmp_full_path);
        SIMPLEQ_INSERT_TAIL(subdirq, subdir_entry, entries);
        
        LOG_INFO("  subdir: %s\n", tmp_full_path);
      }
      else {
        enum media_type type = _should_scan(s, name);
        if (type) {
          struct fileq_entry *entry;
          
          if (parent_entry == NULL) {
            // Add parent directory to list of dirs with files
            parent_entry = malloc(sizeof(struct dirq_entry));
            parent_entry->dir = strdup(dir);
            parent_entry->files = malloc(sizeof(struct fileq));
            SIMPLEQ_INIT(parent_entry->files);
            SIMPLEQ_INSERT_TAIL((struct dirq *)s->_dirq, parent_entry, entries);
          }
          
          // Add scannable file to this directory list
          entry = malloc(sizeof(struct fileq_entry));
          entry->file = strdup(name);
          entry->type = type;
          SIMPLEQ_INSERT_TAIL(parent_entry->files, entry, entries);
          
          s->progress->total++;
          
          LOG_INFO("  [%5d] file: %s\n", s->progress->total, entry->file);
        }
      }
    }
  }
    
  closedir(dirp);
  
  // Send progress update
  if (s->on_progress)
    if (progress_update(s->progress, dir))
      s->on_progress(s, s->progress, s->userdata);

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
}
