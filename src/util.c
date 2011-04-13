///-------------------------------------------------------------------------------------------------
/// @file D:\workspace\scan\src\util.c
///
///  utility functions.
///-------------------------------------------------------------------------------------------------

#ifdef WIN32
#include "win32/include/win32config.h"
#else
#include <pthread.h>
#endif

// If we are on MSVC, disable some stupid MSVC warnings
#ifdef _MSC_VER
#pragma warning( disable: 4996 )
#endif

#include <stdlib.h>
#include <string.h>
#include <libmediascan.h>

#include "common.h"
#include "util.h"

void InitCriticalSection(void *lp) {
#ifdef WIN32
  if (!InitializeCriticalSectionAndSpinCount((LPCRITICAL_SECTION) lp, 0x00000400))
    return;
#else
  pthread_mutex_init((pthread_mutex_t *) lp, NULL);
#endif
}

void CleanupCriticalSection(void *lp) {

#ifdef WIN32

  DeleteCriticalSection((LPCRITICAL_SECTION) lp);

#endif

}

void StartCriticalSection(void *lp) {
#ifdef WIN32

  EnterCriticalSection((LPCRITICAL_SECTION) lp);

#else // POSIX

  pthread_mutex_lock((pthread_mutex_t *) lp);

#endif

}

void EndCriticalSection(void *lp) {
#ifdef WIN32

  LeaveCriticalSection((LPCRITICAL_SECTION) lp);

#else // POSIX

  pthread_mutex_unlock((pthread_mutex_t *) lp);

#endif
}

///-------------------------------------------------------------------------------------------------
///  Calculate a hash for a file
///
/// @author Henry Bennett
/// @date 04/09/2011
///
/// @param [in,out] file File to hash
///
/// @return the file hash
///-------------------------------------------------------------------------------------------------

uint32_t HashFile(const char *file) {
  uint32_t hash;
  char fileData[MAX_PATH];

  // Generate a hash of the full file path, modified time, and file size
  hash = 0;
  hash = hashlittle(file, strlen(file), hash);  // Add path to hash

  _GetFileTime(file, fileData, MAX_PATH);
  hash = hashlittle(fileData, strlen(fileData), hash);  // Add time to hash

  _GetFileSize(file, fileData, MAX_PATH);
  hash = hashlittle(fileData, strlen(fileData), hash);  // Add file size to hash

  return hash;
}                               /* HashFile() */

///-------------------------------------------------------------------------------------------------
///  Match file extension.
///
/// @author Henry Bennett
/// @date 04/04/2011
///
/// @param filename   Filename of the file.
/// @param extensions The extensions.
///
/// @return .
///-------------------------------------------------------------------------------------------------

int match_file_extension(const char *filename, const char *extensions) {
  const char *ext, *p;
  char ext1[32], *q;

  if (!filename)
    return 0;

  ext = strrchr(filename, '.');
  if (ext) {
    ext++;
    p = extensions;
    for (;;) {
      q = ext1;
      while (*p != '\0' && *p != ',' && (q - ext1 < (int)sizeof(ext1) - 1))
        *q++ = *p++;
      *q = '\0';
      if (!strcasecmp(ext1, ext))
        return 1;
      if (*p == '\0')
        break;
      p++;
    }
  }

  return 0;
}                               /* match_file_extension() */

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
