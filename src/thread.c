
// Cross-platform thread abstractions

#ifdef WIN32
# include <Winsock2.h>
# include <io.h>
# include <signal.h>
# include <fcntl.h>
#endif

#include <errno.h>
#include <libmediascan.h>
#include <stdlib.h>
#include <string.h>

#include "mediascan.h"
#include "common.h"
#include "result.h"
#include "progress.h"
#include "error.h"
#include "thread.h"
#include "queue.h"

#ifdef _MSC_VER
#pragma warning( disable: 4127 )
#endif

struct equeue_entry {
  enum event_type type;
  void *data;
    TAILQ_ENTRY(equeue_entry) entries;
};
TAILQ_HEAD(equeue, equeue_entry);

#ifdef WIN32
/* socketpair.c
 * Copyright 2007 by Nathan C. Myers <ncm@cantrip.org>; some rights reserved.
 * This code is Free Software.  It may be copied freely, in original or 
 * modified form, subject only to the restrictions that (1) the author is
 * relieved from all responsibilities for any use for any purpose, and (2)
 * this copyright notice must be retained, unchanged, in its entirety.  If
 * for any reason the author might be held responsible for any consequences
 * of copying or use, license is withheld.  
 */
static int win32_socketpair(int socks[2]) {
  union {
    struct sockaddr_in inaddr;
    struct sockaddr addr;
  } a;
  SOCKET listener;
  int e;
  int addrlen = sizeof(a.inaddr);
  DWORD flags = 0;
  int reuse = 1;

  if (socks == 0) {
    WSASetLastError(WSAEINVAL);
    return SOCKET_ERROR;
  }

  listener = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (listener == INVALID_SOCKET)
    return SOCKET_ERROR;

  memset(&a, 0, sizeof(a));
  a.inaddr.sin_family = AF_INET;
  a.inaddr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
  a.inaddr.sin_port = 0;

  socks[0] = socks[1] = INVALID_SOCKET;
  do {
    if (setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, (char *)&reuse, sizeof(reuse)) == -1)
      break;
    if (bind(listener, &a.addr, sizeof(a.inaddr)) == SOCKET_ERROR)
      break;
    if (getsockname(listener, &a.addr, &addrlen) == SOCKET_ERROR)
      break;
    if (listen(listener, 1) == SOCKET_ERROR)
      break;
    socks[0] = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, flags);
    if (socks[0] == INVALID_SOCKET)
      break;
    if (connect(socks[0], &a.addr, sizeof(a.inaddr)) == SOCKET_ERROR)
      break;
    socks[1] = accept(listener, NULL, NULL);
    if (socks[1] == INVALID_SOCKET)
      break;

    socks[0] = _open_osfhandle(socks[0], O_RDWR | O_BINARY);
    socks[1] = _open_osfhandle(socks[1], O_RDWR | O_BINARY);

    closesocket(listener);
    return 0;

  } while (0);

  e = WSAGetLastError();
  closesocket(listener);
  closesocket(socks[0]);
  closesocket(socks[1]);
  WSASetLastError(e);
  return SOCKET_ERROR;
}
#endif

MediaScanThread *thread_create(void *(*func) (void *), thread_data_type *thread_data, int optional_fds[4]) {
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

  // Setup pipes for communication with main thread
  // The FDs can be passed in if necessary (Win32+Perl), otherwise a pipe is created
  if (optional_fds[0] > 0) {
    t->respipe[0] = optional_fds[0];
    t->respipe[1] = optional_fds[1];

    LOG_DEBUG("Using supplied pipe: %d/%d\n", t->respipe[0], t->respipe[1]);
  }
  else {
#ifdef WIN32
    if (win32_socketpair(t->respipe)) {
#else
    if (pipe(t->respipe)) {
#endif
      LOG_ERROR("Unable to initialize thread result pipe\n");
      goto fail;
    }
  }

  if (pthread_mutex_init(&t->mutex, NULL) != 0) {
    LOG_ERROR("Unable to initialize thread mutex\n");
    goto fail;
  }

  // Launch thread
  err = pthread_create(&t->tid, NULL, func, (void *)thread_data);
  if (err != 0) {
    LOG_ERROR("Unable to create thread (%s)\n", strerror(err));
    goto fail;
  }

  LOG_DEBUG("Thread %x started\n", t->tid);
  goto out;

fail:
  t = NULL;

out:
  return t;
}

// Return the file descriptor that should be watched for
// by the main thread for notifications that events are waiting
int thread_get_result_fd(MediaScanThread *t) {
  return t->respipe[0];
}

// Queue a new event
void thread_queue_event(MediaScanThread *t, enum event_type type, void *data) {
  struct equeue_entry *entry = malloc(sizeof(struct equeue_entry));
  LOG_DEBUG("new equeue_entry @ %p (type %d, data @ %p)\n", entry, type, data);

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
  pthread_mutex_lock(&t->mutex);
}

void thread_unlock(MediaScanThread *t) {
  pthread_mutex_unlock(&t->mutex);
}

void thread_signal(int spipe[2]) {
  static char counter[8];
#ifdef WIN32
  DWORD dummy;

  LOG_DEBUG("thread_signal -> %d\n", spipe[1]);
  send((SOCKET)spipe[1], (LPCVOID)&dummy, 1, 0);
#else
  LOG_DEBUG("thread_signal -> %d\n", spipe[1]);
  write(spipe[1], &counter, 1);
#endif
}

void thread_signal_read(int spipe[2]) {
  char buf[9];

  LOG_DEBUG("thread_signal_read <- %d waiting...\n", spipe[0]);

#ifdef WIN32
  recv((SOCKET)spipe[0], buf, sizeof(buf), 0);
#else
  read(spipe[0], buf, sizeof(buf));
#endif

  LOG_DEBUG("thread_signal_read <- %d OK\n", spipe[0]);
}

// stop thread, blocks until stopped
void thread_stop(MediaScanThread *t) {
#ifndef WIN32
  if (t->tid) {
#else
  if (t->tid.p) {               // XXX needed?
#endif

    LOG_DEBUG("Waiting for thread %x to stop...\n", t->tid);
    pthread_join(t->tid, NULL);

#ifndef WIN32
    t->tid = 0;
#else
    t->tid.p = 0;               // XXX needed?
#endif
    LOG_DEBUG("Thread stopped\n");
    // Close pipes

#ifdef WIN32
    closesocket(t->respipe[0]);
    closesocket(t->respipe[1]);
#else
    close(t->respipe[0]);
    close(t->respipe[1]);
#endif
  }
}

void thread_destroy(MediaScanThread *t) {
  thread_stop(t);

  // Cleanup event queue
  {
    struct equeue *eq = (struct equeue *)t->event_queue;
    while (!TAILQ_EMPTY(eq)) {
      struct equeue_entry *entry = eq->tqh_first;
      TAILQ_REMOVE(eq, entry, entries);

      // Also need to free the internal objects waiting in the queue
      LOG_DEBUG("Cleaning up thread event, type %d @ %p\n", entry->type, entry->data);
      switch (entry->type) {
        case EVENT_TYPE_RESULT:
          result_destroy((MediaScanResult *)entry->data);
          break;

        case EVENT_TYPE_PROGRESS:
          progress_destroy((MediaScanProgress *)entry->data); // freeing a copy of progress
          break;

        case EVENT_TYPE_ERROR:
          error_destroy((MediaScanError *)entry->data);
          break;

        case EVENT_TYPE_FINISH:
        default:
          break;
      }

      LOG_MEM("destroy equeue_entry @ %p\n", entry);
      free(entry);
    }

    LOG_MEM("destroy equeue @ %p\n", eq);
    free(eq);
  }

  pthread_mutex_destroy(&t->mutex);

  LOG_MEM("destroy MediaScanThread @ %p\n", t);
  free(t);
}
