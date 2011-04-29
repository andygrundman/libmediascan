
// Cross-platform thread abstractions

#include <errno.h>
#include <libmediascan.h>
#include <stdlib.h>
#include <string.h>

#ifdef WIN32
#include <io.h>
#include <signal.h>
#endif

#include "mediascan.h"
#include "common.h"
#include "error.h"
#include "thread.h"
#include "queue.h"

struct equeue_entry {
  enum event_type type;
  void *data;
    TAILQ_ENTRY(equeue_entry) entries;
};
TAILQ_HEAD(equeue, equeue_entry);

/*
#define USE_SOCKETS_AS_HANDLES
#ifdef USE_SOCKETS_AS_HANDLES
# define S_TO_HANDLE(x) ((HANDLE)_get_osfhandle (x))
#else
# define S_TO_HANDLE(x) ((HANDLE)x)
#endif
*/

#ifdef _WIN32
/* taken almost verbatim from libev's ev_win32.c */
/* oh, the humanity! */
static int s_pipe(int filedes[2]) {
//  dTHX;

  struct sockaddr_in addr = { 0 };
  int addr_size = sizeof(addr);
  struct sockaddr_in adr2;
  int adr2_size = sizeof(adr2);
  SOCKET listener;
  SOCKET sock[2] = { -1, -1 };

  if ((listener = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET)
    return -1;

  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
  addr.sin_port = 0;

  if (bind(listener, (struct sockaddr *)&addr, addr_size))
    goto fail;

  if (getsockname(listener, (struct sockaddr *)&addr, &addr_size))
    goto fail;

  if (listen(listener, 1))
    goto fail;

  if ((sock[0] = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET)
    goto fail;

  if (connect(sock[0], (struct sockaddr *)&addr, addr_size))
    goto fail;

  if ((sock[1] = accept(listener, 0, 0)) < 0)
    goto fail;

  /* windows vista returns fantasy port numbers for getpeername.
   * example for two interconnected tcp sockets:
   *
   * (Socket::unpack_sockaddr_in getsockname $sock0)[0] == 53364
   * (Socket::unpack_sockaddr_in getpeername $sock0)[0] == 53363
   * (Socket::unpack_sockaddr_in getsockname $sock1)[0] == 53363
   * (Socket::unpack_sockaddr_in getpeername $sock1)[0] == 53365
   *
   * wow! tridirectional sockets!
   *
   * this way of checking ports seems to work:
   */
  if (getpeername(sock[0], (struct sockaddr *)&addr, &addr_size))
    goto fail;

  if (getsockname(sock[1], (struct sockaddr *)&adr2, &adr2_size))
    goto fail;

  errno = WSAEINVAL;
  if (addr_size != adr2_size || addr.sin_addr.s_addr != adr2.sin_addr.s_addr  /* just to be sure, I mean, it's windows */
      || addr.sin_port != adr2.sin_port)
    goto fail;

  closesocket(listener);

  /* when select isn't winsocket, we also expect socket, connect, accept etc.
   * to work on fds */
  filedes[0] = sock[0];
  filedes[1] = sock[1];

  return 0;

fail:
  closesocket(listener);

  if (sock[0] != INVALID_SOCKET)
    closesocket(sock[0]);
  if (sock[1] != INVALID_SOCKET)
    closesocket(sock[1]);

  return -1;
}

/*
#define s_socketpair(domain,type,protocol,filedes) s_pipe (filedes)

static int
s_fd_blocking (int fd, int blocking)
{
  u_long nonblocking = !blocking;

  return ioctlsocket ((SOCKET)S_TO_HANDLE (fd), FIONBIO, &nonblocking);
}

#define s_fd_prepare(fd) s_fd_blocking (fd, 0)
*/

#endif



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

#ifdef WIN32
  // Setup pipes for communication with main thread
  if (s_pipe(t->respipe)) {
    LOG_ERROR("Unable to initialize thread result pipe\n");
    goto fail;
  }

  if (s_pipe(t->reqpipe)) {
    LOG_ERROR("Unable to initialize thread request pipe\n");
    goto fail;
  }
#else

  // Setup pipes for communication with main thread
  if (pipe(t->respipe)) {
    LOG_ERROR("Unable to initialize thread result pipe\n");
    goto fail;
  }

  if (pipe(t->reqpipe)) {
    LOG_ERROR("Unable to initialize thread request pipe\n");
    goto fail;
  }
#endif

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
  pthread_mutex_lock(&t->mutex);
}

void thread_unlock(MediaScanThread *t) {
  pthread_mutex_unlock(&t->mutex);
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
  send(spipe[1], (LPCVOID)&dummy, 1, 0);
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
#else
  if (t->tid.p) { // XXX needed?
#endif
    LOG_DEBUG("Signalling thread %x to stop\n", t->tid);

    // Signal thread to stop with a dummy byte
    // XXX Worker thread needs to check for this
    thread_signal(t->reqpipe);

    pthread_join(t->tid, NULL);
#ifndef WIN32
    t->tid = 0;
#else
    t->tid.p = 0; // XXX needed?
#endif
    LOG_DEBUG("Thread stopped\n");
    // Close pipes

#ifdef WIN32
    closesocket(t->respipe[0]);
    closesocket(t->respipe[1]);
    closesocket(t->reqpipe[0]);
    closesocket(t->reqpipe[1]);
#else
    close(t->respipe[0]);
    close(t->respipe[1]);
    close(t->reqpipe[0]);
    close(t->reqpipe[1]);
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
      LOG_MEM("destroy equeue_entry @ %p\n", entry);
      free(entry);
    }
    LOG_MEM("destroy equeue @ %p\n", eq);
    free(eq);
  }

  LOG_MEM("destroy MediaScanThread @ %p\n", t);
  free(t);
}
