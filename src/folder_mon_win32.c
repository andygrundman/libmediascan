#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <tchar.h>
#include <libmediascan.h>

#include "mediascan.h"
#include "common.h"

//extern HANDLE ghSignalEvent; 

///-------------------------------------------------------------------------------------------------
///  Code to refresh the directory listing, but not the subtree because it would not be necessary.
///
/// @author Henry Bennett
/// @date 03/22/2011
///
/// @param lpDir The pointer to a dir.
///-------------------------------------------------------------------------------------------------

void RefreshDirectory(MediaScan *s, LPTSTR lpDir)
{
   struct dirq_entry *entry;
   char *phase = NULL;

   LOG_LEVEL(1, "Directory (%s) changed.\n", lpDir);

   // The following code is not yet thread safe
   /*
    entry = malloc(sizeof(struct dirq_entry));
    entry->dir = strdup("/"); // so free doesn't choke on this item later
    entry->files = malloc(sizeof(struct fileq));
    SIMPLEQ_INIT(entry->files);
    SIMPLEQ_INSERT_TAIL((struct dirq *)s->_dirq, entry, entries);
    
    phase = (char *)malloc(MAX_PATH);
    sprintf(phase, "Discovering files in %s", lpDir);
    s->progress->phase = phase;
    
    LOG_LEVEL(1, "Scanning %s\n", lpDir);
    recurse_dir(s, lpDir, entry);
    
    free(phase);
	*/

} /* RefreshDirectory() */

///-------------------------------------------------------------------------------------------------
///  Watch directory.
///
/// @author Henry Bennett
/// @date 03/22/2011
///
/// @param lpDir String describing the path to be watched
///-------------------------------------------------------------------------------------------------

DWORD WINAPI WatchDirectory(thread_data_type *thread_data)
{
   DWORD dwWaitStatus; 
   HANDLE dwChangeHandle; 
   TCHAR lpDrive[4];
   TCHAR lpFile[_MAX_FNAME];
   TCHAR lpExt[_MAX_EXT];
   int ThreadRunning = TRUE;
   MediaScan *s = thread_data->s;
   LPTSTR lpDir = thread_data->lpDir;



   _tsplitpath_s(lpDir, lpDrive, 4, NULL, 0, lpFile, _MAX_FNAME, lpExt, _MAX_EXT);

   lpDrive[2] = (TCHAR)'\\';
   lpDrive[3] = (TCHAR)'\0';
 
// Watch the directory for file creation and deletion. 
 
   dwChangeHandle = FindFirstChangeNotification( 
      lpDir,                         // directory to watch 
      TRUE,                         // do not watch subtree 
      FILE_NOTIFY_CHANGE_FILE_NAME); // watch file name changes 
 
   if (dwChangeHandle == INVALID_HANDLE_VALUE) 
   {
     LOG_ERROR("\n ERROR: FindFirstChangeNotification function failed.\n");
     ExitThread(GetLastError()); 
   }
 

// Make a final validation check on our handles.

   if ((dwChangeHandle == NULL))
   {
    LOG_ERROR("\n ERROR: Unexpected NULL from FindFirstChangeNotification.\n");

	 if(dwChangeHandle == NULL)
 		FindCloseChangeNotification(dwChangeHandle);

     ExitThread(GetLastError()); 
   }

// Change notification is set. Now wait on both notification 
// handles and refresh accordingly. 
 
   while (ThreadRunning) 
   { 
	  HANDLE event_list[2] = {dwChangeHandle, s->ghSignalEvent};
   // Wait for notification.
 
      LOG_LEVEL(1, "\nWaiting for notification...\n");

	  dwWaitStatus = WaitForMultipleObjects( 
			2,           // number of objects in array
	        event_list,     // array of objects
			FALSE,       // wait for any object
			INFINITE);       // five-second wait


      switch (dwWaitStatus) 
      { 
		case WAIT_OBJECT_0: 
 
         // A file was created, renamed, or deleted in the directory.
         // Refresh this directory and restart the notification.
 
             RefreshDirectory(s, lpDir); 
             if ( FindNextChangeNotification(dwChangeHandle) == FALSE )
             {
               LOG_ERROR("\n ERROR: FindNextChangeNotification function failed.\n");
			   FindCloseChangeNotification(dwChangeHandle);
               ExitThread(GetLastError()); 
             }
             break; 

		case WAIT_OBJECT_0 + 1: 
 
				ThreadRunning = FALSE;

             break; 

         case WAIT_TIMEOUT:

         // A timeout occurred, this would happen if some value other 
         // than INFINITE is used in the Wait call and no changes occur.
         // In a single-threaded environment you might not want an
         // INFINITE wait.
 
            LOG_LEVEL(1, "\nNo changes in the timeout period.\n");
            break;

         default: 
            LOG_ERROR("\n ERROR: Unhandled dwWaitStatus.\n");
 		    FindCloseChangeNotification(dwChangeHandle);
            ExitThread(GetLastError());
            break;
      }
   }

  FindCloseChangeNotification(dwChangeHandle);

  if(thread_data != NULL)
	{
	HeapFree(GetProcessHeap(), 0, thread_data);
	thread_data = NULL;    // Ensure address is not reused.
	}


  ExitThread(GetLastError());
} /* WatchDirectory() */


