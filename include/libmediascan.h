#ifndef _LIBMEDIASCAN_H
#define _LIBMEDIASCAN_H

#include <unistd.h>
#include <stdio.h>

#define MAX_PATHS 128
#define MAX_IGNORE_EXTS 128

enum media_error {
  MS_ERROR_TYPE_UNKNOWN        = -1,
  MS_ERROR_TYPE_INVALID_PARAMS = -2,
  MS_ERROR_FILE                = -3,
  MS_ERROR_READ                = -4
};

enum media_type {
  TYPE_UNKNOWN = 0,
  TYPE_VIDEO,
  TYPE_AUDIO,
  TYPE_IMAGE
};

enum scan_flags {
  USE_EXTENSION = 1 //< Use a file's extension to determine file format (default)
};

enum log_level {
  ERROR  = 1,
  WARN   = 2,
  INFO   = 3,
  DEBUG  = 4,
  MEMORY = 9
};

struct _Tag {
  const char *type;
  // XXX key/value pairs
};

struct _Audio {
  const char *codec;
  off_t audio_offset;
  off_t audio_size;
  int bitrate;
  int vbr;
  int samplerate;
  int channels;
  
  struct _Image **images;
  struct _Tag **tags;
};
typedef struct _Audio MediaScanAudio;

struct _Image {
  int width;
  int height;
  int offset;       // byte offset to start of image
  const char *data; // image data, if needed
  
  struct _Image **thumbnails;
  struct _Tag **tags;
};
typedef struct _Image MediaScanImage;

struct _Video {
  const char *codec;
  int width;
  int height;
  
  struct _Audio **streams;
  struct _Image **thumbnails;
  struct _Tag **tags;
};
typedef struct _Video MediaScanVideo;

struct _Error {
  enum media_error error_code;
  int averror;                  ///< Optional error code from ffmpeg
  const char *path;
  const char *error_string;
};
typedef struct _Error MediaScanError;

struct _Result {
  enum media_type type;
  const char *path;
  enum scan_flags flags;
  MediaScanError *error;
  
  const char *mime_type;
  const char *dlna_profile;
  off_t size;
  int mtime;
  int bitrate; ///< total bitrate
  int duration_ms;
  
  union {
    MediaScanAudio *audio;
    MediaScanImage *image;
    MediaScanVideo *video;
  } type_data;
  
  // private members
  void *_avf; ///< AVFormatContext instance
  FILE *_fp;
};
typedef struct _Result MediaScanResult;

struct _Progress {
  const char *phase;    ///< Discovering, Scanning, etc
  const char *cur_item; ///< most recently scanned item
  int dir_total;
  int dir_done;
  int file_total;
  int file_done;
  int eta;    ///< eta in seconds
  float rate; ///< rate in items/second
  
  // private
  long _last_callback;
};
typedef struct _Progress MediaScanProgress;

struct _Scan {
  int npaths;
  char *paths[MAX_PATHS];
  int nignore_exts;
  char *ignore_exts[MAX_IGNORE_EXTS];
  int async;
  int async_fd;
  
  MediaScanProgress *progress;
  int progress_interval;
  
  void (*on_result)(struct _Scan *, MediaScanResult *);
  void (*on_error)(struct _Scan *, MediaScanError *);
  void (*on_progress)(struct _Scan *, MediaScanProgress *);
  
  // private
  void *_dirq; // simple queue of all directories found
};
typedef struct _Scan MediaScan;

typedef void (*ResultCallback)(MediaScan *, MediaScanResult *);
typedef void (*ErrorCallback)(MediaScan *, MediaScanError *);
typedef void (*ProgressCallback)(MediaScan *, MediaScanProgress *);

/**
 * Set the logging level.
 * 1 - Error
 * 2 - Warn
 * 3 - Info
 * 4 - Debug
 * ...
 * 9 - Memory alloc/free debugging
 */
void ms_set_log_level(enum log_level level);

/**
 * Allocate a new MediaScan object.
 */
MediaScan * ms_create(void);

/**
 * Destroy the given MediaScan object. If a scan is currently in progress
 * it will be aborted.
 */
void ms_destroy(MediaScan *s);

/**
 * Add a path to be scanned. Up to 128 paths may be added before
 * beginning the scan.
 */
void ms_add_path(MediaScan *s, const char *path);

/**
 * Add a file extension to ignore all files with this extension.
 * 3 special all-caps extensions may be provided:
 * AUDIO - ignore all audio-related extensions.
 * IMAGE - ignore all image-related extensions.
 * VIDEO - ignore all video-related extensions.
 */
void ms_add_ignore_extension(MediaScan *s, const char *extension);

/**
 * By default, scans are synchronous. This means the call to ms_scan will
 * not return until the scan is finished. To enable background asynchronous
 * scanning, pass a true value to this function.
 */
void ms_set_async(MediaScan *s, int enabled);

/**
 * Set a callback that will be called for every scanned file.
 * This callback is required or a scan cannot be started.
 */
void ms_set_result_callback(MediaScan *s, ResultCallback callback);
 
/**
 * Set a callback that will be called for all errors.
 * This callback is optional.
 */
void ms_set_error_callback(MediaScan *s, ErrorCallback callback);

/**
 * Set a callback that will be called during the scan with progress details.
 * This callback is optional.
 */
void ms_set_progress_callback(MediaScan *s, ProgressCallback callback);

/**
 * Set progress callback interval in seconds. Progress callback will not be
 * called more often than this value. This interval defaults to 1 second.
 */
void ms_set_progress_interval(MediaScan *s, int seconds);

/**
 * Begin a recursive scan of all paths previously provided to ms_add_path().
 * If async mode is enabled, this call will return immediately. You must
 * obtain the file descriptor using ms_async_fd and this must be checked using
 * an event loop or select(). When the fd becomes readable you must call
 * ms_async_process to trigger any necessary callbacks. 
 */
void ms_scan(MediaScan *s);

/**
 * Scan a single file. Everything that applies to ms_scan also applies to
 * this function. If you know the type of the file, set the type paramter
 * to one of TYPE_AUDIO, TYPE_VIDEO, or TYPE_IMAGE. Set it to TYPE_UNKNOWN
 * to have it determined automatically.
 */
void ms_scan_file(MediaScan *s, const char *full_path, enum media_type type);

/**
 * Return the file descriptor associated with an async scan. If an async scan
 * is not currently in progress, 0 will be returned.
 */
int ms_async_fd(MediaScan *s);

/**
 * This function should be called whenever the async file descriptor becomes
 * readable. It will trigger one or more callbacks.
 */
void ms_async_process(MediaScan *s);

#endif // _LIBMEDIASCAN_H
