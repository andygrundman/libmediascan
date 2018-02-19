///-------------------------------------------------------------------------------------------------
// file:  win32_port.c
//
// summary: window 32 port class
///-------------------------------------------------------------------------------------------------

#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include < time.h >
#include <Windows.h>

#include "win32config.h"

int strcasecmp(const char *string1, const char *string2) {
  return _stricmp(string1, string2);
}

int strncasecmp(const char *s1, const char *s2, size_t n) {
  return _strnicmp(s1, s2, n);
}

///-------------------------------------------------------------------------------------------------
///  Gets a file size.
///
/// @author Henry Bennett
/// @date 04/09/2011
///
/// @param fileName            Filename of the file.
/// @param [in,out] lpszString File size converted to a string
/// @param dwSize              String length
///
/// @return success.
///-------------------------------------------------------------------------------------------------

int _GetFileSize(const char *fileName, char *lpszString, long dwSize) {
  BOOL fOk;
  WIN32_FILE_ATTRIBUTE_DATA fileInfo;
  DWORD dwRet;

  if (NULL == fileName)
    return -1;

  fOk = GetFileAttributesEx(fileName, GetFileExInfoStandard, (void *)&fileInfo);
  if (!fOk)
    return -1;

  dwRet = sprintf_s(lpszString, dwSize, TEXT("%02d"), fileInfo.nFileSizeLow);

  return dwRet;
}                               /* _GetFileSize() */

 ///-------------------------------------------------------------------------------------------------
 ///  Gets a file's last modified time.
 ///
 /// @author Henry Bennett
 /// @date 04/09/2011
 ///
 /// @param fileName            Filename of the file.
 /// @param [in,out] lpszString Modified time formatted in a string
 /// @param dwSize              Length of a string
 ///
 /// @return success.
 ///-------------------------------------------------------------------------------------------------

int _GetFileTime(const char *fileName, char *lpszString, long dwSize) {
  BOOL fOk;
  WIN32_FILE_ATTRIBUTE_DATA fileInfo;
  SYSTEMTIME stUTC, stLocal;
  DWORD dwRet;

  if (NULL == fileName)
    return FALSE;

  fOk = GetFileAttributesEx(fileName, GetFileExInfoStandard, (void *)&fileInfo);
  if (!fOk)
    return FALSE;


  // Convert the last-write time to local time.
  FileTimeToSystemTime(&fileInfo.ftLastWriteTime, &stUTC);
  SystemTimeToTzSpecificLocalTime(NULL, &stUTC, &stLocal);

  // Build a string showing the date and time.
  dwRet = sprintf_s(lpszString, dwSize,
                    TEXT("%02d/%02d/%d  %02d:%02d"),
                    stLocal.wMonth, stLocal.wDay, stLocal.wYear, stLocal.wHour, stLocal.wMinute);

  if (S_OK == dwRet)
    return TRUE;
  else
    return FALSE;
}                               /* _GetFileTime() */

int TouchFile(const char *fileName) {
  HANDLE hFile;

  FILETIME ft;
  SYSTEMTIME st;
  int f;

  hFile = CreateFile(fileName,  // file to open
                     GENERIC_READ | GENERIC_WRITE,  // open for reading/writing
                     FILE_SHARE_READ, // share for reading
                     NULL,      // default security
                     OPEN_EXISTING, // existing file only
                     FILE_ATTRIBUTE_NORMAL, // normal file
                     NULL);     // no attr. template
  if (hFile == INVALID_HANDLE_VALUE) {
    return FALSE;
  }


  GetSystemTime(&st);           // Gets the current system time
  SystemTimeToFileTime(&st, &ft); // Converts the current system time to file time format
  f = SetFileTime(hFile,        // Sets last-write time of the file
                  (LPFILETIME) NULL,  // to the converted current system time
                  (LPFILETIME) NULL, &ft);

  CloseHandle(hFile);

  return f;
}

///-------------------------------------------------------------------------------------------------
///  Ends the program while outputting a final string to the console.
///
/// @author Henry Bennett
/// @date 03/15/2011
///
/// @param [in]  fmt parameter list like printf.
///
/// ### remarks .
///-------------------------------------------------------------------------------------------------

void croak(char *fmt, ...) {
  va_list argptr;
  va_start(argptr, fmt);

  printf(fmt, argptr);
  va_end(argptr);
  //exit(-1);
}                               /* croak() */
