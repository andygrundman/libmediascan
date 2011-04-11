///-------------------------------------------------------------------------------------------------
// file:	libmediascan\src\mediascan.c
//
// summary:	mediascan class
///-------------------------------------------------------------------------------------------------

#ifdef _DEBUG
#define CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>
#else
#include <stdlib.h>
#endif


#include <ctype.h>

#ifndef WIN32
#include <dirent.h>
#include <sys/time.h>
#include <unistd.h>
#else
#include <time.h>
#include <Winsock2.h>
#include <direct.h>
#endif

#include <libmediascan.h>
#include <libavformat/avformat.h>

#ifdef WIN32
#include "mediascan_win32.h"
#endif


#include "common.h"
#include "queue.h"
#include "progress.h"
#include "result.h"
#include "error.h"
#include "mediascan.h"
#include "image.h"
#include "video.h"


// If we are on MSVC, disable some stupid MSVC warnings
#ifdef _MSC_VER
#pragma warning( disable: 4996 )
#endif

// DLNA support
#include "libdlna/dlna_internals.h"

// Global log level flag
enum log_level Debug = ERR;
static int Initialized = 0;
int ms_errno = 0;
long PathMax = MAX_PATH;

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
static const char *VideoExts = ",asf,avi,divx,flv,hdmov,m1v,m2p,m2t,m2ts,m2v,m4v,mkv,mov,mpg,mpeg,mpe,mp2p,mp2t,mp4,mts,pes,ps,ts,vob,webm,wmv,xvid,3gp,3g2,3gp2,3gpp,";
static const char *ImageExts = ",jpg,png,gif,bmp,jpeg,";

#define REGISTER_DECODER(X,x) { \
          extern AVCodec ff_##x##_decoder; \
		  avcodec_register(&ff_##x##_decoder); /* printf("%X - %s\n", &ff_##x##_decoder, #X); */}

#define REGISTER_PARSER(X,x) { \
          extern AVCodecParser ff_##x##_parser; \
		  av_register_codec_parser(&ff_##x##_parser); /* printf("%X - %s\n", &ff_##x##_parser, #X); */}

///-------------------------------------------------------------------------------------------------
///  Register codecs to be used with ffmpeg.
///
/// @author Andy Grundman
/// @date 03/15/2011
///
/// ### remarks .
///-------------------------------------------------------------------------------------------------

static void register_codecs(void)
{
//printf("----------------------- REGISTER_DECODER -------------------------\n");

  // Video codecs
  REGISTER_DECODER (H263, h263);
  REGISTER_DECODER (H264, h264);
  REGISTER_DECODER (MPEG1VIDEO, mpeg1video);
  REGISTER_DECODER (MPEG2VIDEO, mpeg2video);
  REGISTER_DECODER (MPEG4, mpeg4);
  REGISTER_DECODER (MSMPEG4V1, msmpeg4v1);
  REGISTER_DECODER (MSMPEG4V2, msmpeg4v2);
  REGISTER_DECODER (MSMPEG4V3, msmpeg4v3);
  REGISTER_DECODER (VP6, vp6);
  REGISTER_DECODER (VP6F, vp6f);
  REGISTER_DECODER (VP8, vp8);
  REGISTER_DECODER (WMV1, wmv1);
  REGISTER_DECODER (WMV2, wmv2);
  REGISTER_DECODER (WMV3, wmv3);


  // Audio codecs, needed to get details of audio tracks in videos
  REGISTER_DECODER (AAC, aac);
  REGISTER_DECODER (AC3, ac3);
  REGISTER_DECODER (DCA, dca); // DTS
  REGISTER_DECODER (MP2, mp2);
  REGISTER_DECODER (MP3, mp3);
  REGISTER_DECODER (VORBIS, vorbis);
  REGISTER_DECODER (WMAPRO, wmapro);
  REGISTER_DECODER (WMAV1, wmav1);
  REGISTER_DECODER (WMAV2, wmav2);
  REGISTER_DECODER (WMAVOICE, wmavoice);
  
  // Not sure which PCM codecs we need
  REGISTER_DECODER (PCM_DVD, pcm_dvd);
  REGISTER_DECODER (PCM_S16BE, pcm_s16be);
  REGISTER_DECODER (PCM_S16LE, pcm_s16le);
  REGISTER_DECODER (PCM_S24BE, pcm_s24be);
  REGISTER_DECODER (PCM_S24LE, pcm_s24le);
  
  // Subtitles
  REGISTER_DECODER (ASS, ass);
  REGISTER_DECODER (DVBSUB, dvbsub);
  REGISTER_DECODER (DVDSUB, dvdsub);
  REGISTER_DECODER (PGSSUB, pgssub);
  REGISTER_DECODER (XSUB, xsub);
  

//  printf("----------------------- REGISTER_PARSER -------------------------\n");

  // Parsers 
  REGISTER_PARSER (AAC, aac);
  REGISTER_PARSER (AC3, ac3);
  REGISTER_PARSER (DCA, dca); // DTS
  REGISTER_PARSER (H263, h263);
  REGISTER_PARSER (H264, h264);
  REGISTER_PARSER (MPEG4VIDEO, mpeg4video);
  REGISTER_PARSER (MPEGAUDIO, mpegaudio);
  REGISTER_PARSER (MPEGVIDEO, mpegvideo);
} /* register_codecs() */

#define REGISTER_DEMUXER(X,x) { \
    extern AVInputFormat ff_##x##_demuxer; \
	av_register_input_format(&ff_##x##_demuxer); /* printf("%X  - %s\n", &ff_##x##_demuxer, #X); */}
#define REGISTER_PROTOCOL(X,x) { \
    extern URLProtocol ff_##x##_protocol; \
    av_register_protocol2(&ff_##x##_protocol, sizeof(ff_##x##_protocol)); }

///-------------------------------------------------------------------------------------------------
///  Registers the formats for FFmpeg.
///
/// @author Andy Grundman
/// @date 03/15/2011
///
/// ### remarks .
///-------------------------------------------------------------------------------------------------

static void register_formats(void)
{
  //printf("----------------------- REGISTER_DEMUXER -------------------------\n");


  // demuxers
  REGISTER_DEMUXER (ASF, asf);
  REGISTER_DEMUXER (AVI, avi);
  REGISTER_DEMUXER (FLV, flv);
  REGISTER_DEMUXER (H264, h264);
  REGISTER_DEMUXER (MATROSKA, matroska);
  REGISTER_DEMUXER (MOV, mov);
  REGISTER_DEMUXER (MPEGPS, mpegps);             // VOB files
  REGISTER_DEMUXER (MPEGTS, mpegts);
  REGISTER_DEMUXER (MPEGVIDEO, mpegvideo);


//    printf("----------------------- REGISTER_PROTOCOL -------------------------\n");

  // protocols
  REGISTER_PROTOCOL (FILE, file);
} /* register_formats() */

///-------------------------------------------------------------------------------------------------
///  Initialises ffmpeg.
///
/// @author Andy Grundman
/// @date 03/15/2011
///
/// ### remarks .
///-------------------------------------------------------------------------------------------------

static void _init(void)
{
  if (Initialized)
    return;
  
  register_codecs();
  register_formats();
#ifndef WIN32
  unix_init();
#endif

  Initialized = 1;
  ms_errno = 0;
} /* _init() */

///-------------------------------------------------------------------------------------------------
///  Set the logging level.
///
/// @author Andy Grundman
/// @date 03/15/2011
///
/// @param level The level.
///
/// ### remarks .
///-------------------------------------------------------------------------------------------------

void ms_set_log_level(enum log_level level)
{
  int av_level = AV_LOG_PANIC;
  
  Debug = level;
  
  // Set the corresponding ffmpeg log level
  switch (level) {
    case ERR:  av_level = AV_LOG_ERROR; break;
    case INFO:   av_level = AV_LOG_INFO; break;
    case MEMORY: av_level = AV_LOG_VERBOSE; break;
    case WARN:   av_level = AV_LOG_WARNING; break;
    case DEBUG: 
    default: break;
  }
  
  av_log_set_level(av_level);
} /* ms_set_log_level() */

///-------------------------------------------------------------------------------------------------
///  Allocate a new MediaScan object.
///
/// @author Andy Grundman
/// @date 03/15/2011
///
/// @return null if it fails, else.
///
/// ### remarks .
///-------------------------------------------------------------------------------------------------

MediaScan *ms_create(void)
{
  MediaScan *s = NULL;
  dlna_t *dlna = NULL;
  
  _init();
  
  s = (MediaScan *)calloc(sizeof(MediaScan), 1);
  if (s == NULL) {
	ms_errno = MSENO_MEMERROR;
    FATAL("Out of memory for new MediaScan object\n");
    return NULL;
  }
  
  s->progress = progress_create();
  
  // List of all dirs found
  s->_dirq = malloc(sizeof(struct dirq));
  SIMPLEQ_INIT((struct dirq *)s->_dirq);
  
  // We can't use libdlna's init function because it loads everything in ffmpeg
  dlna = (dlna_t *)calloc(sizeof(dlna_t), 1);
  dlna->inited = 1;
  s->_dlna = (void *)dlna;
  dlna_register_all_media_profiles(dlna);

  InitCriticalSection(&s->CriticalSection);

  
  return s;
} /* ms_create() */

///-------------------------------------------------------------------------------------------------
///  Destroy the given MediaScan object. If a scan is currently in progress it will be aborted.
///
/// @author Andy Grundman
/// @date 03/15/2011
///
/// @param [in,out] s If non-null, the.
///
/// ### remarks .
///-------------------------------------------------------------------------------------------------

void ms_destroy(MediaScan *s)
{
  int i;
  
  for (i = 0; i < s->npaths; i++) {
    free( s->paths[i] );
  }
  
  for (i = 0; i < s->nignore_exts; i++) {
    free( s->ignore_exts[i] );
  }
  
  for (i = 0; i < s->nthumbspecs; i++) {
    free( s->thumbspecs[i] );
  }
  
  progress_destroy(s->progress);
  
  free(s->_dirq);
  free(s->_dlna);

  ms_clear_watch(s);

  CleanupCriticalSection(&s->CriticalSection);

  free(s);
} /* ms_destroy() */

///-------------------------------------------------------------------------------------------------
///  Add a path to be scanned. Up to 128 paths may be added before beginning the scan.
///
/// @author Andy Grundman
/// @date 03/15/2011
///
/// @param [in,out] s If non-null, the.
/// @param path		  Full pathname of the file.
///
/// ### remarks .
///-------------------------------------------------------------------------------------------------

void ms_add_path(MediaScan *s, const char *path)
{
  int len = 0;
  char *tmp = NULL;

  if(s == NULL) {
	ms_errno = MSENO_NULLSCANOBJ;
    FATAL("MediaScan = NULL, aborting scan\n");
    return;
  }

  if (s->npaths == MAX_PATHS) {
    FATAL("Path limit reached (%d)\n", MAX_PATHS);
    return;
  }
  
  len = strlen(path) + 1;
  tmp = malloc(len);
  if (tmp == NULL) {
    FATAL("Out of memory for adding path\n");
    return;
  }
  
  strncpy(tmp, path, len);
  
  s->paths[ s->npaths++ ] = tmp;
}	/* ms_add_path() */

///-------------------------------------------------------------------------------------------------
///  Add a file extension to ignore all files with this extension.
///
/// @author Andy Grundman
/// @date 03/15/2011
///
/// @param [in,out] s If non-null, the.
/// @param extension
/// 3 special all-caps extensions may be provided: AUDIO - ignore all audio-
/// related extensions. IMAGE - ignore all image-related extensions. VIDEO -
/// ignore all video-related extensions.
///
/// ### remarks .
///-------------------------------------------------------------------------------------------------

void ms_add_ignore_extension(MediaScan *s, const char *extension)
{
  int len = 0;
  char *tmp = NULL;

  if(s == NULL) {
	ms_errno = MSENO_NULLSCANOBJ;
    FATAL("MediaScan = NULL, aborting scan\n");
    return;
  }

  if (s->nignore_exts == MAX_IGNORE_EXTS) {
    FATAL("Ignore extension limit reached (%d)\n", MAX_IGNORE_EXTS);
    return;
  }
  
  len = strlen(extension) + 1;
  tmp = malloc(len);
  if (tmp == NULL) {
    FATAL("Out of memory for ignore extension\n");
    return;
  }
  
  strncpy(tmp, extension, len);
  
  s->ignore_exts[ s->nignore_exts++ ] = tmp;
} /* ms_add_ignore_extension() */

void
ms_add_thumbnail_spec(MediaScan *s, enum thumb_format format, int width, int height, int keep_aspect, uint32_t bgcolor, int quality)
{
  // Must have at least width or height
  if (width > 0 || height > 0) {
    MediaScanThumbSpec *spec = (MediaScanThumbSpec *)calloc(sizeof(MediaScanThumbSpec), 1);
    spec->format = format;
    spec->width = width;
    spec->height = height;
    spec->keep_aspect = keep_aspect;
    spec->bgcolor = bgcolor;
    spec->jpeg_quality = quality;
    
    LOG_DEBUG("ms_add_thumbnail_spec width %d height %d\n", spec->width, spec->height);
    
    s->thumbspecs[ s->nthumbspecs++ ] = spec;
  }
}

///-------------------------------------------------------------------------------------------------
///  By default, scans are synchronous. This means the call to ms_scan will not return until
/// 	the scan is finished. To enable background asynchronous scanning, pass a true value to
/// 	this function.
///
/// @author Andy Grundman
/// @date 03/15/2011
///
/// @param [in,out] s If non-null, the.
/// @param enabled    The enabled.
///
/// ### remarks .
///-------------------------------------------------------------------------------------------------

void ms_set_async(MediaScan *s, int enabled)
{
   if(s == NULL) {
	ms_errno = MSENO_NULLSCANOBJ;
    LOG_ERROR("MediaScan = NULL, aborting\n");
    return;
  }

  s->async = enabled ? 1 : 0;
} /* ms_set_async() */

///-------------------------------------------------------------------------------------------------
///  Set a callback that will be called for every scanned file. This callback is required or a
/// 	scan cannot be started.
///
/// @author Andy Grundman
/// @date 03/15/2011
///
/// @param [in,out] s If non-null, the.
/// @param callback   The callback.
///
/// ### remarks .
///-------------------------------------------------------------------------------------------------

void ms_set_result_callback(MediaScan *s, ResultCallback callback)
{
   if(s == NULL) {
	ms_errno = MSENO_NULLSCANOBJ;
    LOG_ERROR("MediaScan = NULL, aborting\n");
    return;
  }

  s->on_result = callback;
} /* ms_set_result_callback() */

///-------------------------------------------------------------------------------------------------
///  Set a callback that will be called for all errors. This callback is optional.
///
/// @author Andy Grundman
/// @date 03/15/2011
///
/// @param [in,out] s If non-null, the.
/// @param callback   The callback.
///
/// ### remarks .
///-------------------------------------------------------------------------------------------------

void ms_set_error_callback(MediaScan *s, ErrorCallback callback)
{
  if(s == NULL) {
	ms_errno = MSENO_NULLSCANOBJ;
    LOG_ERROR("MediaScan = NULL, aborting\n");
    return;
  }

  s->on_error = callback;
} /* ms_set_error_callback() */

///-------------------------------------------------------------------------------------------------
///  Set a callback that will be called during the scan with progress details. This callback
/// 	is optional.
///
/// @author Andy Grundman
/// @date 03/15/2011
///
/// @param [in,out] s If non-null, the.
/// @param callback   The callback.
///
/// ### remarks .
///-------------------------------------------------------------------------------------------------

void ms_set_progress_callback(MediaScan *s, ProgressCallback callback)
{
  if(s == NULL) {
	ms_errno = MSENO_NULLSCANOBJ;
    LOG_ERROR("MediaScan = NULL, aborting\n");
    return;
  }
  s->on_progress = callback;
} /* ms_set_progress_callback() */

///-------------------------------------------------------------------------------------------------
///  Set progress callback interval in seconds. Progress callback will not be called more
/// 	often than this value. This interval defaults to 1 second.
///
/// @author Andy Grundman
/// @date 03/15/2011
///
/// @param [in,out] s If non-null, the.
/// @param seconds    The seconds.
///
/// ### remarks .
///-------------------------------------------------------------------------------------------------

void ms_set_progress_interval(MediaScan *s, int seconds)
{
  if(s == NULL) {
	ms_errno = MSENO_NULLSCANOBJ;
    LOG_ERROR("MediaScan = NULL, aborting\n");
    return;
  }

  if(s->progress == NULL) {
	ms_errno = MSENO_NULLSCANOBJ;
    LOG_ERROR("Progress = NULL, aborting\n");
    return;
  }

  s->progress->interval = seconds;
} /* ms_set_progress_interval() */

///-------------------------------------------------------------------------------------------------
///  Set userdata.
///
/// @author Andy Grundman
/// @date 04/05/2011
///
/// @param [in,out] s    If non-null, the.
/// @param [in,out] data If non-null, the data.
///-------------------------------------------------------------------------------------------------

void ms_set_userdata(MediaScan *s, void *data)
{
  s->userdata = data;
} /* ms_set_userdata() */

///-------------------------------------------------------------------------------------------------
///  Watch a directory in the background.
///
/// @author Henry Bennett
/// @date 03/22/2011
///
/// @param path Path name of the folder to watch
/// @param callback Callback with the changes
///-------------------------------------------------------------------------------------------------
void ms_watch_directory(MediaScan *s, const char *path, FolderChangeCallback callback)
{
	thread_data_type *thread_data;

	s->on_background = callback;

	// This folder monitoring code is only valid for Win32
#ifdef WIN32

	s->ghSignalEvent = CreateEvent( 
    NULL,					// default security attributes
    TRUE,					// manual-reset event
    FALSE,					// initial state is nonsignaled
    "StopEvent"				// "StopEvent" name
    );

	if(s->ghSignalEvent == NULL)
	{
		ms_errno = MSENO_THREADERROR;
		LOG_ERROR("Can't create event\n");
		return;
	}

	thread_data = (thread_data_type*) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(thread_data_type));


	thread_data->lpDir = (char*)path;
	thread_data->s = s;

	s->hThread = CreateThread( 
    NULL,                   // default security attributes
    0,                      // use default stack size  
    WatchDirectory,			// WatchDirectory thread
    (void*)thread_data,	// (void*)thread_data_type
    0,                      // use default creation flags 
    &s->dwThreadId);			// returns the thread identifier 

	if(s->hThread == NULL)
	{
		ms_errno = MSENO_THREADERROR;
		LOG_ERROR("Can't create watch thread\n");
		return;
	}

#endif

} /* ms_watch_directory() */

///-------------------------------------------------------------------------------------------------
///  Clear watch list
///
/// @author Henry Bennett
/// @date 03/22/2011
///
///-------------------------------------------------------------------------------------------------
void ms_clear_watch(MediaScan *s)
{
// This folder monitoring code is only valid for Win32
#ifdef WIN32
	SetEvent(s->ghSignalEvent);

	// Wait until all threads have terminated.
	WaitForSingleObject(s->hThread, INFINITE);

	CloseHandle(s->hThread);
	CloseHandle(s->ghSignalEvent);
#endif

} /* ms_clear_watch() */

///-------------------------------------------------------------------------------------------------
///  Determine if we should scan a path.
///
/// @author Andy Grundman
/// @date 03/15/2011
///
/// @param [in,out] s If non-null, the.
/// @param path		  Full pathname of the file.
///
/// @return .
///
/// ### remarks .
///-------------------------------------------------------------------------------------------------

int _should_scan(MediaScan *s, const char *path)
{
  char *p = NULL;
  char *found = NULL;
  char *ext = strrchr(path, '.');
  int skip_audio = 0;
  int skip_video = 0;
  int skip_image = 0;

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
          return TYPE_UNKNOWN;
        
        if (!strcmp("AUDIO", s->ignore_exts[i]))
          skip_audio = 1;
        else if (!strcmp("VIDEO", s->ignore_exts[i]))
          skip_video = 1;
        else if (!strcmp("IMAGE", s->ignore_exts[i]))
          skip_image = 1;
      }
    }
    
    found = strstr(VideoExts, extc);
    if (found)
      return skip_video ? TYPE_UNKNOWN : TYPE_VIDEO;
    
    found = strstr(AudioExts, extc);
    if (found)
      return skip_audio ? TYPE_UNKNOWN : TYPE_AUDIO;
    
    found = strstr(ImageExts, extc);
    if (found)
      return skip_image ? TYPE_UNKNOWN : TYPE_IMAGE;
    
    return TYPE_UNKNOWN;
  }
      
  return TYPE_UNKNOWN;
} /* _should_scan() */

///-------------------------------------------------------------------------------------------------
///  Begin a recursive scan of all paths previously provided to ms_add_path(). If async mode
/// 	is enabled, this call will return immediately. You must obtain the file descriptor using
/// 	ms_async_fd and this must be checked using an event loop or select(). When the fd becomes
/// 	readable you must call ms_async_process to trigger any necessary callbacks.
///
/// @author Andy Grundman
/// @date 03/15/2011
///
/// @param [in,out] s If non-null, the.
///
/// ### remarks .
///-------------------------------------------------------------------------------------------------
void ms_scan(MediaScan *s)
{
  int i;
  struct dirq *dir_head = (struct dirq *)s->_dirq;
  struct dirq_entry *dir_entry = NULL;
  struct fileq *file_head = NULL;
  struct fileq_entry *file_entry = NULL;

  char tmp_full_path[MAX_PATH]; 

  StartCriticalSection(&s->CriticalSection);
 
  if (s->on_result == NULL) {
    LOG_ERROR("Result callback not set, aborting scan\n");

    EndCriticalSection(&s->CriticalSection);

    return;
  }
  
  if (s->async) {
    LOG_ERROR("async mode not yet supported\n");
    // XXX TODO
  }
  
  // Build a list of all directories and paths
  // We do this first so we can present an accurate scan eta later
  progress_start_phase(s->progress, "Discovering");
  
  for (i = 0; i < s->npaths; i++) {
    LOG_INFO("Scanning %s\n", s->paths[i]);
    recurse_dir(s, s->paths[i]);
  }
  
  // Scan all files found
  progress_start_phase(s->progress, "Scanning");
  
  while (!SIMPLEQ_EMPTY(dir_head)) {
    dir_entry = SIMPLEQ_FIRST(dir_head);
    
    file_head = dir_entry->files;
    while (!SIMPLEQ_EMPTY(file_head)) {
      file_entry = SIMPLEQ_FIRST(file_head);
      
      // Construct full path
	  strcpy(tmp_full_path, dir_entry->dir);
#ifdef WIN32
      strcat(tmp_full_path, "\\");
#else
      strcat(tmp_full_path, "/");
#endif
      strcat(tmp_full_path, file_entry->file);
      
      ms_scan_file(s, tmp_full_path, file_entry->type);
      
      // Send progress update if necessary
      if (s->on_progress) {
        s->progress->done++;
        if (progress_update(s->progress, tmp_full_path))
          s->on_progress(s, s->progress, s->userdata);
      }
      
      SIMPLEQ_REMOVE_HEAD(file_head, entries);
      free(file_entry->file);
      free(file_entry);
    }
    
    SIMPLEQ_REMOVE_HEAD(dir_head, entries);
    free(dir_entry->dir);
    free(dir_entry->files);
    free(dir_entry);
  }
  
  // Send final progress callback
  if (s->on_progress) {
    s->progress->cur_item = NULL;
    s->on_progress(s, s->progress, s->userdata);
  }

  EndCriticalSection(&s->CriticalSection);

} /* ms_scan() */

///-------------------------------------------------------------------------------------------------
///  Scan a single file. Everything that applies to ms_scan also applies to this function. If
/// 	you know the type of the file, set the type paramter to one of TYPE_AUDIO, TYPE_VIDEO, or
/// 	TYPE_IMAGE. Set it to TYPE_UNKNOWN to have it determined automatically.
///
/// @author Andy Grundman
/// @date 03/15/2011
///
/// @param [in,out] s If non-null, the.
/// @param full_path  Full pathname of the full file.
///
/// ### remarks .
///-------------------------------------------------------------------------------------------------
 void ms_scan_file(MediaScan *s, const char *full_path, enum media_type type)
{
  MediaScanError *e = NULL;
  MediaScanResult *r = NULL;

  if(s == NULL) {
	ms_errno = MSENO_NULLSCANOBJ;
    LOG_ERROR("MediaScan = NULL, aborting scan\n");
    return;
  }

  if (s->on_result == NULL) {
	ms_errno = MSENO_NORESULTCALLBACK;
    LOG_ERROR("Result callback not set, aborting scan\n");
    return;
  }
  
  LOG_INFO("Scanning file %s\n", full_path);
  
  if (type == TYPE_UNKNOWN) {
    // auto-detect type
    type = _should_scan(s, full_path);
    if (!type) {
      if (s->on_error) {
		ms_errno = MSENO_SCANERROR;
        e = error_create(full_path, MS_ERROR_TYPE_UNKNOWN, "Unrecognized file extension");
        s->on_error(s, e, s->userdata);
        error_destroy(e);
        return;
      }
    }
  }
  
  r = result_create(s);
  if (r == NULL)
    return;
  
  r->type = type;
  r->path = full_path;
  
  if ( result_scan(r) ) {
    s->on_result(s, r, s->userdata);
  }
  else if (s->on_error && r->error) {
    s->on_error(s, r->error, s->userdata);
  }
  
  result_destroy(r);
} /* ms_scan_file() */

///-------------------------------------------------------------------------------------------------
///  Query if 'path' is absolute path.
///
/// @author Henry Bennett
/// @date 03/18/2011
///
/// @param path Pathname to check.
///
/// @return true if absolute path, false if not.
///
/// ### remarks Henry Bennett, 03/16/2011.
///-------------------------------------------------------------------------------------------------

bool is_absolute_path(const char *path) {

	if(path == NULL)
		return FALSE;

	// \workspace, /workspace, etc
	if( strlen(path) > 1 && ( path[0] == '/' || path[0] == '\\') ) 
		return TRUE;

#ifdef WIN32
	// C:\, D:\, etc
	if( strlen(path) > 2 && path[1] == ':' )
		return TRUE;
#endif

	return FALSE;
} /* is_absolute_path() */

const uint8_t *
ms_result_get_thumbnail(MediaScanResult *r, int index, int *length)
{
  uint8_t *ret = NULL;
  *length = 0;
  
  // XXX refactor, thumbnails should be stored in result
  switch (r->type) {
    case TYPE_VIDEO:
      ret = video_get_thumbnail(r->video, index, length);
      break;
    
    case TYPE_IMAGE:
      ret = image_get_thumbnail(r->image, index, length);
      break;
    
    case TYPE_AUDIO:
      // XXX
      break;
  }
  
  return (const uint8_t *)ret;
}
