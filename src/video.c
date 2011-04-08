


#include <libmediascan.h>

#ifdef WIN32
#include "mediascan_win32.h"
#endif


#ifdef _MSC_VER 
#pragma warning( disable: 4244 ) 
#endif

#include <libavformat/avformat.h>

#ifdef _MSC_VER
#pragma warning( default: 4244 )
#endif


#include "common.h"
#include "image.h"
#include "video.h"
#include "error.h"
#include "util.h"
#include "libdlna/profiles.h"


MediaScanVideo *
video_create(void)
{
  MediaScanVideo *v = (MediaScanVideo *)calloc(sizeof(MediaScanVideo), 1);
  if (v == NULL) {
	ms_errno = MSENO_MEMERROR;
    FATAL("Out of memory for new MediaScanVideo object\n");
    return NULL;
  }
  
  LOG_MEM("new MediaScanVideo @ %p\n", v);  
  return v;
}

MediaScanImage *
video_create_image_from_frame(MediaScanVideo *v, MediaScanResult *r)
{
  MediaScanImage *i = image_create();
  AVFormatContext *avf = (AVFormatContext *)r->_avf;
  av_codecs_t *codecs = (av_codecs_t *)v->_codecs;
  AVCodec *codec = (AVCodec *)v->_avc;
  AVFrame *picture = NULL;
  AVPacket packet;
  int got_picture;
  
  if ( (avcodec_open(codecs->vc, codec)) < 0 ) {
    LOG_ERROR("Couldn't open video codec %s for thumbnail creation\n", codec->name);
    goto err;
  }
  
  LOG_DEBUG("pix_fmt %d\n", codecs->vc->pix_fmt);
  
  picture = avcodec_alloc_frame();
  if (!picture) {
    LOG_ERROR("Couldn't allocate a video frame\n");
    goto err;
  }
  
  av_init_packet(&packet);
  
  i->path = v->path;
  i->width = v->width;
  i->height = v->height;
  
  // Allocate space for the image
  image_alloc_pixbuf(i, i->width, i->height);
    
  for (;;) {  
    if ( (av_read_frame(avf, &packet)) < 0 ) {
      LOG_ERROR("Couldn't read video frame\n");
      goto err;
    }
    
    // Skip frame if it's not from the video stream
    if (packet.stream_index != codecs->vsid)
      continue;
    
    // Skip non-key-frames
    if ( !(packet.flags & PKT_FLAG_KEY) )
      continue;
  
    LOG_DEBUG("Video packet: pos %lld size %d, stream_index %d, duration %d\n", packet.pos, packet.size, packet.stream_index, packet.duration);
  
    if ( (avcodec_decode_video2(codecs->vc, picture, &got_picture, &packet)) < 0 ) {
      LOG_ERROR("Error decoding video frame\n");
      goto err;
    }
    
    if (got_picture) {
      LOG_DEBUG("Video frame: key_frame: %d, pict_type: %d\n", picture->key_frame, picture->pict_type);
      
      // XXX use swscale to convert from source format to RGB24 in our buffer with no resizing
      
      break;
    }
  }
  
err:
  image_destroy(i);
  i = NULL;
  
out:
  av_free_packet(&packet);
  
  if (picture)
    av_free(picture);
  
  avcodec_close(codecs->vc);
  exit(0);
  
  return i;
}

void
video_add_thumbnail(MediaScanVideo *v, MediaScanImage *thumb)
{
  if (v->nthumbnails < MAX_THUMBS)
    v->thumbnails[ v->nthumbnails++ ] = thumb;
}

void
video_destroy(MediaScanVideo *v)
{
  int x;
  
  // free loaded frame if any
  video_unload_frame(v);
  
  // free thumbnails
  for (x = 0; x < v->nthumbnails; x++)
    image_destroy(v->thumbnails[x]);
  
  LOG_MEM("destroy MediaScanVideo @ %p\n", v);
  free(v);
}