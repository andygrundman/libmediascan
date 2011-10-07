///-------------------------------------------------------------------------------------------------
/// @file D:\workspace\scan\src\util.c
///
///  utility functions.
///-------------------------------------------------------------------------------------------------

#ifdef WIN32
#include "win32/include/win32config.h"
#else
#include <pthread.h>
#include <sys/stat.h>
#include <errno.h>
#endif

// If we are on MSVC, disable some stupid MSVC warnings
#ifdef _MSC_VER
#pragma warning( disable: 4996 )
#define snprintf _snprintf
#endif

#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <libmediascan.h>

#include "common.h"
#include "util.h"

///-------------------------------------------------------------------------------------------------
///  Calculate a hash for a file
///
/// @author Henry Bennett
/// @date 04/09/2011
///
/// @param [in,out] file File to hash
/// @param [out] mtime Modification time of the file
/// @param [out] size File size
///
/// @return 32-bit file hash
///-------------------------------------------------------------------------------------------------

uint32_t HashFile(const char *file, int *mtime, uint64_t *size) {
  uint32_t hash;
  char fileData[MAX_PATH_STR_LEN];

#ifndef WIN32
  struct stat64 buf;
#else
  BOOL fOk;
  WIN32_FILE_ATTRIBUTE_DATA fileInfo;
#endif

  *mtime = 0;
  *size = 0;

#ifdef WIN32
  fOk = GetFileAttributesEx(file, GetFileExInfoStandard, (void *)&fileInfo);

  *mtime = fileInfo.ftLastWriteTime.dwLowDateTime;
  *size = ((uint64_t)fileInfo.nFileSizeHigh << 32) | fileInfo.nFileSizeLow;
#else
  if (stat64(file, &buf) != -1) {
    *mtime = (int)buf.st_mtime;
    *size = (uint64_t)buf.st_size;
  }
  else {
    LOG_ERROR("stat error on file %s, errno=%d\n", file, errno);
  }
#endif

  // Generate a hash of the full file path, modified time, and file size
  memset(fileData, 0, sizeof(fileData));
  snprintf(fileData, sizeof(fileData) - 1, "%s%d%llu", file, *mtime, *size);
  hash = hashlittle(fileData, strlen(fileData), 0);

  return hash;
}                               /* HashFile() */


// http://sws.dett.de/mini/hexdump-c/
void hex_dump(void *data, int size) {
  /* dumps size bytes of *data to stdout. Looks like:
   * [0000] 75 6E 6B 6E 6F 77 6E 20
   *                  30 FF 00 00 00 00 39 00 unknown 0.....9.
   * (in a single line of course)
   */

  unsigned char *p = data;
  unsigned char c;
  int n;
  char bytestr[4] = { 0 };
  char addrstr[10] = { 0 };
  char hexstr[16 * 3 + 5] = { 0 };
  char charstr[16 * 1 + 5] = { 0 };
  for (n = 1; n <= size; n++) {
    if (n % 16 == 1) {
      /* store address for this line */
      snprintf(addrstr, sizeof(addrstr), "%.4x", ((unsigned int)p - (unsigned int)data));
    }

    c = *p;
    if (isalnum(c) == 0) {
      c = '.';
    }

    /* store hex str (for left side) */
    snprintf(bytestr, sizeof(bytestr), "%02X ", *p);
    strncat(hexstr, bytestr, sizeof(hexstr) - strlen(hexstr) - 1);

    /* store char str (for right side) */
    snprintf(bytestr, sizeof(bytestr), "%c", c);
    strncat(charstr, bytestr, sizeof(charstr) - strlen(charstr) - 1);

    if (n % 16 == 0) {
      /* line completed */
      printf("[%4.4s]   %-50.50s  %s\n", addrstr, hexstr, charstr);
      hexstr[0] = 0;
      charstr[0] = 0;
    }
    else if (n % 8 == 0) {
      /* half line: add whitespaces */
      strncat(hexstr, "  ", sizeof(hexstr) - strlen(hexstr) - 1);
      strncat(charstr, " ", sizeof(charstr) - strlen(charstr) - 1);
    }
    p++;                        /* next byte */
  }

  if (strlen(hexstr) > 0) {
    /* print rest of buffer if not empty */
    printf("[%4.4s]   %-50.50s  %s\n", addrstr, hexstr, charstr);
  }
}
