#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif


#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>

#include <stdlib.h>
#include <stdio.h>
#include <tchar.h>
#include <libmediascan.h>

#include "mediascan.h"
#include "common.h"
#include "util.h"
#include "thread.h"
#include "error.h"
#include "database.h"

#pragma comment(lib, "ws2_32.lib")


#pragma warning(disable: 4127)  // Conditional expression is a constant

#define DATA_BUFSIZE 9
#define FILE_BUFFER_SZ 1024
#define DETECTION_FILTER (FILE_NOTIFY_CHANGE_LAST_WRITE|FILE_NOTIFY_CHANGE_CREATION|FILE_NOTIFY_CHANGE_FILE_NAME)


static void HandleRemovedFile(MediaScan *s, const char *filename) {
  DBT data, key;
  int ret;

  // Zero out the DBTs before using them.
  memset(&key, 0, sizeof(key));
  memset(&data, 0, sizeof(DBT));

  key.data = (char *)filename;
  key.size = strlen(filename) + 1;

  if (s->dbp != NULL) {

    if ((ret = s->dbp->get(s->dbp, NULL, &key, &data, 0)) == 0) {
      LOG_INFO("db: %s: key retrieved: data was %s.\n", (char *)key.data, (char *)data.data);

      if ((ret = s->dbp->del(s->dbp, NULL, &key, 0)) == 0) {
        LOG_INFO("db: %s: key was deleted.\n", (char *)key.data);
      }
      else {
        s->dbp->err(s->dbp, ret, "DB->del");
      }
    }
    else {
      s->dbp->err(s->dbp, ret, "DB->get");
    }
  }
}                               /* HandleRemovedFile() */

static BOOL WaitForFile(const char *sz, const DWORD dwWaitSecs) {
  DWORD dwEnd = GetTickCount() + (dwWaitSecs * 1000);
  DWORD err;
  while (dwEnd > GetTickCount()) {
    HANDLE h = CreateFile(sz, GENERIC_READ | GENERIC_WRITE,
                          0,    // exclusive mode
                          NULL,
                          OPEN_EXISTING, 0, 0);

    if (h != INVALID_HANDLE_VALUE) {
      CloseHandle(h);
      return TRUE;
    }

    err = GetLastError();
    if (err == ERROR_FILE_NOT_FOUND) {
      LOG_INFO("%s no longer exsists\n", sz);
      break;
    }
    else if (err != ERROR_SHARING_VIOLATION) {
      LOG_ERROR("WaitForFile errno:%d on file: %s\n", err, sz);
      break;
    }

    Sleep(500);
  }
  return FALSE;
}                               /* WaitForFile() */

///-------------------------------------------------------------------------------------------------
///  Watch directory.
///
/// @author Henry Bennett
/// @date 03/22/2011
///
/// @param thread_data_type Structure containing the MediaScan pointer and the directory to scan
///-------------------------------------------------------------------------------------------------

void WatchDirectory(void *thread_data) {
  // Copy thread inputs to local variables
  MediaScan *s = ((thread_data_type *)thread_data)->s;
  LPTSTR lpDir = ((thread_data_type *)thread_data)->lpDir;

  // Data variables for the windows directory change notification
  OVERLAPPED oOverlap;
  FILE_NOTIFY_INFORMATION Buffer[FILE_BUFFER_SZ];
  char buf[256];
  char full_path[MAX_PATH];
  DWORD BytesReturned;

  // Overlapped I/O variables
  WSAOVERLAPPED RecvOverlapped;
  HANDLE hDir;
  DWORD dwWaitStatus;
  DWORD dwBytesRead;
  BOOL bResult;
  WSABUF DataBuf;
  DWORD RecvBytes, Flags;
  char buffer[DATA_BUFSIZE];
  int rc = 0;
  int err = 0;
  char *pBase = 0;

  // Thread state variables
  int ThreadRunning = TRUE;

  // Initialize the cache database
  if (!init_bdb(s)) {
    MediaScanError *e = error_create("", MS_ERROR_CACHE, "Unable to initialize libmediascan cache");
    send_error(s, e);
  }

  SecureZeroMemory(buffer, DATA_BUFSIZE);

  // Make sure the RecvOverlapped struct is zeroed out
  SecureZeroMemory((PVOID) & oOverlap, sizeof(OVERLAPPED));

  // Create the event that will get fired when a directory changes
  oOverlap.hEvent = CreateEvent(NULL, // default security attributes
                                TRUE, // manual-reset event
                                FALSE,  // initial state is nonsignaled
                                TEXT("MyFileChangeEvent")); // "FileChangeEvent" name

  // Make sure the RecvOverlapped struct is zeroed out
  SecureZeroMemory((PVOID) & RecvOverlapped, sizeof(WSAOVERLAPPED));

  // Create an event handle and setup an overlapped structure.
  RecvOverlapped.hEvent = WSACreateEvent();
  if (RecvOverlapped.hEvent == NULL) {
    LOG_ERROR("WSACreateEvent failed: %d\n", WSAGetLastError());
    return;
  }

  DataBuf.len = DATA_BUFSIZE;
  DataBuf.buf = buffer;

  // Open the directory that we are going to have windows monitor, note the share modes
  hDir = CreateFile(lpDir,      // pointer to the file name
                    FILE_LIST_DIRECTORY,  // access (read/write) mode
                    FILE_SHARE_READ | FILE_SHARE_DELETE | FILE_SHARE_WRITE, // share mode
                    NULL,       // security descriptor
                    OPEN_EXISTING,  // how to create
                    FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED,  // file attributes
                    NULL);      // file with attributes to copy

  // Tell windows to monitor the folder asyncroniously using overlapped I/O
  ReadDirectoryChangesW(hDir,   // handle to directory
                        &Buffer,  // read results buffer
                        FILE_BUFFER_SZ * sizeof(FILE_NOTIFY_INFORMATION), // length of buffer
                        TRUE,   // Monitor sub-directories = TRUE
                        DETECTION_FILTER, // filter conditions
                        &BytesReturned, // bytes returned
                        &oOverlap,  // We are using overlapped I/O
                        NULL);  // Not using completion routine

  Flags = 0;
  rc = WSARecv(s->thread->reqpipe[0], &DataBuf, 1, &RecvBytes, &Flags, &RecvOverlapped, NULL);
  if ((rc == SOCKET_ERROR) && (WSA_IO_PENDING != (err = WSAGetLastError()))) {
    LOG_ERROR("WSARecv failed with error: %d\n", err);
    return;
  }

  // Run until we are told to stop. It is important to let the thread clean up after itself so 
  // there is shutdown code at the bottom of this function.
  while (ThreadRunning) {

    // Set up the events are going to wait for
    HANDLE event_list[2] = { oOverlap.hEvent, RecvOverlapped.hEvent };
    FILE_NOTIFY_INFORMATION *fni;
    LOG_LEVEL(1, "\nWaiting for notification...\n");

    // Wait forever for notification.
    dwWaitStatus = WaitForMultipleObjects(2,  // number of objects in array
                                          event_list, // array of objects
                                          FALSE,  // wait for any object
                                          INFINITE);  // infinite wait

    switch (dwWaitStatus) {
        // This is for the event oOverlap.hEvent
      case WAIT_OBJECT_0:
        bResult = GetOverlappedResult(hDir, &oOverlap, &dwBytesRead, TRUE);

        pBase = (char *)Buffer;
        do {

          fni = (FILE_NOTIFY_INFORMATION *) pBase;

          // Note pRecord->FileName is in UTF-16, have to convert it to 8 bits
          WideCharToMultiByte(CP_UTF8, 0, fni->FileName,  // the string you have
                              fni->FileNameLength / 2,  // length of the string
                              buf,  // output
                              _countof(buf),  // size of the buffer in bytes - if you leave it zero the return value is the length required for the output buffer
                              NULL, NULL);
          buf[fni->FileNameLength / 2] = 0;


          // Set up a full path to the file to be scanned and call ms_scan_fileCall scan here with the file changed
          strcpy(full_path, lpDir);
          strcat(full_path, "\\");
          strcat(full_path, buf);
          LOG_INFO("Found File Changed: %s", full_path);

          switch (fni->Action) {

            case FILE_ACTION_ADDED:
              LOG_INFO("  file was added\n");

              // We get notifications even while a file is being copied. This will wait 60 seconds for the file to finish.
              if (WaitForFile(full_path, 10)) {
                ms_scan_file(s, full_path, TYPE_UNKNOWN);
              }
              else {
                LOG_ERROR("A file found by the background scanner never finished copying");
              }

              break;
            case FILE_ACTION_REMOVED:
              LOG_INFO("  file was removed\n");
              HandleRemovedFile(s, full_path);
              break;

            case FILE_ACTION_MODIFIED:
              LOG_INFO("	file was modified\n");  // This can be a change in the time stamp or attributes."; 
              // We get notifications even while a file is being copied. This will wait 60 seconds for the file to finish.
              if (WaitForFile(full_path, 10)) {
                ms_scan_file(s, full_path, TYPE_UNKNOWN);
              }
              else {
                LOG_ERROR("A file found by the background scanner never finished copying");
              }

              break;
/*            case FILE_ACTION_RENAMED_OLD_NAME: 
								LOG_INFO("	file was renamed and this is the old name\n"); 
              break;
            case FILE_ACTION_RENAMED_NEW_NAME: 
								LOG_INFO("	file was renamed and this is the new name\n"); 
              break;
*/ }

          if (!fni->NextEntryOffset)
            break;
          pBase += fni->NextEntryOffset;
        } while (TRUE);

        SecureZeroMemory((PVOID) Buffer, sizeof(Buffer));
        ReadDirectoryChangesW(hDir, // handle to directory
                              &Buffer,  // read results buffer
                              sizeof(Buffer), // length of buffer
                              TRUE, // Monitor sub-directories = TRUE
                              DETECTION_FILTER, // filter conditions
                              &BytesReturned, // bytes returned
                              &oOverlap,  // We are using overlapped I/O
                              NULL);  // Not using completion routine

        break;                  /* WAIT_OBJECT_0: */

        // This is for the event s->ghSignalEvent
      case WAIT_OBJECT_0 + 1:

        ThreadRunning = FALSE;

        break;                  /* WAIT_OBJECT_0 + 1: */

      case WAIT_TIMEOUT:

        // A timeout occurred, this would happen if some value other 
        // than INFINITE is used in the Wait call and no changes occur.
        // In a single-threaded environment you might not want an
        // INFINITE wait.

        LOG_INFO("\nNo changes in the timeout period.\n");
        break;

      default:
        LOG_ERROR("\n ERROR: Unhandled dwWaitStatus.\n");
        ExitThread(GetLastError());
        break;
    }
  }

  // Free the data that was passed to this thread on the heap
  if (thread_data != NULL) {
    free(thread_data);
    thread_data = NULL;         // Ensure address is not reused.
  }

  CancelIo(hDir);

  if (!HasOverlappedIoCompleted(&oOverlap)) {
    SleepEx(5, TRUE);
  }

  WSACloseEvent(RecvOverlapped.hEvent);
  CloseHandle(oOverlap.hEvent);


  ExitThread(GetLastError());

}                               /* WatchDirectory() */
