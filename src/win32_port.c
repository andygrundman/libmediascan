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


int _GetFileSize(const char *fileName, LPTSTR lpszString, DWORD dwSize)
{
    BOOL                        fOk;
    WIN32_FILE_ATTRIBUTE_DATA   fileInfo;
		DWORD												dwRet;

    if (NULL == fileName)
        return -1;

    fOk = GetFileAttributesEx(fileName, GetFileExInfoStandard, (void*)&fileInfo);
    if (!fOk)
        return -1;

		dwRet = sprintf_s(lpszString, dwSize, 
        TEXT("%02d"), fileInfo.nFileSizeLow);

		return dwRet;
}

 int _GetFileTime(const char *fileName, LPTSTR lpszString, DWORD dwSize)
{
    BOOL                        fOk;
    WIN32_FILE_ATTRIBUTE_DATA   fileInfo;
		SYSTEMTIME								  stUTC, stLocal;
		DWORD												dwRet;

    if (NULL == fileName)
        return FALSE;

    fOk = GetFileAttributesEx(fileName, GetFileExInfoStandard, (void*)&fileInfo);
    if (!fOk)
        return FALSE;


		// Convert the last-write time to local time.
		FileTimeToSystemTime(&fileInfo.ftLastWriteTime, &stUTC);
    SystemTimeToTzSpecificLocalTime(NULL, &stUTC, &stLocal);

    // Build a string showing the date and time.
    dwRet = sprintf_s(lpszString, dwSize, 
        TEXT("%02d/%02d/%d  %02d:%02d"),
        stLocal.wMonth, stLocal.wDay, stLocal.wYear,
        stLocal.wHour, stLocal.wMinute);

    if( S_OK == dwRet )
        return TRUE;
    else return FALSE;
}


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
	//exit(-1);
};

