#ifndef _THREAD_H
#define _THREAD_H

MediaScanThread *thread_create(void *(*func) (void *), thread_data_type *thread_data, int optional_fds[4]);
int thread_get_result_fd(MediaScanThread *t);
void thread_queue_event(MediaScanThread *t, enum event_type type, void *data);
enum event_type thread_get_next_event(MediaScanThread *t, void **data_out);
int thread_should_abort(MediaScanThread *t);
void thread_lock(MediaScanThread *t);
void thread_unlock(MediaScanThread *t);
void thread_signal(int spipe[2]);
void thread_signal_read(int spipe[2]);
void thread_stop(MediaScanThread *t);
void thread_destroy(MediaScanThread *t);
void WatchDirectory(void *thread_data);
#endif // _THREAD_H
