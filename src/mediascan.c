///-------------------------------------------------------------------------------------------------
// file:	libmediascan\src\mediascan.c
//
// summary:	mediascan class
///-------------------------------------------------------------------------------------------------

#include <ctype.h>
#include <dirent.h>
#include <stdlib.h>

#ifndef WIN32
#include <sys/time.h>
#else
#include <time.h>
#include <Winsock2.h>
#include <direct.h>
#endif

// Global debug flag, used by LOG_LEVEL macro
int Debug = 0;

#include <libmediascan.h>
#include "common.h"

#include "queue.h"

#include "progress.h"
#include "result.h"

#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>

#include "mediascan.h"

static int Initialized = 0;
static long PathMax = 0;

// File/dir queue struct definitions
struct fileq_entry {
  char *file;
  SIMPLEQ_ENTRY(fileq_entry) entries;
};
SIMPLEQ_HEAD(fileq, fileq_entry);

struct dirq_entry {
  char *dir;
  struct fileq *files;
  SIMPLEQ_ENTRY(dirq_entry) entries;
};
SIMPLEQ_HEAD(dirq, dirq_entry);

/*
 Video support
 -------------
 We are only supporting a subset of FFmpeg's supported formats:
 
 Containers:   MPEG-4, ASF, AVI, Matroska, VOB, FLV, WebM
 Video codecs: h.264, MPEG-1, MPEG-2, MPEG-4, Microsoft MPEG-4, WMV, VP6F (FLV), VP8
 Audio codecs: AAC, AC3, DTS, MP3, MP2, Vorbis, WMA
 Subtitles:    All
 
 Audio support
 -------------
 TODO
 
 Image support
 -------------
 TODO
*/

// File extensions to look for (leading/trailing comma are required)
static const char *AudioExts = ",aif,aiff,wav,";
static const char *VideoExts = ",asf,avi,divx,flv,m2t,m4v,mkv,mov,mpg,mpeg,mp4,m2p,m2t,mts,m2ts,ts,vob,webm,wmv,xvid,3gp,3g2,3gp2,3gpp,";
static const char *ImageExts = ",jpg,png,gif,bmp,jpeg,";

#ifdef WIN32
#define REGISTER_DECODER(X,x) { \
          extern AVCodec ff_##x##_decoder; \
          avcodec_register(&ff_##x##_decoder); }
#else
#define REGISTER_DECODER(X,x) { \
          extern AVCodec x##_decoder; \
          avcodec_register(&x##_decoder); }
#endif

#ifdef WIN32
#define REGISTER_PARSER(X,x) { \
          extern AVCodecParser ff_##x##_parser; \
          av_register_codec_parser(&ff_##x##_parser); }
#else
#define REGISTER_PARSER(X,x) { \
          extern AVCodecParser x##_parser; \
          av_register_codec_parser(&x##_parser); }
#endif

static void

///-------------------------------------------------------------------------------------------------
/// <summary>	Register codecs to be used with ffmpeg </summary>
///
/// <remarks>	 </remarks>
///-------------------------------------------------------------------------------------------------

register_codecs(void)
{
  // Video codecs
  REGISTER_DECODER (H264, h264);
  REGISTER_DECODER (MPEG1VIDEO, mpeg1video);
  REGISTER_DECODER (MPEG2VIDEO, mpeg2video);
  REGISTER_DECODER (MPEG4, mpeg4);
  REGISTER_DECODER (MSMPEG4V1, msmpeg4v1);
  REGISTER_DECODER (MSMPEG4V2, msmpeg4v2);
  REGISTER_DECODER (MSMPEG4V3, msmpeg4v3);
  REGISTER_DECODER (VP6F, vp6f);
  REGISTER_DECODER (VP8, vp8);
  REGISTER_DECODER (WMV1, wmv1);
  REGISTER_DECODER (WMV2, wmv2);
  REGISTER_DECODER (WMV3, wmv3);
  
  // Audio codecs, needed to get details of audio tracks in videos
  REGISTER_DECODER (AAC, aac);
  REGISTER_DECODER (AC3, ac3);
  REGISTER_DECODER (DCA, dca); // DTS
  REGISTER_DECODER (MP3, mp3);
  REGISTER_DECODER (MP2, mp2);
  REGISTER_DECODER (VORBIS, vorbis);
  REGISTER_DECODER (WMAPRO, wmapro);
  REGISTER_DECODER (WMAV1, wmav1);
  REGISTER_DECODER (WMAV2, wmav2);
  REGISTER_DECODER (WMAVOICE, wmavoice);
  
  // Subtitles
  REGISTER_DECODER (ASS, ass);
  REGISTER_DECODER (DVBSUB, dvbsub);
  REGISTER_DECODER (DVDSUB, dvdsub);
  REGISTER_DECODER (PGSSUB, pgssub);
  REGISTER_DECODER (XSUB, xsub);
  
  // Parsers 
  REGISTER_PARSER (AAC, aac);
  REGISTER_PARSER (AC3, ac3);
  REGISTER_PARSER (DCA, dca); // DTS
  REGISTER_PARSER (H264, h264);
  REGISTER_PARSER (MPEG4VIDEO, mpeg4video);
  REGISTER_PARSER (MPEGAUDIO, mpegaudio);
  REGISTER_PARSER (MPEGVIDEO, mpegvideo);
}
#ifdef WIN32

#define REGISTER_DEMUXER(X,x) { \
    extern AVInputFormat ff_##x##_demuxer; \
    av_register_input_format(&ff_##x##_demuxer); }
#define REGISTER_PROTOCOL(X,x) { \
    extern URLProtocol ff_##x##_protocol; \
    av_register_protocol2(&ff_##x##_protocol, sizeof(ff_##x##_protocol)); }


#else
#define REGISTER_DEMUXER(X,x) { \
    extern AVInputFormat x##_demuxer; \
    av_register_input_format(&x##_demuxer); }
#define REGISTER_PROTOCOL(X,x) { \
    extern URLProtocol x##_protocol; \
    av_register_protocol2(&x##_protocol, sizeof(x##_protocol)); }
#endif

///-------------------------------------------------------------------------------------------------
/// <summary>	Registers the formats for FFmpeg </summary>
///
/// <remarks>	 </remarks>
///-------------------------------------------------------------------------------------------------

static void register_formats(void)
{
  // demuxers
  REGISTER_DEMUXER (ASF, asf);
  REGISTER_DEMUXER (AVI, avi);
  REGISTER_DEMUXER (FLV, flv);
  REGISTER_DEMUXER (H264, h264);
  REGISTER_DEMUXER (MATROSKA, matroska);
  REGISTER_DEMUXER (MOV, mov);
  REGISTER_DEMUXER (MPEGPS, mpegps);             // VOB files
  REGISTER_DEMUXER (MPEGVIDEO, mpegvideo);
  
  // protocols
  REGISTER_PROTOCOL (FILE, file);
}

///-------------------------------------------------------------------------------------------------
/// <summary>	Initialises ffmpeg </summary>
///
/// <remarks>	 </remarks>
///-------------------------------------------------------------------------------------------------

static void _init(void)
{
  if (Initialized)
    return;
  
  register_codecs();
  register_formats();
  //av_register_all();

#ifndef WIN32
  PathMax = pathconf(".", _PC_PATH_MAX); // 1024
#endif

  Initialized = 1;
}

///-------------------------------------------------------------------------------------------------
/// <summary>	 Set the logging level. </summary>
///
/// <remarks>	 </remarks>
///
/// <param name="level">	The level. </param>
///-------------------------------------------------------------------------------------------------

void ms_set_log_level(int level)
{
  Debug = level;
}

///-------------------------------------------------------------------------------------------------
/// <summary>	Allocate a new MediaScan object. </summary>
///
/// <remarks>	 </remarks>
///
/// <returns>	null if it fails, else. </returns>
///-------------------------------------------------------------------------------------------------

MediaScan *ms_create(void)
{
  MediaScan *s;

  _init();
  
  s = (MediaScan *)calloc(sizeof(MediaScan), 1);
  if (s == NULL) {
    LOG_ERROR("Out of memory for new MediaScan object\n");
    return NULL;
  }
  
  s->npaths = 0;
  s->paths[0] = NULL;
  s->nignore_exts = 0;
  s->ignore_exts[0] = NULL;
  s->async = 0;
  s->async_fd = 0;
  
  s->progress = progress_create();
  s->progress_interval = 1;
  
  s->on_result = NULL;
  s->on_error = NULL;
  s->on_progress = NULL;
  
  // List of all dirs found
  s->_dirq = malloc(sizeof(struct dirq));
  SIMPLEQ_INIT((struct dirq *)s->_dirq);
  
  return s;
}

///-------------------------------------------------------------------------------------------------
/// <summary>
/// 	Destroy the given MediaScan object. If a scan is currently in progress it will be aborted.
/// </summary>
///
/// <remarks>	 </remarks>
///
/// <param name="s">	[in,out] If non-null, the. </param>
///-------------------------------------------------------------------------------------------------

void ms_destroy(MediaScan *s)
{
  int i = 0;
  struct dirq *head = (struct dirq *)s->_dirq;
  struct dirq_entry *entry = NULL;
  struct fileq *file_head = NULL;
  struct fileq_entry *file_entry = NULL;

  for (i = 0; i < s->npaths; i++) {
    free( s->paths[i] );
  }
  
  for (i = 0; i < s->nignore_exts; i++) {
    free( s->ignore_exts[i] );
  }
  
  // Free everything in our list of dirs/files
  while (!SIMPLEQ_EMPTY(head)) {
    entry = SIMPLEQ_FIRST(head);
    
    if (entry->files != NULL) {
      file_head = entry->files;
      while (!SIMPLEQ_EMPTY(file_head)) {
        file_entry = SIMPLEQ_FIRST(file_head);
        free(file_entry->file);
        SIMPLEQ_REMOVE_HEAD(file_head, entries);
        free(file_entry);
      }
      free(entry->files);
    }
    
    SIMPLEQ_REMOVE_HEAD(head, entries);
    free(entry->dir);
    free(entry);
  }
  
  free(s->_dirq);
  
  progress_destroy(s->progress);
  
  free(s);
}

///-------------------------------------------------------------------------------------------------
/// <summary>
/// 	Add a path to be scanned. Up to 128 paths may be added before beginning the scan.
/// </summary>
///
/// <remarks>	 </remarks>
///
/// <param name="s">   	[in,out] If non-null, the. </param>
/// <param name="path">	Full pathname of the file. </param>
///-------------------------------------------------------------------------------------------------

void ms_add_path(MediaScan *s, const char *path)
{
  int len = 0;
  char *tmp = NULL;

  if (s->npaths == MAX_PATHS) {
    LOG_ERROR("Path limit reached (%d)\n", MAX_PATHS);
    return;
  }
  
  len = strlen(path) + 1;
  tmp = malloc(len);
  if (tmp == NULL) {
    LOG_ERROR("Out of memory for adding path\n");
    return;
  }
  
  strncpy(tmp, path, len);
  
  s->paths[ s->npaths++ ] = tmp;
}

///-------------------------------------------------------------------------------------------------
/// <summary>	Add a file extension to ignore all files with this extension. </summary>
///
/// <remarks>	 </remarks>
///
/// <param name="s">			[in,out] If non-null, the. </param>
/// <param name="extension">	3 special all-caps extensions may be provided:
///								AUDIO - ignore all audio-related extensions.
///								IMAGE - ignore all image-related extensions.
///								VIDEO - ignore all video-related extensions. </param>
///-------------------------------------------------------------------------------------------------

void ms_add_ignore_extension(MediaScan *s, const char *extension)
{
  int len = 0;
  char *tmp = NULL;

  if (s->nignore_exts == MAX_IGNORE_EXTS) {
    LOG_ERROR("Ignore extension limit reached (%d)\n", MAX_IGNORE_EXTS);
    return;
  }
  
  len = strlen(extension) + 1;
  tmp = malloc(len);
  if (tmp == NULL) {
    LOG_ERROR("Out of memory for ignore extension\n");
    return;
  }
  
  strncpy(tmp, extension, len);
  
  s->ignore_exts[ s->nignore_exts++ ] = tmp;
}

///-------------------------------------------------------------------------------------------------
/// <summary>
/// 	By default, scans are synchronous. This means the call to ms_scan will not return until
/// 	the scan is finished. To enable background asynchronous scanning, pass a true value to
/// 	this function.
/// </summary>
///
/// <remarks>	 </remarks>
///
/// <param name="s">	  	[in,out] If non-null, the. </param>
/// <param name="enabled">	The enabled. </param>
///-------------------------------------------------------------------------------------------------

void ms_set_async(MediaScan *s, int enabled)
{
  s->async = enabled ? 1 : 0;
}

///-------------------------------------------------------------------------------------------------
/// <summary>
/// 	Set a callback that will be called for every scanned file. This callback is required or a
/// 	scan cannot be started.
/// </summary>
///
/// <remarks>	 </remarks>
///
/// <param name="s">	   	[in,out] If non-null, the. </param>
/// <param name="callback">	The callback. </param>
///-------------------------------------------------------------------------------------------------

void ms_set_result_callback(MediaScan *s, ResultCallback callback)
{
  s->on_result = callback;
}

///-------------------------------------------------------------------------------------------------
/// <summary>
/// 	Set a callback that will be called for all errors. This callback is optional.
/// </summary>
///
/// <remarks>	 </remarks>
///
/// <param name="s">	   	[in,out] If non-null, the. </param>
/// <param name="callback">	The callback. </param>
///-------------------------------------------------------------------------------------------------

void ms_set_error_callback(MediaScan *s, ErrorCallback callback)
{
  s->on_error = callback;
}

///-------------------------------------------------------------------------------------------------
/// <summary>
/// 	Set a callback that will be called during the scan with progress details. This callback
/// 	is optional.
/// </summary>
///
/// <remarks>	 </remarks>
///
/// <param name="s">	   	[in,out] If non-null, the. </param>
/// <param name="callback">	The callback. </param>
///-------------------------------------------------------------------------------------------------

void ms_set_progress_callback(MediaScan *s, ProgressCallback callback)
{
  s->on_progress = callback;
}

///-------------------------------------------------------------------------------------------------
/// <summary>
/// 	Set progress callback interval in seconds. Progress callback will not be called more
/// 	often than this value. This interval defaults to 1 second.
/// </summary>
///
/// <remarks>	 </remarks>
///
/// <param name="s">	  	[in,out] If non-null, the. </param>
/// <param name="seconds">	The seconds. </param>
///-------------------------------------------------------------------------------------------------

void ms_set_progress_interval(MediaScan *s, int seconds)
{
  s->progress_interval = seconds;
}

///-------------------------------------------------------------------------------------------------
/// <summary>	Determine if we should scan a path </summary>
///
/// <remarks>	 </remarks>
///
/// <param name="s">   	[in,out] If non-null, the. </param>
/// <param name="path">	Full pathname of the file. </param>
///
/// <returns>	. </returns>
///-------------------------------------------------------------------------------------------------

static int _should_scan(MediaScan *s, const char *path)
{
  char *p = NULL;
  char *found = NULL;
  char *ext = strrchr(path, '.');

  if (ext != NULL) {
    // Copy the extension and lowercase it
    char extc[10];
    extc[0] = ',';
    strncpy(extc + 1, ext + 1, 7);
    extc[9] = 0;
    
    p = &extc[1];
    while (*p != 0) {
      *p = tolower(*p);
      p++;
    }
    *p++ = ',';
    *p = 0;
    
    if (s->nignore_exts) {
      // Check for ignored extension
      int i;
      for (i = 0; i < s->nignore_exts; i++) {
        if (strstr(extc, s->ignore_exts[i]))
          return 0;
      }
    }
    
    
    found = strstr(AudioExts, extc);
    if (found)
      return TYPE_AUDIO;
    
    found = strstr(VideoExts, extc);
    if (found)
      return TYPE_VIDEO;
    
    found = strstr(ImageExts, extc);
    if (found)
      return TYPE_IMAGE;
    
    return 0;
  }
      
  return 0;
}

///-------------------------------------------------------------------------------------------------
/// <summary>	Recursively walk a directory struction </summary>
///
/// <remarks>	 </remarks>
///
/// <param name="s">	 	[in,out] If non-null, the. </param>
/// <param name="path">  	Full pathname of the file. </param>
/// <param name="curdir">	[in,out] If non-null, the curdir. </param>
///-------------------------------------------------------------------------------------------------

static void recurse_dir(MediaScan *s, const char *path, struct dirq_entry *curdir)
{
  char *dir = NULL;
  char *p = NULL;
  DIR *dirp;
  struct dirq *subdirq;
  char *tmp_full_path;
  struct dirent *dp;

  if (path[0] != '/') { // XXX Win32
    // Get full path
    char *buf = (char *)malloc((size_t)PathMax);
    if (buf == NULL) {
      LOG_ERROR("Out of memory for directory scan\n");
      return;
    }

#ifdef WIN32
    dir = _getcwd(buf, (size_t)PathMax);
#else
    dir = getcwd(buf, (size_t)PathMax);
#endif

    strcat(dir, "/");
    strcat(dir, path);
  }
  else {
    dir = strdup(path);
  }

  // Strip trailing slash if any
  p = &dir[0];
  while (*p != 0) {
#ifdef _WIN32
    if (p[1] == 0 && (*p == '/' || *p == '\\'))
#else
    if (p[1] == 0 && *p == '/')
#endif
      *p = 0;
    p++;
  }
  
  LOG_LEVEL(2, "Recursed into %s\n", dir);

  if ((dirp = opendir(dir)) == NULL) {
    LOG_ERROR("Unable to open directory %s: %s\n", dir, strerror(errno));
    goto out;
  }
  
  subdirq = malloc(sizeof(struct dirq));
  SIMPLEQ_INIT(subdirq);

  tmp_full_path = malloc((size_t)PathMax);

  
  while ((dp = readdir(dirp)) != NULL) {
    char *name = dp->d_name;

    // skip all dot files
    if (name[0] != '.') {
        // Construct full path
        *tmp_full_path = 0;
        strcat(tmp_full_path, dir);
        strcat(tmp_full_path, "/");
        strcat(tmp_full_path, name);
        
      // XXX some platforms may be missing d_type/DT_DIR
      if (dp->d_type == DT_DIR) {        
        // Entry for complete list of dirs
        // XXX somewhat inefficient, we create this for every directory
        // even those that don't end up having any scannable files
		struct dirq_entry *subdir_entry;
        struct dirq_entry *entry = malloc(sizeof(struct dirq_entry));
        entry->dir = strdup(tmp_full_path);
        entry->files = malloc(sizeof(struct fileq));
        SIMPLEQ_INIT(entry->files);
        
        // Temporary list of subdirs of the current directory
        subdir_entry = malloc(sizeof(struct dirq_entry));
        
        // Copy entry to subdir_entry, dir will be freed by ms_destroy()
        memcpy(subdir_entry, entry, sizeof(struct dirq_entry));
        SIMPLEQ_INSERT_TAIL(subdirq, subdir_entry, entries);
        
        SIMPLEQ_INSERT_TAIL((struct dirq *)s->_dirq, entry, entries);
        
        s->progress->dir_total++;
        
        LOG_LEVEL(2, "  [%5d] subdir: %s\n", s->progress->dir_total, entry->dir);
      }
      else {
        if ( _should_scan(s, name) ) {
          // To save memory by not storing the full path to every file,
          // each dir has a list of files in that dir
          struct fileq_entry *entry = malloc(sizeof(struct fileq_entry));
          entry->file = strdup(name);
          SIMPLEQ_INSERT_TAIL(curdir->files, entry, entries);
          
          s->progress->file_total++;
          
          LOG_LEVEL(2, "  [%5d] file: %s\n", s->progress->file_total, entry->file);
          
          // Scan the file
          ms_scan_file(s, tmp_full_path);
        }
      }
    }
  }
    
  closedir(dirp);
  
  // Send progress update
  if (s->on_progress) {
    struct timeval now;
    gettimeofday(&now, NULL);
    
    if (now.tv_sec - s->progress->_last_callback >= s->progress_interval) {
      s->progress->cur_item = dir;
      s->progress->_last_callback = now.tv_sec;
      s->on_progress(s, s->progress);
    }
  }

  // process subdirs
  while (!SIMPLEQ_EMPTY(subdirq)) {
    struct dirq_entry *subdir_entry = SIMPLEQ_FIRST(subdirq);
    SIMPLEQ_REMOVE_HEAD(subdirq, entries);
    recurse_dir(s, subdir_entry->dir, subdir_entry);
    free(subdir_entry);
  }
  
  free(subdirq);
  free(tmp_full_path);

out:
  free(dir);
}

///-------------------------------------------------------------------------------------------------
/// <summary>
/// 	Begin a recursive scan of all paths previously provided to ms_add_path(). If async mode
/// 	is enabled, this call will return immediately. You must obtain the file descriptor using
/// 	ms_async_fd and this must be checked using an event loop or select(). When the fd becomes
/// 	readable you must call ms_async_process to trigger any necessary callbacks.
/// </summary>
///
/// <remarks>	 </remarks>
///
/// <param name="s">	[in,out] If non-null, the. </param>
///-------------------------------------------------------------------------------------------------

void ms_scan(MediaScan *s)
{
  int i = 0;  

  if (s->on_result == NULL) {
    LOG_ERROR("Result callback not set, aborting scan\n");
    return;
  }
  
  if (s->async) {
    LOG_ERROR("async mode not yet supported\n");
    // XXX TODO
  }
  
  for (i = 0; i < s->npaths; i++) {
	char *phase = NULL;
    struct dirq_entry *entry = malloc(sizeof(struct dirq_entry));
    entry->dir = strdup("/"); // so free doesn't choke on this item later
    entry->files = malloc(sizeof(struct fileq));
    SIMPLEQ_INIT(entry->files);
    SIMPLEQ_INSERT_TAIL((struct dirq *)s->_dirq, entry, entries);
    
    phase = (char *)malloc((size_t)PathMax);
    sprintf(phase, "Discovering files in %s", s->paths[i]);
    s->progress->phase = phase;
    
    LOG_LEVEL(1, "Scanning %s\n", s->paths[i]);
    recurse_dir(s, s->paths[i], entry);
    
    // Send final progress callback
    if (s->on_progress) {
      s->progress->cur_item = NULL;
      s->on_progress(s, s->progress);
    }
    
    free(phase);
  }
}

///-------------------------------------------------------------------------------------------------
/// <summary>
/// 	Scan a single file. Everything that applies to ms_scan also applies to this function. If
/// 	you know the type of the file, set the type paramter to one of TYPE_AUDIO, TYPE_VIDEO, or
/// 	TYPE_IMAGE. Set it to TYPE_UNKNOWN to have it determined automatically.
/// </summary>
///
/// <remarks>	 </remarks>
///
/// <param name="s">			[in,out] If non-null, the. </param>
/// <param name="full_path">	Full pathname of the full file. </param>
///-------------------------------------------------------------------------------------------------

void ms_scan_file(MediaScan *s, const char *full_path)
{
  MediaScanResult *r = NULL;

  LOG_LEVEL(1, "Scanning file %s\n", full_path);
  
  r = result_create();
  if (r == NULL)
    return;
    
  s->on_result(s, r);
  
  result_destroy(r);
}

/*
ScanData
mediascan_scan_file(const char *path, int flags)
{
  _init();
  
  ScanData s = NULL;
  int type = _is_media(path);
            
  if (type == TYPE_VIDEO && !(flags & SKIP_VIDEO)) {
    s = mediascan_new_ScanData(path, flags, type);
  }
  else if (type == TYPE_AUDIO && !(flags & SKIP_AUDIO)) {
    s = mediascan_new_ScanData(path, flags, type);
  }
  else if (type == TYPE_IMAGE && !(flags & SKIP_IMAGE)) {
    s = mediascan_new_ScanData(path, flags, type);
  }
  
  return s;
}

*/
