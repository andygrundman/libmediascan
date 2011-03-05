#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/stat.h>

#include <libmediascan.h>
#include "common.h"
#include "result.h"

// Audio formats
#include "wav.h"

type_ext audio_types[] = {
/*
  {"mp4", {"mp4", "m4a", "m4b", "m4p", "m4v", "m4r", "k3g", "skm", "3gp", "3g2", "mov", 0}},
  {"aac", {"aac", 0}},
  {"mp3", {"mp3", "mp2", 0}},
  {"ogg", {"ogg", "oga", 0}},
  {"mpc", {"mpc", "mp+", "mpp", 0}},
  {"ape", {"ape", "apl", 0}},
  {"flc", {"flc", "flac", "fla", 0}},
  {"asf", {"wma", "asf", "wmv", 0}},
*/
  {"wav", {"wav", "aif", "aiff", 0}},
//  {"wvp", {"wv", 0}},
  {0, {0, 0}}
};

type_handler audio_handlers[] = {
/*
  { "mp4", get_mp4tags, 0, mp4_find_frame, mp4_find_frame_return_info },
  { "aac", get_aacinfo, 0, 0, 0 },
  { "mp3", get_mp3tags, get_mp3fileinfo, mp3_find_frame, 0 },
  { "ogg", get_ogg_metadata, 0, ogg_find_frame, 0 },
  { "mpc", get_ape_metadata, get_mpcfileinfo, 0, 0 },
  { "ape", get_ape_metadata, get_macfileinfo, 0, 0 },
  { "flc", get_flac_metadata, 0, flac_find_frame, 0 },
  { "asf", get_asf_metadata, 0, asf_find_frame, 0 },
  { "wav", wav_scan },
  { "wvp", get_ape_metadata, get_wavpack_info, 0 },
*/
  { NULL, 0 }
};

MediaScanResult *
result_create(void)
{
  MediaScanResult *r = (MediaScanResult *)calloc(sizeof(MediaScanResult), 1);
  if (r == NULL) {
    LOG_ERROR("Out of memory for new MediaScanResult object\n");
    return NULL;
  }
  
  LOG_LEVEL(9, "new MediaScanResult @ %p\n", r);
  
  r->type = TYPE_UNKNOWN;
  r->path = NULL;
  r->mime_type = NULL;
  r->dlna_profile = NULL;
  r->size = 0;
  r->mtime = 0;
  r->bitrate = 0;
  r->duration_ms = 0;
  
  r->error = NULL;
  
  r->_avf = NULL;
  r->_fp = NULL;
  
  return r;
}

void
result_destroy(MediaScanResult *r)
{
  LOG_LEVEL(9, "destroy MediaScanResult @ %p\n", r);
  
  free(r);
}

/*
static type_handler *
get_type_handler(char *ext, type_ext *types, type_handler *handlers)
{
  int typeindex = -1;
  int i, j;
  type_handler *hdl = NULL;
  
  for (i = 0; typeindex == -1 && types[i].type; i++) {
    for (j = 0; typeindex == -1 && types[i].ext[j]; j++) {
#ifdef _MSC_VER
      if (!stricmp(types[i].ext[j], ext)) {
#else
      if (!strcasecmp(types[i].ext[j], ext)) {
#endif
        typeindex = i;
        break;
      }
    }
  }
  
  LOG_DEBUG("typeindex: %d\n", typeindex);
    
  if (typeindex > -1) {
    for (hdl = handlers; hdl->type; ++hdl)
      if (!strcmp(hdl->type, types[typeindex].type))
        break;
  }
  
  if (hdl)
    LOG_DEBUG("type handler: %s\n", hdl->type);
  
  return hdl;
}

static void
set_size(ScanData s)
{
#ifdef _WIN32
  // Win32 doesn't work right with fstat
  fseek(s->fp, 0, SEEK_END);
  s->size = ftell(s->fp);
  fseek(s->fp, 0, SEEK_SET);
#else
  struct stat buf;

  if ( !fstat( fileno(s->fp), &buf ) ) {
    s->size = buf.st_size;
  }
#endif
}

// Scan a video file with libavformat
static void
scan_video(ScanData s)
{
  AVFormatContext *avf = NULL;
  AVInputFormat *iformat = NULL;

  if (s->flags & USE_EXTENSION) {
    // Set AVInputFormat based on file extension to avoid guessing
    while ((iformat = av_iformat_next(iformat))) {
      if ( av_match_ext(s->path, iformat->name) )
        break;

      if (iformat->extensions) {
        if ( av_match_ext(s->path, iformat->extensions) )
          break;
      }
    }

#ifdef DEBUG
    if (iformat)
      LOG_DEBUG("Forcing format: %s\n", iformat->name);
#endif
  }

  if ( av_open_input_file(&avf, s->path, iformat, 0, NULL) != 0 ) {
    s->error = SCAN_FILE_OPEN;
    goto out;
  }

  if ( av_find_stream_info(avf) < 0 ) {
    s->error = SCAN_FILE_READ_INFO;
    goto out;
  }

#ifdef DEBUG
  //dump_format(avf, 0, s->path, 0);
#endif

  s->_avf = avf;

  s->type_name   = avf->iformat->long_name;
  s->bitrate     = avf->bit_rate / 1000;
  s->duration_ms = avf->duration / 1000;
  s->metadata    = avf->metadata;
  s->nstreams    = avf->nb_streams;

  if (avf->nb_streams) {
    mediascan_add_StreamData(s, avf->nb_streams);

    int i;
    for (i = 0; i < s->nstreams; i++) {    
      AVStream *st = avf->streams[i];

      switch (st->codec->codec_type) {
        case AVMEDIA_TYPE_VIDEO:
          s->streams[i].type = TYPE_VIDEO;
          s->streams[i].bitrate = st->codec->bit_rate;
          s->streams[i].width = st->codec->width;
          s->streams[i].height = st->codec->height;

          if (st->avg_frame_rate.num && st->avg_frame_rate.den)
            s->streams[i].fps = st->avg_frame_rate.num / (double)st->avg_frame_rate.den;
          break;
        case AVMEDIA_TYPE_AUDIO:
        {
          int bps = av_get_bits_per_sample(st->codec->codec_id); // bps for PCM types

          s->streams[i].type = TYPE_AUDIO;
          s->streams[i].samplerate = st->codec->sample_rate;
          s->streams[i].channels = st->codec->channels;
          s->streams[i].bit_depth = 0; // XXX not supported by libavformat
          s->streams[i].bitrate = bps
            ? st->codec->sample_rate * st->codec->channels * bps
            : st->codec->bit_rate;
          break;
        }
        default:
          s->streams[i].type = TYPE_UNKNOWN;
          break;
      }

      if (s->streams[i].bitrate)
        s->streams[i].bitrate /= 1000;

      AVCodec *c = avcodec_find_decoder(st->codec->codec_id);  
      if (c) {
        s->streams[i].codec_name = c->name;
      }
      else if (st->codec->codec_name[0] != '\0') {
        s->streams[i].codec_name = st->codec->codec_name;
      }
      else {
        s->streams[i].codec_name = "Unknown";
      }
    }
  }

out:
  return;
}

static void
scan_audio(ScanData s)
{
  char *ext = strrchr(s->path, '.');
  if (ext == NULL)
    return;
  
  type_handler *hdl = get_type_handler(ext + 1, audio_types, audio_handlers);
  if (hdl == NULL)
    return;
  
  // Open the file unless we already have an open fd
  int opened = s->fp != NULL;
  if (!opened) {
    if ((s->fp = fopen(s->path, "rb")) == NULL) {
      LOG_WARN("Cannot open %s: %s\n", s->path, strerror(errno));
      return;
    }
  }

  set_size(s);

  hdl->scan(s);

  // Close the file if we opened it
  if (!opened) {
    fclose(s->fp);
    s->fp = NULL;
  }
  
  return;
}

static void
scan_image(ScanData s)
{
  // XXX
}

ScanData
mediascan_new_ScanData(const char *path, int flags, int type)
{
  ScanData s = (ScanData)calloc(sizeof(struct _ScanData), 1);
  if (s == NULL) {
    LOG_ERROR("Out of memory for new ScanData object\n");
    return NULL;
  }
  
  s->path = path;
  s->flags = flags;
  s->type = type;
  
  switch (type) {
    case TYPE_VIDEO:
      scan_video(s);
      break;
      
    case TYPE_AUDIO:
      scan_audio(s);
      break;
      
    case TYPE_IMAGE:
      scan_image(s);
      break;
  }
    
  return s;
}

void
mediascan_free_ScanData(ScanData s)
{
  if (s->streams)
    free(s->streams);
  
  if (s->_avf != NULL)
    av_close_input_file(s->_avf);
  
  free(s);
}

void
mediascan_add_StreamData(ScanData s, int nstreams)
{
  s->nstreams = nstreams;
  s->streams = (StreamData)calloc(sizeof(struct _StreamData), nstreams);
}

*/
