#include <ctype.h>
#include <dirent.h>
#include <stdlib.h>

#include <libavformat/avformat.h>

#include <libmediascan.h>
#include "common.h"
#include "queue.h"
#include "progress.h"
#include "result.h"
#include "error.h"

// DLNA support
#include "libdlna/dlna_internals.h"

// Global log level flag
enum log_level Debug = ERROR;

// Global max path length
long PathMax = 0;

// Local initialized flag
static int Initialized = 0;

// File/dir queue struct definitions
struct fileq_entry {
  char *file;
  enum media_type type;
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
static const char *VideoExts = ",asf,avi,divx,flv,hdmov,m1v,m2p,m2t,m2ts,m2v,m4v,mkv,mov,mpg,mpeg,mpe,mp2p,mp2t,mp4,mts,pes,ps,ts,vob,webm,wmv,xvid,3gp,3g2,3gp2,3gpp,";
static const char *ImageExts = ",jpg,png,gif,bmp,jpeg,";

#define REGISTER_DECODER(X,x) { \
          extern AVCodec ff_##x##_decoder; \
          avcodec_register(&ff_##x##_decoder); }
#define REGISTER_PARSER(X,x) { \
          extern AVCodecParser ff_##x##_parser; \
          av_register_codec_parser(&ff_##x##_parser); }

static void
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
  
  // Parsers 
  REGISTER_PARSER (AAC, aac);
  REGISTER_PARSER (AC3, ac3);
  REGISTER_PARSER (DCA, dca); // DTS
  REGISTER_PARSER (H264, h264);
  REGISTER_PARSER (MPEG4VIDEO, mpeg4video);
  REGISTER_PARSER (MPEGAUDIO, mpegaudio);
  REGISTER_PARSER (MPEGVIDEO, mpegvideo);
}

#define REGISTER_DEMUXER(X,x) { \
    extern AVInputFormat ff_##x##_demuxer; \
    av_register_input_format(&ff_##x##_demuxer); }
#define REGISTER_PROTOCOL(X,x) { \
    extern URLProtocol ff_##x##_protocol; \
    av_register_protocol2(&ff_##x##_protocol, sizeof(ff_##x##_protocol)); }

static void
register_formats(void)
{
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
  
  // protocols
  REGISTER_PROTOCOL (FILE, file);
}

static void
_init(void)
{
  if (Initialized)
    return;
  
  register_codecs();
  register_formats();
  
  PathMax = pathconf(".", _PC_PATH_MAX); // 1024
  
  Initialized = 1;
}

void
ms_set_log_level(enum log_level level)
{
  int av_level = AV_LOG_PANIC;
  
  Debug = level;
  
  // Set the corresponding ffmpeg log level
  switch (level) {
    case ERROR:  av_level = AV_LOG_ERROR; break;
    case WARN:   av_level = AV_LOG_WARNING; break;
    case INFO:   av_level = AV_LOG_INFO; break;
    case DEBUG: 
    case MEMORY: av_level = AV_LOG_VERBOSE; break;
    default: break;
  }
  
  av_log_set_level(av_level);
}

MediaScan *
ms_create(void)
{
  MediaScan *s = NULL;
  dlna_t *dlna = NULL;
  
  _init();
  
  s = (MediaScan *)calloc(sizeof(MediaScan), 1);
  if (s == NULL) {
    FATAL("Out of memory for new MediaScan object\n");
    return NULL;
  }
  
  s->npaths = 0;
  s->paths[0] = NULL;
  s->nignore_exts = 0;
  s->ignore_exts[0] = NULL;
  s->async = 0;
  s->async_fd = 0;
  
  s->progress = progress_create();
  
  s->on_result = NULL;
  s->on_error = NULL;
  s->on_progress = NULL;
  s->userdata = NULL;
  
  // List of all dirs found
  s->_dirq = malloc(sizeof(struct dirq));
  SIMPLEQ_INIT((struct dirq *)s->_dirq);
  
  // We can't use libdlna's init function because it loads everything in ffmpeg
  dlna = (dlna_t *)calloc(sizeof(dlna_t), 1);
  dlna->inited = 1;
  s->_dlna = (void *)dlna;
  dlna_register_all_media_profiles(dlna);
  
  return s;
}

void
ms_destroy(MediaScan *s)
{
  int i;
  
  for (i = 0; i < s->npaths; i++) {
    free( s->paths[i] );
  }
  
  for (i = 0; i < s->nignore_exts; i++) {
    free( s->ignore_exts[i] );
  }
  
  progress_destroy(s->progress);
  
  free(s->_dirq);
  free(s->_dlna);
  free(s);
}

void
ms_add_path(MediaScan *s, const char *path)
{
  if (s->npaths == MAX_PATHS) {
    LOG_ERROR("Path limit reached (%d)\n", MAX_PATHS);
    return;
  }
  
  int len = strlen(path) + 1;
  char *tmp = malloc(len);
  if (tmp == NULL) {
    FATAL("Out of memory for adding path\n");
    return;
  }
  
  strncpy(tmp, path, len);
  
  s->paths[ s->npaths++ ] = tmp;
}

void
ms_add_ignore_extension(MediaScan *s, const char *extension)
{
  if (s->nignore_exts == MAX_IGNORE_EXTS) {
    LOG_ERROR("Ignore extension limit reached (%d)\n", MAX_IGNORE_EXTS);
    return;
  }
  
  int len = strlen(extension) + 1;
  char *tmp = malloc(len);
  if (tmp == NULL) {
    FATAL("Out of memory for ignore extension\n");
    return;
  }
  
  strncpy(tmp, extension, len);
  
  s->ignore_exts[ s->nignore_exts++ ] = tmp;
}

void
ms_set_async(MediaScan *s, int enabled)
{
  s->async = enabled ? 1 : 0;
}

void
ms_set_result_callback(MediaScan *s, ResultCallback callback)
{
  s->on_result = callback;
}

void
ms_set_error_callback(MediaScan *s, ErrorCallback callback)
{
  s->on_error = callback;
}

void
ms_set_progress_callback(MediaScan *s, ProgressCallback callback)
{
  s->on_progress = callback;
}

void
ms_set_userdata(MediaScan *s, void *data)
{
  s->userdata = data;
}

void
ms_set_progress_interval(MediaScan *s, int seconds)
{
  s->progress->interval = seconds;
}

static int
_should_scan(MediaScan *s, const char *path)
{
  char *ext = strrchr(path, '.');
  if (ext != NULL) {
    // Copy the extension and lowercase it
    char extc[10];
    extc[0] = ',';
    strncpy(extc + 1, ext + 1, 7);
    extc[9] = 0;
    
    char *p = &extc[1];
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
      }
    }
    
    char *found;
    
    found = strstr(VideoExts, extc);
    if (found)
      return TYPE_VIDEO;
    
    found = strstr(AudioExts, extc);
    if (found)
      return TYPE_AUDIO;
    
    found = strstr(ImageExts, extc);
    if (found)
      return TYPE_IMAGE;
    
    return TYPE_UNKNOWN;
  }
      
  return TYPE_UNKNOWN;
}

static void
recurse_dir(MediaScan *s, const char *path)
{
  char *dir, *p;
  char *tmp_full_path;
  DIR *dirp;
  struct dirent *dp;
  struct dirq *subdirq; // list of subdirs of the current directory
  struct dirq_entry *parent_entry = NULL; // entry for current dir in s->_dirq

  if (path[0] != '/') { // XXX Win32
    // Get full path
    char *buf = (char *)malloc((size_t)PathMax);
    if (buf == NULL) {
      FATAL("Out of memory for directory scan\n");
      return;
    }

    dir = getcwd(buf, (size_t)PathMax);
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
  
  LOG_INFO("Recursed into %s\n", dir);
  
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
      // XXX some platforms may be missing d_type/DT_DIR
      if (dp->d_type == DT_DIR) {
        // Add to list of subdirectories we need to recurse into
        struct dirq_entry *subdir_entry = malloc(sizeof(struct dirq_entry));
        
        // Construct full path
        *tmp_full_path = 0;
        strcat(tmp_full_path, dir);
        strcat(tmp_full_path, "/");
        strcat(tmp_full_path, name);
        
        subdir_entry->dir = strdup(tmp_full_path);
        SIMPLEQ_INSERT_TAIL(subdirq, subdir_entry, entries);
        
        LOG_INFO("  subdir: %s\n", tmp_full_path);
      }
      else {
        enum media_type type = _should_scan(s, name);
        if (type) {
          struct fileq_entry *entry;
          
          if (parent_entry == NULL) {
            // Add parent directory to list of dirs with files
            parent_entry = malloc(sizeof(struct dirq_entry));
            parent_entry->dir = strdup(dir);
            parent_entry->files = malloc(sizeof(struct fileq));
            SIMPLEQ_INIT(parent_entry->files);
            SIMPLEQ_INSERT_TAIL((struct dirq *)s->_dirq, parent_entry, entries);
          }
          
          // Add scannable file to this directory list
          entry = malloc(sizeof(struct fileq_entry));
          entry->file = strdup(name);
          entry->type = type;
          SIMPLEQ_INSERT_TAIL(parent_entry->files, entry, entries);
          
          s->progress->total++;
          
          LOG_INFO("  [%5d] file: %s\n", s->progress->total, entry->file);
        }
      }
    }
  }
    
  closedir(dirp);
  
  // Send progress update
  if (s->on_progress)
    if (progress_update(s->progress, dir))
      s->on_progress(s, s->progress, s->userdata);

  // process subdirs
  while (!SIMPLEQ_EMPTY(subdirq)) {
    struct dirq_entry *subdir_entry = SIMPLEQ_FIRST(subdirq);
    SIMPLEQ_REMOVE_HEAD(subdirq, entries);
    recurse_dir(s, subdir_entry->dir);
    free(subdir_entry);
  }
  
  free(subdirq);
  free(tmp_full_path);

out:
  free(dir);
}

void
ms_scan(MediaScan *s)
{
  int i;
  struct dirq *dir_head = (struct dirq *)s->_dirq;
  struct dirq_entry *dir_entry;
  struct fileq *file_head;
  struct fileq_entry *file_entry;
  char *tmp_full_path = malloc((size_t)PathMax);
  
  if (s->on_result == NULL) {
    LOG_ERROR("Result callback not set, aborting scan\n");
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
      *tmp_full_path = 0;
      strcat(tmp_full_path, dir_entry->dir);
      strcat(tmp_full_path, "/");
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
  
  free(tmp_full_path);
}

void
ms_scan_file(MediaScan *s, const char *full_path, enum media_type type)
{
  if (s->on_result == NULL) {
    LOG_ERROR("Result callback not set, aborting scan\n");
    return;
  }
  
  LOG_INFO("Scanning file %s\n", full_path);
  
  if (type == TYPE_UNKNOWN) {
    // auto-detect type
    type = _should_scan(s, full_path);
    if (!type) {
      if (s->on_error) {
        MediaScanError *e = error_create(full_path, MS_ERROR_TYPE_UNKNOWN, "Unrecognized file extension");
        s->on_error(s, e, s->userdata);
        error_destroy(e);
        return;
      }
    }
  }
  
  MediaScanResult *r = result_create(s);
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
}
