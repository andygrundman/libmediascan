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
#include <libmediascan.h>


void InitCriticalSection(void *lp)
{
#ifdef WIN32
  if (!InitializeCriticalSectionAndSpinCount((LPCRITICAL_SECTION)lp, 0x00000400) ) 
    return NULL;
#else
  pthread_mutex_init((pthread_mutex_t *)lp, NULL);
#endif
}

void CleanupCriticalSection(void *lp)
{

#ifdef WIN32
  
	DeleteCriticalSection((LPCRITICAL_SECTION)lp);

#endif

}

void StartCriticalSection(void *lp)
{
#ifdef WIN32

	EnterCriticalSection((LPCRITICAL_SECTION)lp);

#else // POSIX

	pthread_mutex_lock( (pthread_mutex_t *)lp );

#endif

}

void EndCriticalSection(void *lp)
{
  #ifdef WIN32

    LeaveCriticalSection((LPCRITICAL_SECTION)lp);

  #else // POSIX
	
	pthread_mutex_unlock( (pthread_mutex_t *)lp );

  #endif
}


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

int match_file_extension (const char *filename, const char *extensions)
{
  const char *ext, *p;
  char ext1[32], *q;

  if (!filename)
    return 0;

  ext = strrchr (filename, '.');
  if (ext)
  {
    ext++;
    p = extensions;
    for (;;)
    {
      q = ext1;
      while (*p != '\0' && *p != ',' && (q - ext1 < (int) sizeof (ext1) - 1))
        *q++ = *p++;
      *q = '\0';
      if (!strcasecmp (ext1, ext))
        return 1;
      if (*p == '\0')
        break;
      p++;
    }
  }
  
  return 0;
} /* match_file_extension() */
