///-------------------------------------------------------------------------------------------------
// file:  libmediascan\include\libmediascan.h
//
// summary: External header for the LibMediaScan library.
///-------------------------------------------------------------------------------------------------

#ifndef _LIBMEDIASCAN_H
#define _LIBMEDIASCAN_H

#include <db.h>

#include <unistd.h>
#include <stdio.h>
#include <stdint.h>

#ifdef WIN32
#include <Windows.h>
#include <wchar.h>
#endif

#include <pthread.h>


#define MAX_PATHS        64
#define MAX_IGNORE_EXTS  128
#define MAX_IGNORE_SDIRS 128
#define MAX_THUMBS       8
#define MAX_TAG_ITEMS    256

enum media_error {
  MS_ERROR_TYPE_UNKNOWN = -1,
  MS_ERROR_TYPE_INVALID_PARAMS = -2,
  MS_ERROR_FILE = -3,
  MS_ERROR_READ = -4,
  MS_ERROR_CACHE = -5
};

enum media_type {
  TYPE_UNKNOWN = 0,
  TYPE_VIDEO,
  TYPE_AUDIO,
  TYPE_IMAGE,
  TYPE_LNK
};

enum scan_flags {
  MS_USE_EXTENSION = 1,
  MS_FULL_SCAN = 1 << 1,
  MS_RESCAN = 1 << 2,
  MS_INCLUDE_DELETED = 1 << 3,
  MS_WATCH_CHANGES = 1 << 4,
  MS_CLEARDB = 1 << 5           /* DEBUG: Clear the BDB when ms_scan is called */
};

enum thumb_format {
  THUMB_AUTO = 1,               //< Use JPEG for square thumbnails, transparent PNG for non-square
  THUMB_JPEG,
  THUMB_PNG
};

enum exif_orientation {
  ORIENTATION_NORMAL = 1,
  ORIENTATION_MIRROR_HORIZ,
  ORIENTATION_180,
  ORIENTATION_MIRROR_VERT,
  ORIENTATION_MIRROR_HORIZ_270_CCW,
  ORIENTATION_90_CCW,
  ORIENTATION_MIRROR_HORIZ_90_CCW,
  ORIENTATION_270_CCW
};

enum event_type {
  EVENT_TYPE_RESULT = 1,
  EVENT_TYPE_PROGRESS,
  EVENT_TYPE_ERROR,
  EVENT_TYPE_FINISH
};

enum log_level {
  ERR = 1,
  WARN = 2,
  INFO = 3,
  DEBUG = 4,
  MEMORY = 9
};

enum tag_value_type {
  TYPE_UTF8 = 1,
  TYPE_BINARY,
  TYPE_INT32
};

struct _Thread {
  int respipe[2];               // pipe for worker thread to signal main thread
  void *event_queue;            // TAILQ for events
  int aborted;                  // flag set when thread should abort itself

  pthread_t tid;
  pthread_mutex_t mutex;
};
typedef struct _Thread MediaScanThread;

struct _TagItem {
  char *key;
  char *value;                  // XXX may not be able to store all tag items as strings
};
typedef struct _TagItem MediaScanTagItem;

struct _Tag {
  const char *type;             // ID3v2, EXIF, etc
  int nitems;
  struct _TagItem *items[MAX_TAG_ITEMS];  // array of individual tag items
};
typedef struct _Tag MediaScanTag;

struct _Audio {
  const char *codec;
  uint64_t audio_offset;
  uint64_t audio_size;
  int bitrate;
  int vbr;
  int samplerate;
  int channels;
};
typedef struct _Audio MediaScanAudio;

struct _Image {
  const char *path;             ///< Path to the file containing this image
  const char *codec;
  int width;
  int height;
  int channels;
  int has_alpha;
  int offset;                   // byte offset to start of image
  enum exif_orientation orientation;

  // private members
  void *_dbuf;                  // Buffer for compressed image data
  uint32_t *_pixbuf;            // Uncompressed image data used during resize
  int _pixbuf_size;             // Size of data in pixbuf
  int _pixbuf_is_copy;          // Flag if dst pixbuf is a pointer to src pixbuf
  void *_jpeg;                  // JPEG-specific internal data
  void *_png;                   // PNG-specific internal data
  void *_bmp;                   // BMP-specific internal data
  void *_gif;                   // GIF-specific internal data
#ifdef TIFF_SUPPORT
  void *_tiff;                  // TIFF-specific internal data
#endif
};
typedef struct _Image MediaScanImage;

struct _Video {
  const char *path;             ///< Path to the file containing this video
  const char *codec;
  int width;
  int height;
  double fps;

  struct _Audio **streams;

  // private members
  uint32_t *_pixbuf;            // Uncompressed frame image data used during resize
  int _pixbuf_size;             // Size of data in pixbuf
  void *_codecs;                // av_codecs_t containing AVStream, AVCodecContext for video/audio
  void *_avc;                   // AVCodec instance
};
typedef struct _Video MediaScanVideo;

struct _Error {
  enum media_error error_code;
  int averror;                  ///< Optional error code from ffmpeg
  char *path;
  char *error_string;
};
typedef struct _Error MediaScanError;

struct _Result {
  enum media_type type;
  char *path;
  int flags;
  MediaScanError *error;
  int deleted;                  ///< Set if scan flag MS_INCLUDE_DELETED was used and this result is for a deleted file.
  /// NOTE: Only the type and path data will be set for deleted files.
  int changed;                  ///< Set if scan flag MS_RESCAN was used and this result is for a changed file.

  const char *mime_type;
  const char *dlna_profile;
  uint64_t size;
  int mtime;
  int bitrate;                  ///< total bitrate
  int duration_ms;

  uint32_t hash;

  // All media types have thumbnails
  int nthumbnails;

  MediaScanAudio *audio;        ///< Audio-specific data, only present if type is TYPE_AUDIO or TYPE_VIDEO.
  MediaScanImage *image;        ///< Image-specific data, only present if type is TYPE_IMAGE.
  MediaScanVideo *video;        ///< Video-specific data, only present if type is TYPE_VIDEO.

  // private members
  void *_scan;                  // reference to scan that created this result
  void *_avf;                   // AVFormatContext instance
  FILE *_fp;                    // opened file if necessary
  void *_buf;                   // buffer if necessary
  struct _Image *_thumbs[MAX_THUMBS]; // generated thumbs
  struct _Tag *_tag;            // tag data
};
typedef struct _Result MediaScanResult;

struct _Progress {
  char *phase;                  ///< Discovering, Scanning, etc
  char *cur_item;               ///< most recently scanned item, NULL on last callback when done
  int interval;
  int total;
  int done;
  int eta;                      ///< eta in seconds
  int rate;                     ///< rate in items/second

  // private
  long _start_ts;
  long _last_update_ts;
};
typedef struct _Progress MediaScanProgress;

typedef struct _ThumbSpec {
  enum thumb_format format;
  int width;
  int height;
  int keep_aspect;
  uint32_t bgcolor;
  int jpeg_quality;

  // Internal data
  int width_padding;
  int width_inner;
  int height_padding;
  int height_inner;
} MediaScanThumbSpec;

struct _Scan {
  int npaths;
  char *paths[MAX_PATHS];
  int nignore_exts;
  char *ignore_exts[MAX_IGNORE_EXTS];
  int nignore_sdirs;
  char *ignore_sdirs[MAX_IGNORE_SDIRS];
  int nthumbspecs;
  MediaScanThumbSpec *thumbspecs[MAX_THUMBS];
  int async;
  int async_fds[2];
  char *cachedir;
  int flags;
  int watch_interval;

  MediaScanProgress *progress;
  MediaScanThread *thread;

  void (*on_result) (struct _Scan *, MediaScanResult *, void *);
  void (*on_error) (struct _Scan *, MediaScanError *, void *);
  void (*on_progress) (struct _Scan *, MediaScanProgress *, void *);
  void (*on_finish) (struct _Scan *, void *);
  void *userdata;

  DB *dbp;                      /* DB structure handle */

  // private
  void *_dirq;                  // simple queue of all directories found
  void *_dlna;                  // libdlna instance
  int _want_abort;              // set when scan should abort as soon as possible
};

typedef struct _Scan MediaScan;

typedef void (*ResultCallback) (MediaScan *, MediaScanResult *, void *);
typedef void (*ErrorCallback) (MediaScan *, MediaScanError *, void *);
typedef void (*ProgressCallback) (MediaScan *, MediaScanProgress *, void *);
typedef void (*FinishCallback) (MediaScan *, void *);

///< libmediascan's errno
extern int ms_errno;

// This failure will be set if...
enum {
  MSENO_DIRECTORYFAIL = 1000,   // ms_scan doesn't have a valid directory in its scan list
  MSENO_NORESULTCALLBACK = 1001,  // no result callback set
  MSENO_NULLSCANOBJ = 1002,     // Illegal scan oject
  MSENO_SCANERROR = 1003,       // ScanErrorObject thrown
  MSENO_MEMERROR = 1004,        // Out of memory error
  MSENO_NOERRORCALLBACK = 1005, // No error callback
  MSENO_THREADERROR = 1006,     // Theading error
  MSENO_DBERROR = 1007,         // Database error
  MSENO_ILLEGALPARAMETER = 1008 // Illegal function parameter
};

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
MediaScan *ms_create(void);

/**
 * Destroy the given MediaScan object. Do not call this from within a callback.
 * See ms_abort() to stop a running scan.
 */
void ms_destroy(MediaScan *s);

/**
 * Abort a scan in process. This function is safe to call from within a callback.
 */
void ms_abort(MediaScan *s);

/**
 * Add a path to be scanned. Up to 64 paths may be added before
 * beginning the scan.
 */
void ms_add_path(MediaScan *s, const char *path);


/**
 * Add a subdirectory name to be ignored. For example, if you add ".ite" then all subdirectories
 * named /.ite will be ignored by the scanner
 */
void ms_add_ignore_directory_suffix(MediaScan *s, const char *suffix);

/**
 * Add a file extension to ignore all files with this extension.
 * 3 special all-caps extensions may be provided:
 * AUDIO - ignore all audio-related extensions.
 * IMAGE - ignore all image-related extensions.
 * VIDEO - ignore all video-related extensions.
 */
void ms_add_ignore_extension(MediaScan *s, const char *extension);

/**
 * Specify a thumbnail to be created for all media containing an image, such as embedded images
 * in audio files, video frames, and normal images. Multiple thumbnails can be defined.
 * @param format One of THUMB_AUTO, THUMB_JPEG, or THUMB_PNG. Auto will use JPEG for square thumbnails
 * and transparent PNG for non-square images.
 * @param width If >0, the thumbnail width
 * @param height If >0, the thumbnail height. If only one of width or height are specified
 * the other dimension will be set to retain the original aspect ratio.
 * @param keep_aspect If both width and height are specified, setting this to 1 will
 * keep the aspect ratio of the source as well as center the image when resizing into
 * a different aspect ratio.
 * @param bgcolor If the image needs to be padded and is not transparent, specify a 24-bit bgcolor
 * such as 0xffffff (white) or 0x000000 (black).
 * @param quality For JPEG thumbnails, specify the desired quality. Defaults to 90 if set to 0.
 */
void ms_add_thumbnail_spec(MediaScan *s, enum thumb_format format, int width,
                           int height, int keep_aspect, uint32_t bgcolor, int quality);

/**
 * By default, scans are synchronous. This means the call to ms_scan will
 * not return until the scan is finished. To enable background asynchronous
 * scanning, pass a true value to this function.
 */
void ms_set_async(MediaScan *s, int enabled);

/**
 * Specify a directory to be used for cache files. If not specified the current directory will
 * be used, which is probably not what you want.
 */
void ms_set_cachedir(MediaScan *s, const char *path);

/**
 * Set one or more flags ORed together to alter the behavior of the scan. If ms_set_flags
 * is not called before ms_scan, a default set of flags is used. The default set is:
 * MS_USE_EXTENSION | MS_FULL_SCAN
 *
 * @param flags Available flags are:
 * MS_USE_EXTENSION - Use a file's extension to determine file format. If unset, the scanner
 *   will try to detect a file's type by looking at the actual data. This method is also slower.
 * MS_FULL_SCAN - Scan all files found that are not specified in the ignore list.
 * MS_RESCAN - Perform a fast rescan by only scanning files that are new, or have changed their
 *   size and/or modification timestamp since the last scan was run. If the database from a prior
 *   scan is not available (libmediascan.db), the scan is the same as a full scan. The result for a changed
 *   file will have r->changed set.
 * MS_INCLUDE_DELETED - It is often useful to know that a file has been deleted. With this flag,
 *   a file that was previously scanned but has since been deleted will be reported to the result_callback
 *   and the r->deleted value will be set. NOTE: Only r->type, r->path, and r->deleted are valid for deleted
 *   results. Also note that this cannot distinguish files that were simply renamed or moved.
 * MS_WATCH_CHANGES - With this flag, after the scan has completed the path(s) will be monitored for changes.
 *   For files located on a local drive under OSX, Linux, or Windows, OS-native change detection will be used.
 *   For files on other systems or on remote network shares, the library will manually look for changes at regular
 *   intervals. Use ms_set_watch_interval() to configure this interval. To stop watching for changes, call
 *   ms_clear_watch().
 */
void ms_set_flags(MediaScan *s, int flags);

/**
 * Set the interval the library will use to look for changes to files located on non-local filesystems
 * or on systems that don't support OS-specific change notification methods. If this is not called, the
 * default watch interval is 10 minutes.
 * @param interval Watch interval, in seconds.
 */
void ms_set_watch_interval(MediaScan *s, int interval_seconds);

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
 * Set a callback that will be called at regular intervals during the scan
 * with progress details. This callback is optional. See ms_set_progress_interval
 * to adjust the callback interval.
 */
void ms_set_progress_callback(MediaScan *s, ProgressCallback callback);

/**
 * Set progress callback interval in seconds. Progress callback will not be
 * called more often than this value. This interval defaults to 1 second.
 */
void ms_set_progress_interval(MediaScan *s, int seconds);

/**
 * Set a callback that will be called when the scanning has finished.
 * This callback is optional.
 */
void ms_set_finish_callback(MediaScan *s, FinishCallback callback);

/**
 * Set an optional user pointer to be passed to all callbacks.
 */
void ms_set_userdata(MediaScan *s, void *data);

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
 * Supply your own pair of connected file descriptors for use with thread communication.
 * This is a hack to work around some issues with the Perl binding. This must be called
 * before ms_scan().
 * @param s MediaScan instance.
 * @param respipe[2] A pipe/socketpair to be used for worker->main thread communication.
 */
void ms_set_async_pipe(MediaScan *s, int respipe[2]);

/**
 * This function should be called whenever the async file descriptor becomes
 * readable. It will trigger one or more callbacks.
 */
void ms_async_process(MediaScan *s);

/**
 * For debugging or logging purposes, dump the contents of the given MediaScanResult
 * to stdout.
 */
void ms_dump_result(MediaScanResult *r);

/**
 * Get thumbnail for a given result.
 * @param r MediaScanResult instance.
 * @param index 0-based index of the thumbnail to return. Check r->nthumbnails for the total number.
 * @return A MediaScanImage instance for the thumbnail.
 */
MediaScanImage *ms_result_get_thumbnail(MediaScanResult *r, int index);

/**
 * Get thumbnail data for a given result.
 * @param r MediaScanResult instance.
 * @param index 0-based index of the thumbnail to return. Check r->nthumbnails for the total number.
 * @param *length (OUT) Returns the length of the thumbnail data.
 * @return A pointer to the raw JPEG or PNG thumbnail data.
 */
const uint8_t *ms_result_get_thumbnail_data(MediaScanResult *r, int index, int *length);

/**
 * Return the total number of tags for a given result.
 * @param r MediaScanResult instance.
 * @return The total number of tags.
 */
int ms_result_get_tag_count(MediaScanResult *r);

/**
 * Get tag data for a given result.
 * @param r MediaScanResult instance.
 * @param index 0-based index of the tag to return. Check ms_result_get_tag_count for the total number.
 * @param key (OUT) Returns the key string.
 * @param value (OUT) Returns the value string.
 */
void ms_result_get_tag(MediaScanResult *r, int index, const char **key, const char **value);

///-------------------------------------------------------------------------------------------------
///  Watch a directory in the background.
///
/// @author Henry Bennett
/// @date 03/22/2011
///
/// @param path Path name of the folder to watch
/// @param callback Callback with the changes
///-------------------------------------------------------------------------------------------------

// XXX replaced by MS_WATCH_CHANGES
void ms_watch_directory(MediaScan *s, const char *path);

/**
 * If ms_scan was run with the flag MS_WATCH_CHANGES, this call will stop watching for changes.
 * To begin watching again, you must call ms_scan() again.
 */
void ms_clear_watch(MediaScan *s);

#endif // _LIBMEDIASCAN_H
