#ifndef _UTIL_H
#define _UTIL_H

#include <stdint.h>  

int match_file_extension (const char *filename, const char *extensions);


void InitCriticalSection(void *lp);
void CleanupCriticalSection(void *lp);
void StartCriticalSection(void *lp);
void EndCriticalSection(void *lp);

#ifdef WIN32
int _GetFileSize(const char *fileName, LPTSTR lpszString, DWORD dwSize);
int _GetFileTime(const char *fileName, LPTSTR lpszString, DWORD dwSize);
#endif

uint32_t hashlittle(const void *key, size_t length, uint32_t initval);

void hex_dump(void *data, int size);

#endif
