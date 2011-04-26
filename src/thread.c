
// Cross-platform thread abstractions

#include <errno.h>
#include <libmediascan.h>
#include <stdlib.h>
#include <string.h>

#include "mediascan.h"
#include "common.h"
#include "error.h"
#include "thread.h"
#include "queue.h"

#ifdef USE_SOCKETS_AS_HANDLES
# define S_TO_HANDLE(x) ((HANDLE)win32_get_osfhandle (x))
#else
# define S_TO_HANDLE(x) ((HANDLE)x)
#endif

struct equeue_entry {
  enum event_type type;
  void *data;
    TAILQ_ENTRY(equeue_entry) entries;
};
TAILQ_HEAD(equeue, equeue_entry);

MediaScanThread *thread_create(void *(*func) (void *), thread_data_type *thread_data) {
  int err;
  MediaScanThread *t = (MediaScanThread *)calloc(sizeof(MediaScanThread), 1);
  if (t == NULL) {
    LOG_ERROR("Out of memory for new MediaScanThread object\n");
    goto fail;
  }

  LOG_MEM("new MediaScanThread @ %p\n", t);

  // Setup event queue
  t->event_queue = malloc(sizeof(struct equeue));
  TAILQ_INIT((struct equeue *)t->event_queue);
  LOG_MEM("new equeue @ %p\n", t->event_queue);

#ifndef WIN32
  // Setup pipes for communication with main thread
  if (pipe(t->respipe)) {
    LOG_ERROR("Unable to initialize thread result pipe\n");
    goto fail;
  }

  if (pipe(t->reqpipe)) {
    LOG_ERROR("Unable to initialize thread request pipe\n");
    goto fail;
  }

  if (pthread_mutex_init(&t->mutex, NULL) != 0) {
    LOG_ERROR("Unable to initialize thread mutex\n");
    goto fail;
  }

  // Launch thread
  err = pthread_create(&t->tid, NULL, func, thread_data->s);
  if (err != 0) {
    LOG_ERROR("Unable to create thread (%s)\n", strerror(err));
    goto fail;
  }

  LOG_DEBUG("Thread %x started\n", t->tid);
#else

  t->ghSignalEvent = CreateEvent(NULL,  // default security attributes
                                 TRUE,  // manual-reset event
                                 FALSE, // initial state is nonsignaled
                                 "StopEvent"  // "StopEvent" name
    );

  if (t->ghSignalEvent == NULL) {
    ms_errno = MSENO_THREADERROR;
    LOG_ERROR("Can't create event\n");
    goto fail;
  }

//  thread_data = (thread_data_type *) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(thread_data_type));


//  thread_data->lpDir = NULL; //(char *)path;
//  thread_data->s = thread_data;

  t->hThread = CreateThread(NULL, // default security attributes
                            0,  // use default stack size
                            (LPTHREAD_START_ROUTINE) func,  // WatchDirectory thread
                            (void *)thread_data,  // (void*)thread_data_type
                            0,  // use default creation flags
                            &t->dwThreadId);  // returns the thread identifier

  if (t->hThread == NULL) {
    ms_errno = MSENO_THREADERROR;
    LOG_ERROR("Can't create watch thread\n");
    goto fail;
  }

  if (!InitializeCriticalSectionAndSpinCount(&t->CriticalSection, 0x00000400)) {
    LOG_ERROR("Unable to initialize critical section\n");
    goto fail;
  }

  LOG_DEBUG("Win32 thread %x started\n", t->dwThreadId);

#endif

  goto out;

fail:
  t = NULL;

out:
  return t;
}

// Return the file descriptor that should be watched for
// by the main thread for notifications that events are waiting
int thread_get_result_fd(MediaScanThread *t) {
#ifndef WIN32
  return t->respipe[0];
#endif
}

// Queue a new event
void thread_queue_event(MediaScanThread *t, enum event_type type, void *data) {
  struct equeue_entry *entry = malloc(sizeof(struct equeue_entry));
  LOG_MEM("new equeue_entry @ %p (type %d, data @ %p)\n", entry, type, data);

  entry->type = type;
  entry->data = data;

  thread_lock(t);
  TAILQ_INSERT_TAIL((struct equeue *)t->event_queue, entry, entries);
  thread_unlock(t);

  // Signal respipe that a new event is ready
  thread_signal(t->respipe);
}

// Return the next queued event
enum event_type thread_get_next_event(MediaScanThread *t, void **data_out) {
  char buf[4];
  struct equeue *eq = (struct equeue *)t->event_queue;
  struct equeue_entry *entry = NULL;
  enum event_type type = 0;

  thread_lock(t);

  if (TAILQ_EMPTY(eq)) {
    *data_out = NULL;
    goto out;
  }

  // Pop the first item in the queue
  entry = TAILQ_FIRST(eq);
  TAILQ_REMOVE(eq, entry, entries);

  type = entry->type;
  *data_out = entry->data;

  LOG_MEM("destroy equeue_entry @ %p\n", entry);
  free(entry);

out:
  thread_unlock(t);

  return type;
}

void thread_lock(MediaScanThread *t) {
#ifndef WIN32
  pthread_mutex_lock(&t->mutex);
#else
  EnterCriticalSection(&t->CriticalSection);
#endif
}

void thread_unlock(MediaScanThread *t) {
#ifndef WIN32
  pthread_mutex_unlock(&t->mutex);
#else
  LeaveCriticalSection(&t->CriticalSection);
#endif
}

void thread_signal(int spipe[2]) {
#ifndef WIN32
  static uint64_t counter = 1;

  LOG_DEBUG("thread_signal -> %d\n", spipe[1]);
  if (write(spipe[1], &counter, 1) < 0)
    LOG_ERROR("Error signalling thread: %s\n", strerror(errno));
#else
  DWORD dummy;

  LOG_DEBUG("thread_signal -> %d\n", spipe[1]);
  WriteFile(S_TO_HANDLE(spipe[1]), (LPCVOID)&dummy, 1, &dummy, 0);
#endif
}

void thread_signal_read(int spipe[2]) {
  char buf[9];

  LOG_DEBUG("thread_signal_read <- %d waiting...\n", spipe[0]);

#ifndef WIN32
  read(spipe[0], buf, sizeof(buf));
#else
  recv(spipe[0], buf, sizeof(buf), 0);
#endif

  LOG_DEBUG("thread_signal_read <- %d OK\n", spipe[0]);
}

// stop thread, blocks until stopped
void thread_stop(MediaScanThread *t) {
#ifndef WIN32
  if (t->tid) {
    LOG_DEBUG("Signalling thread %x to stop\n", t->tid);

    // Signal thread to stop with a dummy byte
    // XXX Worker thread needs to check for this
    thread_signal(t->reqpipe);

    pthread_join(t->tid, NULL);
    t->tid = 0;
    LOG_DEBUG("Thread stopped\n");

    // Close pipes
    close(t->respipe[0]);
    close(t->respipe[1]);
    close(t->reqpipe[0]);
    close(t->reqpipe[1]);
  }
#else
  if (t != NULL) {
    SetEvent(t->ghSignalEvent);

    // Wait until all threads have terminated.
    WaitForSingleObject(t->hThread, INFINITE);

    CloseHandle(t->hThread);
    CloseHandle(t->ghSignalEvent);
  }


#endif
}

void thread_destroy(MediaScanThread *t) {
  thread_stop(t);

#ifdef WIN32
  DeleteCriticalSection(&t->CriticalSection);
#endif

  // Cleanup event queue
  {
    struct equeue *eq = (struct equeue *)t->event_queue;
    while (!TAILQ_EMPTY(eq)) {
      struct equeue_entry *entry = eq->tqh_first;
      TAILQ_REMOVE(eq, entry, entries);
      LOG_MEM("destroy equeue_entry @ %p\n", entry);
      free(entry);
    }
    LOG_MEM("destroy equeue @ %p\n", eq);
    free(eq);
  }

  LOG_MEM("destroy MediaScanThread @ %p\n", t);
  free(t);
}
