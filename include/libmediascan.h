#ifndef _LIBMEDIASCAN_H
#define _LIBMEDIASCAN_H

#include <unistd.h>
#include <stdio.h>

#define MAX_PATHS 128
#define MAX_IGNORE_EXTS 128

enum scan_errors {
  SCAN_FILE_OK = 0,
  SCAN_FILE_OPEN,
  SCAN_FILE_READ_INFO
};

enum media_type {
  TYPE_UNKNOWN = 0,
  TYPE_VIDEO,
  TYPE_AUDIO,
  TYPE_IMAGE
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

struct _Image {
  int width;
  int height;
  int offset;       // byte offset to start of image
  const char *data; // image data, if needed
  
  struct _Image **thumbnails;
  struct _Tag **tags;
};

struct _Video {
  const char *codec;
  int width;
  int height;
  
  struct _Audio **streams;
  struct _Image **thumbnails;
  struct _Tag **tags;
};

struct _Result {
  enum media_type type;
  const char *path;
  const char *mime_type;
  const char *dlna_profile;
  off_t size;
  int mtine;
  int bitrate; // total bitrate
  int duration_ms;
  
  union {
    struct _Audio *audio;
    struct _Image *image;
    struct _Video *video;
  } type_data;
  
  // private members
  void *_avf; // AVFormatContext instance
  FILE *_fp;
};
typedef struct _Result MediaScanResult;

struct _Error {
  const char *path;
  const char *error_string;
};
typedef struct _Error MediaScanError;

struct _Progress {
  int total;  // total number of items
  int done;   // number of items completed
  int eta;    // eta in seconds
  float rate; // rate in items/second
};
typedef struct _Progress MediaScanProgress;

struct _Scan {
  int npaths;
  char *paths[MAX_PATHS];
  int nignore_exts;
  char *ignore_exts[MAX_IGNORE_EXTS];
  int async;
  void (*on_result)(struct _Scan *, struct _Result *);
  void (*on_error)(struct _Scan *, struct _Error *);
  void (*on_progress)(struct _Scan *, struct _Progress *);
};
typedef struct _Scan MediaScan;

typedef void (*ResultCallback)(MediaScan *, MediaScanResult *);
typedef void (*ErrorCallback)(MediaScan *, MediaScanError *);
typedef void (*ProgressCallback)(MediaScan *, MediaScanProgress *);

/**
 * Set the logging level.
 */
void ms_set_log_level(int level);

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
 * Begin a recursive scan of all paths previously provided to ms_add_path().
 */
void ms_scan(MediaScan *s);

#endif // _LIBMEDIASCAN_H
