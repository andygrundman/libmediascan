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

#pragma comment(lib, "ws2_32.lib")


#pragma warning(disable: 4127)      // Conditional expression is a constant

#define DATA_BUFSIZE 9

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
  MediaScan *s = ((thread_data_type*)thread_data)->s;
  LPTSTR lpDir = ((thread_data_type*)thread_data)->lpDir;

  // Data variables for the windows directory change notification
  FILE_NOTIFY_INFORMATION Buffer[1024];
  char buf[256];
  char full_path[MAX_PATH];
  DWORD BytesReturned;
  FILE_NOTIFY_INFORMATION *pRecord;

  // Overlapped I/O variables
  OVERLAPPED oOverlap;
  WSAOVERLAPPED RecvOverlapped;
  HANDLE hDir;
  DWORD dwWaitStatus;
  DWORD dwBytesRead;
  BOOL bResult;
	WSABUF DataBuf;
  DWORD RecvBytes, Flags;
	char buffer[DATA_BUFSIZE];
	int rc;
	int err = 0;

  // Thread state variables
  int ThreadRunning = TRUE;

	// Make sure the RecvOverlapped struct is zeroed out
  SecureZeroMemory((PVOID) &oOverlap, sizeof(OVERLAPPED ) );

  // Create the event that will get fired when a directory changes
  oOverlap.hEvent = CreateEvent(NULL, // default security attributes
                                TRUE, // manual-reset event
                                FALSE,  // initial state is nonsignaled
                                "FileChangeEvent"); // "FileChangeEvent" name

  // Make sure the RecvOverlapped struct is zeroed out
  SecureZeroMemory((PVOID) &RecvOverlapped, sizeof(WSAOVERLAPPED) );

  // Create an event handle and setup an overlapped structure.
  RecvOverlapped.hEvent = WSACreateEvent();
	if (RecvOverlapped.hEvent  == NULL) {
      printf("WSACreateEvent failed: %d\n", WSAGetLastError());
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
                    FILE_FLAG_BACKUP_SEMANTICS, // file attributes
                    NULL);      // file with attributes to copy

  // Tell windows to monitor the folder asyncroniously 
  ReadDirectoryChangesW(hDir,   // handle to directory
                        &Buffer,  // read results buffer
                        sizeof(Buffer), // length of buffer
                        TRUE,   // Monitor sub-directories = TRUE
                        FILE_NOTIFY_CHANGE_SECURITY | FILE_NOTIFY_CHANGE_CREATION | FILE_NOTIFY_CHANGE_LAST_ACCESS | FILE_NOTIFY_CHANGE_LAST_WRITE | FILE_NOTIFY_CHANGE_SIZE | FILE_NOTIFY_CHANGE_ATTRIBUTES | FILE_NOTIFY_CHANGE_DIR_NAME | FILE_NOTIFY_CHANGE_FILE_NAME,  // filter conditions
                        &BytesReturned, // bytes returned
                        &oOverlap,  // We are using overlapped I/O
                        NULL    // Not using completion routine
    );

	Flags = 0;
	rc = WSARecv(s->thread->reqpipe[0], &DataBuf, 1, &RecvBytes, &Flags, &RecvOverlapped, NULL);
	if ( (rc == SOCKET_ERROR) && (WSA_IO_PENDING != (err = WSAGetLastError()))) {
    printf("WSARecv failed with error: %d\n", err);
    return;
	}

  // Run until we are told to stop. It is important to let the thread clean up after itself so 
  // there is shutdown code at the bottom of this function.
  while (ThreadRunning) {
    // Set up the events are going to wait for
		HANDLE event_list[2] = { oOverlap.hEvent, RecvOverlapped.hEvent };
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

        pRecord = Buffer;
        do {

          switch (pRecord->Action) {
            case FILE_ACTION_ADDED:  //helper_txt = "The file was added to the directory"; 
              break;
            case FILE_ACTION_REMOVED:  // helper_txt = "The file was removed from the directory"; 
              break;
            case FILE_ACTION_MODIFIED: // helper_txt = "The file was modified. This can be a change in the time stamp or attributes."; 
              break;
            case FILE_ACTION_RENAMED_OLD_NAME: // helper_txt = "The file was renamed and this is the old name."; 
              break;
            case FILE_ACTION_RENAMED_NEW_NAME: //helper_txt = "The file was renamed and this is the new name."; 
              break;
          }

          // Note pRecord->FileName is in UTF-16, have to convert it to 8 bits
          WideCharToMultiByte(CP_UTF8, 0, pRecord->FileName,  // the string you have
                              pRecord->FileNameLength / 2,  // length of the string
                              buf,  // output
                              _countof(buf),  // size of the buffer in bytes - if you leave it zero the return value is the length required for the output buffer
                              NULL, NULL);
          buf[pRecord->FileNameLength / 2] = 0;


          // Set up a full path to the file to be scanned and call ms_scan_fileCall scan here with the file changed
          strcpy(full_path, lpDir);
          strcat(full_path, "\\");
          strcat(full_path, buf);
          printf("Found File Changed: %s\n", full_path);

        ms_scan_file(s, full_path, TYPE_UNKNOWN);

          if (0 == pRecord->NextEntryOffset)
            break;
          pRecord = (FILE_NOTIFY_INFORMATION *) (&((unsigned char *)Buffer)[pRecord->NextEntryOffset]);
        } while (TRUE);

        // Manual-reset event should be reset since it is now signaled.
        ResetEvent(oOverlap.hEvent);

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

        //LOG_LEVEL(1, "\nNo changes in the timeout period.\n");
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
	WSACloseEvent(RecvOverlapped.hEvent);



  ExitThread(GetLastError());

}                               /* WatchDirectory() */
