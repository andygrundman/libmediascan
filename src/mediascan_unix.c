///-------------------------------------------------------------------------------------------------
/// @file libmediascan\src\mediascan_macos.m
///
///  mediascan cocoa methods
///-------------------------------------------------------------------------------------------------

#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <errno.h>
#include <unistd.h>

#include <libmediascan.h>
#include "common.h"
#include "progress.h"
#include "mediascan.h"

extern long PathMax;

void unix_init(void) {
  PathMax = pathconf(".", _PC_PATH_MAX);  // 1024
}

///-------------------------------------------------------------------------------------------------
///  Recursively walk a directory struction.
///
/// @author Andy Grundman
/// @date 03/15/2011
///
/// @param [in,out] s    If non-null, the.
/// @param path        Full pathname of the file.
/// @param [in,out] curdir If non-null, the curdir.
///
/// ### remarks .
///-------------------------------------------------------------------------------------------------

void recurse_dir(MediaScan *s, const char *path, int recurse_count) {
  char *dir, *p;
  char tmp_full_path[PathMax];
  DIR *dirp;
  struct dirent *dp;
  struct dirq *subdirq;         // list of subdirs of the current directory
  struct dirq_entry *parent_entry = NULL; // entry for current dir in s->_dirq
  char redirect_dir[MAX_PATH];

  if (recurse_count > RECURSE_LIMIT) {
    LOG_ERROR("Hit recurse limit of %d scanning path %s\n", RECURSE_LIMIT, path);
    return;
  }

  if (path[0] != '/') {         // XXX Win32
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
#ifdef USING_TCMALLOC
    // strdup will cause tcmalloc to crash on free
    dir = (char *)malloc((size_t)PathMax);
    strcpy(dir, path);
#else
    dir = strdup(path);
#endif
  }

  // Strip trailing slash if any
  p = &dir[0];
  while (*p != 0) {
    if (p[1] == 0 && *p == '/')
      *p = 0;
    p++;
  }

  LOG_INFO("Recursed into %s\n", dir);

#if defined(__APPLE__)
  if (isAlias(dir)) {
    LOG_WARN("recurse_dir 1\n");
    CheckMacAlias(dir, redirect_dir);
    LOG_WARN("recurse_dir 2\n");
    LOG_INFO("Resolving Alias %s to %s\n", dir, redirect_dir);
    strcpy(dir, redirect_dir);
  }
#elif defined(__linux__)
  if (isAlias(dir)) {
    FollowLink(dir, redirect_dir);
    LOG_INFO("Resolving symlink %s to %s\n", dir, redirect_dir);
    strcpy(dir, redirect_dir);
  }
#endif

  if ((dirp = opendir(dir)) == NULL) {
    LOG_ERROR("Unable to open directory %s: %s\n", dir, strerror(errno));
    goto out;
  }

  subdirq = malloc(sizeof(struct dirq));
  SIMPLEQ_INIT(subdirq);

  while ((dp = readdir(dirp)) != NULL) {
    char *name = dp->d_name;

    // skip all dot files
    if (name[0] != '.') {
      // XXX some platforms may be missing d_type/DT_DIR
      if (dp->d_type == DT_DIR) {
        // Add to list of subdirectories we need to recurse into
        struct dirq_entry *subdir_entry = malloc(sizeof(struct dirq_entry));

        // Construct full path
        //*tmp_full_path = 0;
        strcpy(tmp_full_path, dir);
        strcat(tmp_full_path, "/");
        strcat(tmp_full_path, name);

        subdir_entry->dir = strdup(tmp_full_path);
        SIMPLEQ_INSERT_TAIL(subdirq, subdir_entry, entries);

        LOG_INFO(" subdir: %s\n", tmp_full_path);
      }
      else {
        enum media_type type = _should_scan(s, name);

        LOG_INFO("name %s = type %d\n", name, type);

        if (type) {
          struct fileq_entry *entry;

          // Check if this file is a shortcut and if so resolve it
#if defined(__APPLE__)
          if (isAlias(name)) {
            char full_name[MAX_PATH];

            LOG_INFO("Mac Alias detected\n");

            strcpy(full_name, dir);
            strcat(full_name, "\\");
            strcat(full_name, name);
            parse_lnk(full_name, redirect_dir, MAX_PATH);
            if (PathIsDirectory(redirect_dir)) {
              struct dirq_entry *subdir_entry = malloc(sizeof(struct dirq_entry));

              subdir_entry->dir = strdup(redirect_dir);
              SIMPLEQ_INSERT_TAIL(subdirq, subdir_entry, entries);

              LOG_INFO(" subdir: %s\n", tmp_full_path);
              type = 0;
            }

          }
#elif defined(__linux__)
          if (isAlias(name)) {
            char full_name[MAX_PATH];

            printf("Linux Alias detected\n");

            strcpy(full_name, dir);
            strcat(full_name, "\\");
            strcat(full_name, name);
            FollowLink(full_name, redirect_dir);
            if (PathIsDirectory(redirect_dir)) {
              struct dirq_entry *subdir_entry = malloc(sizeof(struct dirq_entry));

              subdir_entry->dir = strdup(redirect_dir);
              SIMPLEQ_INSERT_TAIL(subdirq, subdir_entry, entries);

              LOG_INFO(" subdir: %s\n", tmp_full_path);
              type = 0;
            }

          }
#endif
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

          LOG_INFO(" [%5d] file: %s\n", s->progress->total, entry->file);
        }
      }
    }
  }

  closedir(dirp);

  // Send progress update
  if (s->on_progress)
    if (progress_update(s->progress, dir))
      send_progress(s);

  // process subdirs
  while (!SIMPLEQ_EMPTY(subdirq)) {
    struct dirq_entry *subdir_entry = SIMPLEQ_FIRST(subdirq);
    SIMPLEQ_REMOVE_HEAD(subdirq, entries);
    recurse_dir(s, subdir_entry->dir, recurse_count);
    free(subdir_entry);
  }

  free(subdirq);

out:
  free(dir);
}
