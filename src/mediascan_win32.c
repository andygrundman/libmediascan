///-------------------------------------------------------------------------------------------------
/// @file libmediascan\src\mediascan_win32.c
///
///  mediascan window 32 class.
///-------------------------------------------------------------------------------------------------

#include <Windows.h>
#include <tchar.h>
#include <stdio.h>
#include <stdlib.h>
#include <strsafe.h>
#include <direct.h>
#include <tchar.h>
#include <Msi.h>
#include <Shobjidl.h>
#include <objbase.h>
#include <objidl.h>
#include <shlguid.h>
#include <shlobj.h>             /* For IShellLink */
#include <Shlwapi.h>

#include <libmediascan.h>


#include "common.h"
#include "queue.h"
#include "mediascan.h"
#include "progress.h"

#ifdef _MSC_VER
#pragma warning( disable: 4127 )
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "uuid.lib")
#pragma comment(lib, "Msi.lib")
#endif

int parse_lnk(const char *path, LPTSTR szTarget, SIZE_T cchTarget) {
  char szProductCode[39];
  char szFeatureId[MAX_FEATURE_CHARS + 1];
  char szComponentCode[39];
  IShellLink *psl = NULL;
  IPersistFile *ppf = NULL;
  BOOL bResult = FALSE;

#if !defined(UNICODE)
  WCHAR wsz[MAX_PATH_STR_LEN];
  if (0 == MultiByteToWideChar(CP_ACP, 0, path, -1, wsz, MAX_PATH_STR_LEN))
    goto cleanup;
#else
  LPCWSTR wsz = szShortcutFile;
#endif

  // First check if this is a shell lnk or some other kind of link
  // http://msdn.microsoft.com/en-us/library/aa370299%28VS.85%29.aspx
  // if( MsiGetShortcutTarget(path, szProductCode, szFeatureId, szComponentCode) == ERROR_FUNCTION_FAILED )
  //   goto cleanup;   // This means it is a shell lnk and can be looked at by IShellLink

  // Now parse the link using the IShellLink COM interface
  // http://msdn.microsoft.com/en-us/library/bb776891%28v=vs.85%29.aspx
  if (FAILED(CoCreateInstance(&CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, &IID_IShellLink, (void **)&psl)))
    goto cleanup;

  if (FAILED(psl->lpVtbl->QueryInterface(psl, &IID_IPersistFile, (void **)&ppf)))
    goto cleanup;

  if (FAILED(ppf->lpVtbl->Load(ppf, wsz, STGM_READ)))
    goto cleanup;

  if (NOERROR != psl->lpVtbl->GetPath(psl, szTarget, cchTarget, NULL, 0))
    goto cleanup;

  bResult = TRUE;

cleanup:
  if (ppf)
    ppf->lpVtbl->Release(ppf);
  if (psl)
    psl->lpVtbl->Release(psl);
  if (!bResult && cchTarget != 0)
    szTarget[0] = TEXT('\0');
  return bResult;
}                               /* parse_lnk() */

///-------------------------------------------------------------------------------------------------
///  Recursively walk a directory struction using Win32 style directory commands
///
/// @author Henry Bennett
/// @date 03/15/2011
///
/// @param [in,out] s    If non-null, the.
/// @param path        Full pathname of the file.
/// @param [in,out] curdir If non-null, the curdir.
///
/// ### remarks .
///-------------------------------------------------------------------------------------------------

void recurse_dir(MediaScan *s, const char *path, int recurse_count) {
  char *dir = NULL;
  char *p = NULL;
  char *tmp_full_path;
  struct dirq_entry *parent_entry = NULL; // entry for current dir in s->_dirq
  struct dirq *subdirq;         // list of subdirs of the current directory
  char redirect_dir[MAX_PATH_STR_LEN];

  // Windows directory browsing variables
  HANDLE hFind = INVALID_HANDLE_VALUE;
  WIN32_FIND_DATA ffd;
  DWORD dwError = 0;
  TCHAR findDir[MAX_PATH_STR_LEN];

  recurse_count++;
  if (recurse_count > RECURSE_LIMIT) {
    LOG_ERROR("Hit recurse limit of %d scanning path %s\n", RECURSE_LIMIT, path);
    return;
  }

  if (s == NULL) {
    ms_errno = MSENO_NULLSCANOBJ;
    LOG_ERROR("MediaScan = NULL, aborting scan\n");
    return;
  }

  if (!is_absolute_path(path)) {
    // Get full path
    char *buf = (char *)malloc(MAX_PATH_STR_LEN);
    if (buf == NULL) {
      LOG_ERROR("Out of memory for directory scan\n");
      return;
    }

    dir = _getcwd(buf, MAX_PATH_STR_LEN);

    strcat_s(dir, MAX_PATH_STR_LEN, "\\");
    strcat_s(dir, MAX_PATH_STR_LEN, path);
  }
  else {
    dir = _strdup(path);
  }


  // Strip trailing slash if any
  p = &dir[0];
  while (*p != 0) {
    if (p[1] == 0 && (*p == '/' || *p == '\\'))
      *p = 0;
    p++;
  }

  LOG_LEVEL(2, "Recursed into %s\n", dir);


  // Prepare string for use with FindFile functions.  First, copy the
  // string to a buffer, then append '\*' to the directory name.
  StringCchCopy(findDir, MAX_PATH_STR_LEN, dir);
  StringCchCat(findDir, MAX_PATH_STR_LEN, TEXT("\\*"));


  // Find the first file in the directory.
  hFind = FindFirstFile(findDir, &ffd);
  if (INVALID_HANDLE_VALUE == hFind) {
    LOG_ERROR("Unable to open directory %s errno %d\n", dir, MSENO_DIRECTORYFAIL);
    ms_errno = MSENO_DIRECTORYFAIL;
    goto out;
  }


  subdirq = malloc(sizeof(struct dirq));
  SIMPLEQ_INIT(subdirq);

  tmp_full_path = malloc(MAX_PATH_STR_LEN);


  do {

    char *name = ffd.cFileName;

    // skip all dot files
    if (name[0] != '.') {
      // Check if scan should be aborted
      if (unlikely(s->_want_abort))
        break;

      if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
        struct dirq_entry *subdir_entry = malloc(sizeof(struct dirq_entry));

        // Construct full path
        *tmp_full_path = 0;
        strcat_s(tmp_full_path, MAX_PATH_STR_LEN, dir);
        strcat_s(tmp_full_path, MAX_PATH_STR_LEN, "\\");
        strcat_s(tmp_full_path, MAX_PATH_STR_LEN, name);

        if (_should_scan_dir(s, tmp_full_path)) {
          subdir_entry->dir = _strdup(tmp_full_path);
          SIMPLEQ_INSERT_TAIL(subdirq, subdir_entry, entries);
          LOG_INFO(" subdir: %s\n", tmp_full_path);
        }
        else {
          LOG_INFO(" skipping subdir: %s\n", tmp_full_path);
        }
      }
      else {
        enum media_type type = _should_scan(s, name);

        // Check if this file is a shortcut and if so resolve it
        if (type == TYPE_LNK) {
          char full_name[MAX_PATH_STR_LEN];
          strcpy(full_name, dir);
          strcat(full_name, "\\");
          strcat(full_name, name);
          parse_lnk(full_name, redirect_dir, MAX_PATH_STR_LEN);
          if (PathIsDirectory(redirect_dir)) {
            struct dirq_entry *subdir_entry = malloc(sizeof(struct dirq_entry));
            subdir_entry->dir = _strdup(redirect_dir);
            SIMPLEQ_INSERT_TAIL(subdirq, subdir_entry, entries);
            LOG_INFO("shortcut dir: %s\n", redirect_dir);
            type = 0;
          }

        }
        if (type) {
          struct fileq_entry *entry;

          if (parent_entry == NULL) {
            // Add parent directory to list of dirs with files
            parent_entry = malloc(sizeof(struct dirq_entry));
            parent_entry->dir = _strdup(dir);
            parent_entry->files = malloc(sizeof(struct fileq));
            SIMPLEQ_INIT(parent_entry->files);
            SIMPLEQ_INSERT_TAIL((struct dirq *)s->_dirq, parent_entry, entries);
          }

          // Add scannable file to this directory list
          entry = malloc(sizeof(struct fileq_entry));
          entry->file = _strdup(name);
          entry->type = type;
          SIMPLEQ_INSERT_TAIL(parent_entry->files, entry, entries);

          s->progress->total++;

          LOG_INFO(" [%5d] file: %s\n", s->progress->total, entry->file);
        }
      }
    }
  } while (FindNextFile(hFind, &ffd) != 0);

  dwError = GetLastError();
  if (dwError != ERROR_NO_MORE_FILES) {
    LOG_ERROR("Error searching files.");
  }

  FindClose(hFind);

  LOG_INFO("Going to send progress update\n");
  // Send progress update
  if (s->on_progress && !s->_want_abort)
    if (progress_update(s->progress, dir))
      send_progress(s);

  // process subdirs
  while (!SIMPLEQ_EMPTY(subdirq)) {
    struct dirq_entry *subdir_entry = SIMPLEQ_FIRST(subdirq);
    SIMPLEQ_REMOVE_HEAD(subdirq, entries);
    if (!s->_want_abort)
      recurse_dir(s, subdir_entry->dir, recurse_count);
    free(subdir_entry);
  }
  free(subdirq);
  free(tmp_full_path);

out:
  free(dir);
}                               /* recurse_dir() */
