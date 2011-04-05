#ifndef _UTIL_H
#define _UTIL_H

int match_file_extension (const char *filename, const char *extensions);


void InitCriticalSection(void *lp);
void CleanupCriticalSection(void *lp);
void StartCriticalSection(void *lp);
void EndCriticalSection(void *lp);


#endif
