///-------------------------------------------------------------------------------------------------
/// @file libmediascan\src\mediascan_macos.c
///
///  mediascan mac OS class.
///-------------------------------------------------------------------------------------------------

#include <dirent.h>

#include <libmediascan.h>
#include "common.h"
#include "mediascan.h"

static long PathMax = 0;

static void macos_init(void)
{
  PathMax = pathconf(".", _PC_PATH_MAX); // 1024
}

///-------------------------------------------------------------------------------------------------
///  Recursively walk a directory struction.
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

static void recurse_dir(MediaScan *s, const char *path, struct dirq_entry *curdir)
{
  char *dir = NULL;
  char *p = NULL;
  DIR *dirp;
  struct dirq *subdirq;
  char *tmp_full_path;
  struct dirent *dp;

  if (path[0] != '/') { // XXX Win32
    // Get full path
    char *buf = (char *)malloc((size_t)PathMax);
    if (buf == NULL) {
      LOG_ERROR("Out of memory for directory scan\n");
      return;
    }

#ifdef WIN32
    dir = _getcwd(buf, (size_t)PathMax);
#else
    dir = getcwd(buf, (size_t)PathMax);
#endif

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
  
  LOG_LEVEL(2, "Recursed into %s\n", dir);

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
    entry->dir = strdup("/"); // so free doesn't choke on this item later
    entry->files = malloc(sizeof(struct fileq));
    SIMPLEQ_INIT(entry->files);
    SIMPLEQ_INSERT_TAIL((struct dirq *)s->_dirq, entry, entries);
    
    phase = (char *)malloc((size_t)PathMax);
    sprintf(phase, "Discovering files in %s", s->paths[i]);
    s->progress->phase = phase;
    
    LOG_LEVEL(1, "Scanning %s\n", s->paths[i]);
    recurse_dir(s, s->paths[i], entry);
    
    // Send final progress callback
    if (s->on_progress) {
      s->progress->cur_item = NULL;
      s->on_progress(s, s->progress);
    }
    
    free(phase);
  }
}