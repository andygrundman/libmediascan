///-------------------------------------------------------------------------------------------------
/// @file libmediascan\src\mediascan_linux.c
///
///  mediascan linux methods
///-------------------------------------------------------------------------------------------------

#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <libmediascan.h>
#include <errno.h>

#define LINK_NONE 		0
#define LINK_ALIAS 		1
#define LINK_SYMLINK 	2

#include "common.h"

int isAlias(const char *incoming_path) {

  char buffer[MAX_PATH_STR_LEN];

  int ret = readlink(incoming_path, buffer, MAX_PATH_STR_LEN);
  if (ret == -1) {
    return LINK_NONE;
  }

  return LINK_SYMLINK;
}                               /* isAlias() */

int FollowLink(const char *incoming_path, char *out_path) {

  char buf[MAX_PATH_STR_LEN];
  ssize_t len;

  if ((len = readlink(incoming_path, buf, MAX_PATH_STR_LEN - 1)) != -1) {
    buf[len] = '\0';

    // Check if this is a relative path
    if (buf[0] == '.')
      realpath(buf, out_path);
    else
      strcpy(out_path, buf);
  }
  else {
    strcpy(out_path, "");
    LOG_ERROR("readlink %s failed: %d\n", incoming_path, errno);
  }

  return LINK_SYMLINK;
}                               /* FollowLink() */


int PathIsDirectory(const char *dir) {
  struct stat st_buf;

  if (stat(dir, &st_buf) == -1)
    return 0;

  // Get the status of the file
  if (S_ISREG(st_buf.st_mode)) {
    return 0;                   //return false if path is a regular file
  }
  if (S_ISDIR(st_buf.st_mode)) {
    return 1;                   //return true if path is a directory
  }
}                               /* PathIsDirectory() */
