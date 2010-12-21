#ifndef _LIBMEDIASCAN_H
#define _LIBMEDIASCAN_H

#include <libavformat/avformat.h>

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

#define USE_EXTENSION 0x0001

struct _StreamData {
  enum media_type type;
  const char *codec_name;
  
  // Video/Audio
  int bitrate;
  
  // Video/Image
  int width;
  int height;

  // Audio
  int samplerate;
  int channels;
  int bit_depth;
  
  // Video
  double fps;
};
typedef struct _StreamData * StreamData;

struct _ScanData {
  const char *path;
  int flags;
  enum media_type type;
  enum scan_errors error;
  
  const char *type_name;
  int bitrate;
  int duration_ms;
  
  int nstreams;
  struct _StreamData *streams;
  
  AVMetadata *metadata;
  
  // private members
  AVFormatContext *_avf;
};
typedef struct _ScanData * ScanData;

typedef int (*ScanDataCallback)(ScanData);

/**
 * Scan a single media file.
 * @param path Full path to the file to be scanned.
 * @param flags Optional flags to control how the file will be scanned.
 *   XXX document flags
 * return An instance of ScanData. 
 * You should check that s->error is 0 before using anything within ScanData.
 * Caller is responsible for calling mediascan_free_ScanData for the returned value.
 * The scanned file will remain opened until the call to free.
 */
ScanData mediascan_scan_file(const char *path, int flags);

/*
 * Free the given instance of ScanData.
 * @param s An instance of ScanData.
 */
void mediascan_free_ScanData(ScanData s);

/*
 * Scan a directory tree recursively.
 * @param dir Full path to the directory to be scanned.
 * @param flags Optional ORed flags:
 *   USE_EXTENSION - Force file type from the file extension, instead of guessing. Results in faster scanning.
 * @param callback This callback is passed a ScanData for every valid file scanned.
 */
void mediascan_scan_tree(const char *dir, int flags, ScanDataCallback callback);

#endif // _LIBMEDIASCAN_H
