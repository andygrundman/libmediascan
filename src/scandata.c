#include <libmediascan.h>
#include "common.h"
#include "scandata.h"

#include <string.h>
#include <strings.h>

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
  
  AVFormatContext *avf = NULL;
  AVInputFormat *iformat = NULL;
  
  if (flags & USE_EXTENSION) {
    // Set AVInputFormat based on file extension to avoid guessing
    while ((iformat = av_iformat_next(iformat))) {
      if ( av_match_ext(path, iformat->name) )
        break;

      if (iformat->extensions) {
        if ( av_match_ext(path, iformat->extensions) )
          break;
      }
    }
    
#if DEBUG
    if (iformat)
      LOG_DEBUG("Forcing format: %s\n", iformat->name);
#endif
  }
  
  if ( av_open_input_file(&avf, path, iformat, 0, NULL) != 0 ) {
    s->error = SCAN_FILE_OPEN;
    goto out;
  }
  
  if ( av_find_stream_info(avf) < 0 ) {
    s->error = SCAN_FILE_READ_INFO;
    goto out;
  }
  
#if DEBUG
  //dump_format(avf, 0, path, 0);
#endif
  
  s->_avf = avf;
  
  s->type_name   = avf->iformat->long_name;
  s->bitrate     = avf->bit_rate / 1000;
  s->duration_ms = avf->duration / 1000;
  s->metadata    = avf->metadata;
  s->nstreams    = avf->nb_streams;
  
  if (s->nstreams) {
    s->streams = (struct _StreamData *)calloc(sizeof(struct _StreamData), s->nstreams);
  
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
          else if (st->r_frame_rate.num && st->r_frame_rate.den)
            s->streams[i].fps = st->r_frame_rate.num / (double)st->r_frame_rate.den;
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
  return s;
}

void
mediascan_free_ScanData(ScanData s)
{
  if (s->nstreams)
    free(s->streams);
  
  if (s->_avf != NULL)
    av_close_input_file(s->_avf);
  
  free(s);
}
