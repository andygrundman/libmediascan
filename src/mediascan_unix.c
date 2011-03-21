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

static long PathMax = 0;

void macos_init(void)
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
recurse_dir(MediaScan *s, const char *path, struct dirq_entry *curdir)
{
  char *dir;

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
  char *p = &dir[0];
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

  DIR *dirp;
  if ((dirp = opendir(dir)) == NULL) {
    LOG_ERROR("Unable to open directory %s: %s\n", dir, strerror(errno));
    goto out;
  }
  
  struct dirq *subdirq = malloc(sizeof(struct dirq));
  SIMPLEQ_INIT(subdirq);

  char *tmp_full_path = malloc((size_t)PathMax);

  struct dirent *dp;
  while ((dp = readdir(dirp)) != NULL) {
    char *name = dp->d_name;

    // skip all dot files
    if (name[0] != '.') {
        // Construct full path
        *tmp_full_path = 0;
        strcat(tmp_full_path, dir);
        strcat(tmp_full_path, "/");
        strcat(tmp_full_path, name);
        
      // XXX some platforms may be missing d_type/DT_DIR
      if (dp->d_type == DT_DIR) {
        // Entry for complete list of dirs
        // XXX somewhat inefficient, we create this for every directory
        // even those that don't end up having any scannable files
        struct dirq_entry *entry = malloc(sizeof(struct dirq_entry));
        entry->dir = strdup(tmp_full_path);
        entry->files = malloc(sizeof(struct fileq));
        SIMPLEQ_INIT(entry->files);
        
        // Temporary list of subdirs of the current directory
        struct dirq_entry *subdir_entry = malloc(sizeof(struct dirq_entry));
        
        // Copy entry to subdir_entry, dir will be freed by ms_destroy()
        memcpy(subdir_entry, entry, sizeof(struct dirq_entry));
        SIMPLEQ_INSERT_TAIL(subdirq, subdir_entry, entries);
        
        SIMPLEQ_INSERT_TAIL((struct dirq *)s->_dirq, entry, entries);
        
        s->progress->dir_total++;
        
        LOG_INFO(" [%5d] subdir: %s\n", s->progress->dir_total, entry->dir);
      }
      else {
        enum media_type type = _should_scan(s, name);
        if (type) {
          // To save memory by not storing the full path to every file,
          // each dir has a list of files in that dir
          struct fileq_entry *entry = malloc(sizeof(struct fileq_entry));
          entry->file = strdup(name);
          SIMPLEQ_INSERT_TAIL(curdir->files, entry, entries);
          
          s->progress->file_total++;
          
          LOG_INFO(" [%5d] file: %s\n", s->progress->file_total, entry->file);
          
          // Scan the file
          ms_scan_file(s, tmp_full_path, type);
        }
      }
    }
  }
    
  closedir(dirp);
  
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
    recurse_dir(s, subdir_entry->dir, subdir_entry);
    free(subdir_entry);
  }
  
  free(subdirq);
  free(tmp_full_path);

out:
  free(dir);
}