#ifndef _UTIL_H
#define _UTIL_H

#include <stdint.h>

int match_file_extension(const char *filename, const char *extensions);


void InitCriticalSection(void *lp);
void CleanupCriticalSection(void *lp);
void StartCriticalSection(void *lp);
void EndCriticalSection(void *lp);

#ifdef WIN32
int _GetFileSize(const char *fileName, char *lpszString, long dwSize);
int _GetFileTime(const char *fileName, char *lpszString, long dwSize);
#endif

uint32_t hashlittle(const void *key, size_t length, uint32_t initval);
uint32_t HashFile(const char *file, int *mtime, size_t *size);
int TouchFile(const char *fileName);
void hex_dump(void *data, int size);


// In LIBDLNA profiles.c
int match_file_extension (const char *filename, const char *extensions);

#endif
