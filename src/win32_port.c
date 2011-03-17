///-------------------------------------------------------------------------------------------------
// file:	win32_port.c
//
// summary:	window 32 port class
///-------------------------------------------------------------------------------------------------

#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include < time.h >
#include <Windows.h>

#include "win32config.h"

#if defined(_MSC_VER) || defined(_MSC_EXTENSIONS)
  #define DELTA_EPOCH_IN_MICROSECS  11644473600000000Ui64
#else
  #define DELTA_EPOCH_IN_MICROSECS  11644473600000000ULL
#endif
 
struct timezone 
{
  int  tz_minuteswest; /* minutes W of Greenwich */
  int  tz_dsttime;     /* type of dst correction */
};

///-------------------------------------------------------------------------------------------------
///  Ends the program while outputting a final string to the console.
///
/// @author Henry Bennett
/// @date 03/15/2011
///
/// @param [in]	 fmt parameter list like printf.
///
/// ### remarks .
///-------------------------------------------------------------------------------------------------

void croak(char *fmt, ...) {
	va_list argptr;
	va_start(argptr, fmt);
	 
	printf(fmt, argptr);
    va_end(argptr);
	exit(-1);
};

///-------------------------------------------------------------------------------------------------
/// <summary>
/// 	The gettimeofday() function obtains the current time, expressed as seconds and
/// 	microseconds since the Epoch, and store it in the timeval structure pointed to by tv. As
/// 	posix says gettimeoday should return zero and should not reserve any value for error,
/// 	this function returns zero.
/// </summary>
///
/// <remarks>	Henry Bennett, 03/16/2011. </remarks>
///
/// <param name="tv">	[in,out] If non-null, the tv. </param>
/// <param name="tz">	[in,out] If non-null, the tz. </param>
///
/// <returns>	. </returns>
///-------------------------------------------------------------------------------------------------

int gettimeofday(struct timeval *tv, struct timezone *tz)
{
  FILETIME ft;
  unsigned __int64 tmpres = 0;
  static int tzflag;
 
  if (NULL != tv)
  {
    GetSystemTimeAsFileTime(&ft);
 
    tmpres |= ft.dwHighDateTime;
    tmpres <<= 32;
    tmpres |= ft.dwLowDateTime;
 
    /*converting file time to unix epoch*/
    tmpres -= DELTA_EPOCH_IN_MICROSECS; 
    tmpres /= 10;  /*convert into microseconds*/
    tv->tv_sec = (long)(tmpres / 1000000UL);
    tv->tv_usec = (long)(tmpres % 1000000UL);
  }
 
  if (NULL != tz)
  {
    if (!tzflag)
    {
      _tzset();
      tzflag++;
    }
    tz->tz_minuteswest = _timezone / 60;
    tz->tz_dsttime = _daylight;
  }
 
  return 0;
}