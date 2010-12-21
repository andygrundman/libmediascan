#include <ctype.h>
#include <dirent.h>
#include <stdio.h>
#include <unistd.h>

#include <libmediascan.h>
#include "common.h"
#include "scandata.h"
#include "queue.h"

static int Initialized = 0;
static long PathMax = 0;

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
static const char VideoExts[] = ",asf,avi,divx,flv,m2t,m4v,mkv,mpg,mpeg,mp4,mts,m2ts,ts,vob,webm,wmv,xvid,";

#define REGISTER_DECODER(X,x) { \
          extern AVCodec x##_decoder; \
          avcodec_register(&x##_decoder); }
#define REGISTER_PARSER(X,x) { \
          extern AVCodecParser x##_parser; \
          av_register_codec_parser(&x##_parser); }

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
    extern AVInputFormat x##_demuxer; \
    av_register_input_format(&x##_demuxer); }
#define REGISTER_PROTOCOL(X,x) { \
    extern URLProtocol x##_protocol; \
    av_register_protocol2(&x##_protocol, sizeof(x##_protocol)); }

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
  //av_register_all();
  
  PathMax = pathconf(".", _PC_PATH_MAX); // 1024
  
  Initialized = 1;
}

static inline int
_is_media(const char *path)
{
  char *ext = strrchr(path, '.');
  if (ext != NULL) {
    // Copy the extension and lowercase it
    char extc[8];
    extc[0] = ',';
    strncpy(extc + 1, ext + 1, 7);
    
    char *p = &extc[1];
    while (*p != 0) {
      *p = tolower(*p);
      p++;
    }
    *p++ = ',';
    *p = 0;
    
    char *found = strstr(VideoExts, extc);
    return found == NULL ? 0 : TYPE_VIDEO;
  }
  return 0;
}

ScanData
mediascan_scan_file(const char *path, int flags)
{
  _init();
  
  int type = _is_media(path);
  if (type) {
    ScanData s = mediascan_new_ScanData(path, flags, type);
    return s;
  }
  
  return NULL;
}

void 
mediascan_scan_tree(const char *path, int flags, ScanDataCallback callback)
{
  char *dir;
  
  _init();
  
  if (path[0] != '/') { // XXX Win32
    // Get full path
    char *buf = (char *)malloc((size_t)PathMax);
    if (buf == NULL) {
      LOG_ERROR("Out of memory for directory scan\n");
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
  char *p = &dir[0];
  while (*p != 0) {
#ifdef _WIN32
    if (p[1] == 0 && (*p == '/' || *p == '\\'))
#else
    if (p[1] == 0 && *p == '/')
#endif
      *p = 0;
    p++;
  }
    
  LOG_DEBUG("scan_tree(%s)\n", dir);
  
  DIR *dirp;
  if ((dirp = opendir(dir)) == NULL) {
    LOG_ERROR("Unable to open directory %s: %s\n", dir, strerror(errno));
    goto out;
  }
  
  struct dirent *dp;
  struct dirq_entry {
    char *dir;
    TAILQ_ENTRY(dirq_entry) entries;
  };
  TAILQ_HEAD(, dirq_entry) dirq_head;
  TAILQ_INIT(&dirq_head);
  
  char *tmp = malloc((size_t)PathMax);
  
  while ((dp = readdir(dirp)) != NULL) {
    char *name = dp->d_name;
    
    // skip all dot files
    if (name[0] != '.') {
      // XXX some platforms may be missing d_type/DT_DIR
      if (dp->d_type == DT_DIR) {        
        struct dirq_entry *entry = malloc(sizeof(struct dirq_entry));
        entry->dir = strdup(dp->d_name);
        TAILQ_INSERT_TAIL(&dirq_head, entry, entries);
      }
      else {
        int type = _is_media(dp->d_name);
        
        if (type) {
          *tmp = 0;
          strcat(tmp, dir);
          strcat(tmp, "/");
          strcat(tmp, dp->d_name);
        
          LOG_DEBUG("%s\n", tmp);
        
          // Scan the file
          ScanData s = mediascan_new_ScanData(tmp, flags, type);
          if (s)
            callback(s);
          mediascan_free_ScanData(s);
        }
      }
    }
  }
  
  closedir(dirp);
  
  // process any subdirs
  if ( !TAILQ_EMPTY(&dirq_head) ) {
    struct dirq_entry *entry;
    
    TAILQ_FOREACH(entry, &dirq_head, entries) {
      *tmp = 0;
      strcat(tmp, dir);
      strcat(tmp, "/");
      strcat(tmp, entry->dir);
      
      mediascan_scan_tree(tmp, flags, callback);
      
      free(entry);
    }
  }
  
  free(tmp);

out:
  free(dir);  
}
