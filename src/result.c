
#include <fcntl.h>
#include <string.h>

#ifndef WIN32
#include <strings.h>
#else
#include "win32config.h"
#endif

// If we are on MSVC, disable some stupid MSVC warnings
#ifdef _MSC_VER
#pragma warning( disable: 4996 )
#pragma warning( disable: 4244 )
#endif

#include <libavformat/avformat.h>
#include <libavutil/dict.h>

#ifdef _MSC_VER
#pragma warning( default: 4244 )
#endif

#include <libmediascan.h>



#include "common.h"
#include "buffer.h"
#include "result.h"
#include "error.h"
#include "video.h"
#include "audio.h"
#include "image.h"
#include "thumb.h"
#include "util.h"
#include "mediascan.h"
#include "tag.h"

// DLNA support
#include "libdlna/dlna.h"
#include "libdlna/profiles.h"

// Audio formats
//#include "wav.h"

const char CODEC_MP1[] = "mp1";

// *INDENT-OFF*
static type_ext audio_types[] = {
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

static type_handler audio_handlers[] = {
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
  {NULL, 0}
};

// MIME type extension mappings
static const struct {
  const char *extensions;
  const char *mime_type;
} mime_extension_mapping[] = {
//There is no IETF endorsed MIME type for Matroska files. But you can use the ones we have defined on our web server :
//    * .mka : Matroska audio audio/x-matroska
//    * .mkv : Matroska video video/x-matroska
//    * .mk3d : Matroska 3D video video/x-matroska-3d
	{ "mka",											"audio/x-matroska" },
	{ "mkv",											"video/x-matroska" },

// http://wiki.xiph.org/MIME_Types_and_File_Extensions
// The following MIME types are now officially registered with IANA and specified with the IETF as RFC 5334
// http://tools.ietf.org/html/rfc5215
	{ "flac",											"audio/flac" },
	{ "oga,ogg,spx",									"audio/ogg" },
	{ "ogv",											"video/ogg" },

	{ "mov",											"video/x-quicktime" },

// http://tools.ietf.org/html/rfc2046
  { "mpg,mpeg,mp1,mp2,mlv,mpv", "video/mpeg" },
  { "mp3,mla,m2a,mpa",          "audio/mpeg" },

// http://support.microsoft.com/kb/288102
  { "asf,asx",									"video/x-ms-asf" },
  { "wma",											"audio/x-ms-wma" },
  { "wax",											"audio/x-ms-wax" },
  { "wmv",											"video/x-ms-wmv" },
  { "wvx",											"video/x-ms-wvx" },
  { "wm",												"video/x-ms-wm" },
  { "wmx",											"video/x-ms-wmx" },

// http://tools.ietf.org/html/rfc3003
  { "mp3,mpa",									"audio/mpeg" },

// http://tools.ietf.org/html/rfc2361
  { "wav",											"audio/vnd.wave" },
	
// http://real.custhelp.com/cgi-bin/real.cfg/php/enduser/std_adp.php?p_faqid=2559&p_created=&p_sid=uz4Tpoti&p_lva=1085179956&p_sp=2559&p_li=cF9zcmNoPTEmcF9zb3J0X2J5PSZwX2dyaWRzb3J0PSZwX3Jvd19jbnQ9MSZwX3Byb2RzPTMsMTEmcF9jYXRzPSZwX3B2PTIuMTEmcF9jdj0mcF9zZWFyY2hfdHlwZT1hbnN3ZXJzLmFfaWQmcF9wYWdlPTEmcF9zZWFyY2hfdGV4dD0yNTU5cF9zcmNoPTEmcF9zb3J0X2J5PSZwX2dyaWRzb3J0PSZwX3Jvd19jbnQ9MyZwX3Byb2RzPTMsMTEmcF9jYXRzPSZwX3B2PTIuMTEmcF9jdj0mcF9zZWFyY2hfdHlwZT1hbnN3ZXJzLnNlYXJjaF9ubCZwX3BhZ2U9MSZwX3NlYXJjaF90ZXh0PU1JTUU*&p_prod_lvl1=3&p_prod_lvl2=11&tabName=tab0&p_topview=1
  { "ra,ram",										"audio/vnd.rn-realaudio" },

// http://en.wikipedia.org/wiki/WebM
  { "webm",											"audio/webm" },

// http://www.iana.org/assignments/media-types/video/quicktime
  { "qt",												"video/quicktime" },

// http://tools.ietf.org/html/rfc4337
  { "mp4,m4p,m4b,m4r,m4v",			"video/mp4" },
  { "m4a",											"audio/mp4" },

// http://tools.ietf.org/html/rfc2361
// Note: This one is kind of complicated as there are many different kinds of AVI formats. 
// Instead of just specifying video/vnd.avi and trying to figure out the codec, we are just
// going to specify video/avi, which isn't really what is specified by IETF but is much
// more straightforward. Other options would be: video/msvideo, video/x-msvideo
  { "avi",											"video/avi"    },

	// http://tools.ietf.org/html/rfc2045
  { "gif",											"image/gif"    },

	// http://tools.ietf.org/html/rfc2045
  { "jpg,jpeg,jpe",									"image/jpeg"    },

	// http://tools.ietf.org/html/rfc2083
  { "png",											"image/png"    },

	// http://tools.ietf.org/html/rfc3302
  { "tiff,tif",									"image/tiff"   },

	// No offical ruling for this mimetype however this is the unoffical one per
	// http://en.wikipedia.org/wiki/BMP_file_format
  { "bmp",											"image/x-ms-bmp"   },

  { NULL, 0 }
};
// *INDENT-ON*

///-------------------------------------------------------------------------------------------------
///  Try to find a matching DLNA profile, this is OK if it fails
///
/// @author Andy Grundman
/// @date 03/24/2011
///
/// @param [in,out] r    If non-null, the.
/// @param [in,out] codecs If non-null, the codecs.
///-------------------------------------------------------------------------------------------------

static void scan_dlna_profile(MediaScanResult *r, av_codecs_t *codecs) {
  dlna_registered_profile_t *p;
  dlna_profile_t *profile = NULL;
  dlna_container_type_t st;
  AVFormatContext *avf = (AVFormatContext *)r->_avf;
  dlna_t *dlna = (dlna_t *)((MediaScan *)r->_scan)->_dlna;

  st = stream_get_container(avf);

  p = dlna->first_profile;
  while (p) {
    dlna_profile_t *prof = NULL;

    if (r->flags & MS_USE_EXTENSION) {
      if (p->extensions) {
        /* check for valid file extension */
        if (!match_file_extension(r->path, p->extensions)) {
          p = p->next;
          continue;
        }
      }
    }

    prof = p->probe(avf, st, codecs);
    if (prof) {
      profile = prof;
      profile->class = p->class;
      break;
    }
    p = p->next;
  }

  if (profile) {
    r->mime_type = profile->mime;
    r->dlna_profile = profile->id;
  }
}                               /* scan_dlna_profile() */

///-------------------------------------------------------------------------------------------------
///  Ensures the object is open
///
/// @author Andy Grundman
/// @date 03/24/2011
///
/// @param [in,out] r If non-null, the.
///
/// @return .
///-------------------------------------------------------------------------------------------------

static int ensure_opened(MediaScanResult *r) {
  // Open the file unless we already have an open fd
  if (!r->_fp) {
    if ((r->_fp = fopen(r->path, "rb")) == NULL) {
      LOG_WARN("Cannot open %s: %s\n", r->path, strerror(errno));
      return 0;
    }
  }
  return 1;
}                               /* ensure_opened() */

static int ensure_opened_with_buf(MediaScanResult *r, int min_bytes) {
  Buffer *buf;

  if (!ensure_opened(r))
    return 0;

  buf = (Buffer *)malloc(sizeof(Buffer));
  r->_buf = (void *)buf;
  LOG_MEM("new result buffer @ %p\n", r->_buf);

  buffer_init(buf, BUF_SIZE);

  if (!buffer_check_load(buf, r->_fp, min_bytes, BUF_SIZE))
    return 0;

  return 1;
}

static const char *find_mime_type(const char *path) {

  int i = 0;

  for (i = 0; mime_extension_mapping[i].extensions; i++)
    if (match_file_extension(path, mime_extension_mapping[i].extensions)) {
      return mime_extension_mapping[i].mime_type;
    }

  return NULL;
}

///-------------------------------------------------------------------------------------------------
///  Scan a video file with libavformat
///
/// @author Andy Grundman
/// @date 03/24/2011
///
/// @param [in,out] r If non-null, the.
///
/// @return .
///-------------------------------------------------------------------------------------------------

static int scan_video(MediaScanResult *r) {
  AVFormatContext *avf = NULL;
  AVInputFormat *iformat = NULL;
  AVCodec *c = NULL;
  AVDictionaryEntry *tag = NULL;
  MediaScanVideo *v = NULL;
  MediaScanAudio *a = NULL;
  MediaScan *s = NULL;
  av_codecs_t *codecs = NULL;
  int AVError = 0;
  int ret = 1;

  if (r->flags & MS_USE_EXTENSION) {
    // Set AVInputFormat based on file extension to avoid guessing
    while ((iformat = av_iformat_next(iformat))) {
      if (av_match_ext(r->path, iformat->name))
        break;

      if (iformat->extensions) {
        if (av_match_ext(r->path, iformat->extensions))
          break;
      }
    }

    if (iformat)
      LOG_INFO("Forcing format: %s\n", iformat->name);
  }

  if ((AVError = av_open_input_file(&avf, r->path, iformat, 0, NULL)) != 0) {
    r->error = error_create(r->path, MS_ERROR_FILE, "[libavformat] Unable to open file for reading");
    r->error->averror = AVError;
    ret = 0;
    goto out;
  }

  r->_avf = (void *)avf;

  if ((AVError = av_find_stream_info(avf)) < 0) {
    r->error = error_create(r->path, MS_ERROR_READ, "[libavformat] Unable to find stream info");
    r->error->averror = AVError;
    ret = 0;
    goto out;
  }

  // Use libdlna's handy codecs struct
  codecs = av_profile_get_codecs(avf);
  if (!codecs) {
    r->error = error_create(r->path, MS_ERROR_READ, "Unable to determine audio/video codecs");
    ret = 0;
    goto out;
  }

  if (stream_ctx_is_audio(codecs)) {
    // XXX some extensions (e.g. mp4) can be either video or audio,
    // if after checking we don't find a video stream, we need to
    // send the result through the audio path
    r->error =
      error_create(r->path, MS_ERROR_READ, "Skipping audio file passed to scan_video. (This will be fixed later.)");
    ret = 0;
    goto out;
  }

  scan_dlna_profile(r, codecs);

  // If scanning for a DLNA profile did not find a mimetype
  // then guess one based on the file extension
  if (!r->mime_type) {
    r->mime_type = find_mime_type(r->path);
  }


  r->bitrate = avf->bit_rate;
  r->duration_ms = (int)(avf->duration / 1000);

  // Video-specific metadata
  v = r->video = video_create();
  v->path = r->path;

  c = avcodec_find_decoder(codecs->vc->codec_id);
  if (c) {
    v->codec = c->name;

    // Save structures needed for thumbnail generation
    v->_codecs = (void *)codecs;
    v->_avc = (void *)c;
  }
  else if (codecs->vc->codec_name[0] != '\0') {
    v->codec = codecs->vc->codec_name;
  }
  else {
    char codec_tag_string[128];

    // Check for DRM files and ignore them
    av_get_codec_tag_string(codec_tag_string, sizeof(codec_tag_string), codecs->vc->codec_tag);
    if (!strcmp("drmi", codec_tag_string)) {
      r->error = error_create(r->path, MS_ERROR_READ, "Skipping DRM-protected video file");
      ret = 0;
      goto out;
    }

    v->codec = "Unknown";
  }

  v->width = codecs->vc->width;
  v->height = codecs->vc->height;
  v->fps = av_q2d(codecs->vs->avg_frame_rate);

  // Audio metadata from the primary audio stream
  if (codecs->ac) {
    AVCodec *ac = NULL;

    a = r->audio = audio_create();

    ac = avcodec_find_decoder(codecs->ac->codec_id);
    if (ac) {
      a->codec = ac->name;
    }
    else if (codecs->ac->codec_name[0] != '\0') {
      a->codec = codecs->ac->codec_name;
    }
    // Special case for handling MP1 audio streams which FFMPEG can't identify a codec for
    else if (codecs->ac->codec_id == CODEC_ID_MP1) {
      a->codec = CODEC_MP1;
    }
    else {
      a->codec = "Unknown";
    }

    a->bitrate = codecs->ac->bit_rate;
    a->samplerate = codecs->ac->sample_rate;
    a->channels = codecs->ac->channels;
  }

  // XXX additional streams(?)

  // Process any video tags
  while ((tag = av_dict_get(avf->metadata, "", tag, AV_DICT_IGNORE_SUFFIX))) {
    if (!r->_tag)
      result_create_tag(r, "Video");

    LOG_DEBUG("Saving video tag: %s: %s\n", tag->key, tag->value);

    tag_add_item(r->_tag, tag->key, tag->value);
  }

  // Create thumbnail(s) if we found a valid video decoder above
  s = (MediaScan *)r->_scan;
  if (s->nthumbspecs && v->_avc) {
    int x;
    MediaScanImage *i = video_create_image_from_frame(v, r);  // Decode and load a frame of video we'll use for the thumbnail
    if (i) {
      // XXX sort from biggest to smallest, resize in series

      for (x = 0; x < s->nthumbspecs; x++) {
        MediaScanImage *thumb = thumb_create_from_image(i, s->thumbspecs[x]);
        if (thumb)
          result_add_thumbnail(r, thumb);
      }

      image_destroy(i);
    }
  }

out:
  if (codecs) {
    LOG_MEM("destroy video codecs @ %p\n", codecs);
    free(codecs);
  }

  return ret;
}                               /* scan_video() */

static int scan_image(MediaScanResult *r) {
  int ret = 1;
  MediaScanImage *i = NULL;
  MediaScan *s;
  int w, h;

  // Open the file and read in a buffer of at least 8 bytes
  if (!ensure_opened_with_buf(r, 8)) {
    r->error = error_create(r->path, MS_ERROR_FILE, "Unable to open file for reading");
    ret = 0;
    goto out;
  }

  i = r->image = image_create();
  i->path = r->path;

  if (!image_read_header(i, r)) {
    r->error = error_create(r->path, MS_ERROR_READ, "Invalid or corrupt image file");
    ret = 0;
    goto out;
  }

  // Guess a mime type based on the file extension
  if (!r->mime_type) {
    r->mime_type = find_mime_type(r->path);
  }

  // Save original image dimensions as thumbnail creation may alter it (e.g. for JPEG scaling)
  w = i->width;
  h = i->height;

  // Create thumbnail(s)
  s = (MediaScan *)r->_scan;
  if (s->nthumbspecs) {
    int x;

    // XXX sort from biggest to smallest, resize in series
    // XXX Move image_load call here

    for (x = 0; x < s->nthumbspecs; x++) {
      MediaScanImage *thumb = thumb_create_from_image(i, s->thumbspecs[x]);
      if (thumb)
        result_add_thumbnail(r, thumb);
    }
  }

  // Restore dimensions
  i->width = w;
  i->height = h;

out:
  // Close the file here, to avoid stacking up a bunch of open files in async mode
  if (r->_fp) {
    fclose(r->_fp);
    r->_fp = NULL;
  }

  return ret;
}

///-------------------------------------------------------------------------------------------------
///  Result create.
///
/// @author Andy Grundman
/// @date 03/24/2011
///
/// @param [in,out] s If non-null, the.
///
/// @return null if it fails, else.
///-------------------------------------------------------------------------------------------------

MediaScanResult *result_create(MediaScan *s) {
  MediaScanResult *r = (MediaScanResult *)calloc(sizeof(MediaScanResult), 1);
  if (r == NULL) {
    ms_errno = MSENO_MEMERROR;
    FATAL("Out of memory for new MediaScanResult object\n");
    return NULL;
  }

  LOG_MEM("new MediaScanResult @ %p\n", r);

  r->type = TYPE_UNKNOWN;
  r->flags = s->flags;

  r->_scan = s;
  r->_avf = NULL;
  r->_fp = NULL;

  r->hash = 0;

  return r;
}                               /* result_create() */

///-------------------------------------------------------------------------------------------------
///  Result scan.
///
/// @author Andy Grundman
/// @date 03/24/2011
///
/// @param [in,out] r If non-null, the.
///
/// @return .
///-------------------------------------------------------------------------------------------------

int result_scan(MediaScanResult *r) {
  if (!r->type || !r->path) {
    r->error = error_create("", MS_ERROR_TYPE_INVALID_PARAMS, "Invalid parameters passed to result_scan()");
    return FALSE;
  }

  switch (r->type) {
    case TYPE_VIDEO:
      return scan_video(r);
      break;

    case TYPE_IMAGE:
      return scan_image(r);
      break;

    default:
      break;
  }

  return FALSE;
}                               /* result_scan() */

void result_create_tag(MediaScanResult *r, const char *type) {
  r->_tag = tag_create(type);
}

///-------------------------------------------------------------------------------------------------
///  Result destroy.
///
/// @author Andy Grundman
/// @date 03/24/2011
///
/// @param [in,out] r If non-null, the.
///-------------------------------------------------------------------------------------------------

void result_destroy(MediaScanResult *r) {
  int i;

  if (r->path)
    free(r->path);

  if (r->error)
    error_destroy(r->error);

  if (r->video)
    video_destroy(r->video);

  if (r->audio)
    audio_destroy(r->audio);

  if (r->image)
    image_destroy(r->image);

  if (r->_tag)
    tag_destroy(r->_tag);

  if (r->_avf) {
    av_close_input_file(r->_avf);
  }

  if (r->_fp)
    fclose(r->_fp);

  if (r->_buf) {
    buffer_free((Buffer *)r->_buf);
    LOG_MEM("destroy result buffer @ %p\n", r->_buf);
    free(r->_buf);
  }

  // free thumbnails
  for (i = 0; i < r->nthumbnails; i++)
    image_destroy(r->_thumbs[i]);

  LOG_MEM("destroy MediaScanResult @ %p\n", r);
  free(r);
}                               /* result_destroy() */

///-------------------------------------------------------------------------------------------------
///  Dump result to the log output
///
/// @author Andy Grundman
/// @date 03/24/2011
///
/// @param [in,out] r If non-null, the.
///-------------------------------------------------------------------------------------------------

void ms_dump_result(MediaScanResult *r) {
  int i;

  LOG_OUTPUT("%s\n", r->path);
  LOG_OUTPUT("  MIME type:    %s\n", r->mime_type);
  LOG_OUTPUT("  DLNA profile: %s\n", r->dlna_profile);
  LOG_OUTPUT("  File size:    %llu\n", r->size);
  LOG_OUTPUT("  Modified:     %d\n", r->mtime);
  if (r->bitrate)
    LOG_OUTPUT("  Bitrate:      %d bps\n", r->bitrate);
  if (r->duration_ms)
    LOG_OUTPUT("  Duration:     %d ms\n", r->duration_ms);

  switch (r->type) {
    case TYPE_VIDEO:
      LOG_OUTPUT("  Video:        %s\n", r->video->codec);
      LOG_OUTPUT("    Dimensions: %d x %d\n", r->video->width, r->video->height);
      LOG_OUTPUT("    Framerate:  %.2f\n", r->video->fps);
      if (r->audio) {
        LOG_OUTPUT("  Audio:        %s\n", r->audio->codec);
        LOG_OUTPUT("    Bitrate:    %d bps\n", r->audio->bitrate);
        LOG_OUTPUT("    Samplerate: %d kHz\n", r->audio->samplerate);
        LOG_OUTPUT("    Channels:   %d\n", r->audio->channels);
      }
      LOG_OUTPUT("  FFmpeg details:\n");
      av_dump_format(r->_avf, 0, r->path, 0);
      break;

    case TYPE_IMAGE:
      LOG_OUTPUT("  Image:        %s\n", r->image->codec);
      LOG_OUTPUT("    Dimensions: %d x %d\n", r->image->width, r->image->height);
      break;

    case TYPE_AUDIO:
      LOG_OUTPUT("  Audio:        %s\n", r->audio->codec);
      LOG_OUTPUT("    Bitrate:    %d bps\n", r->audio->bitrate);
      LOG_OUTPUT("    Samplerate: %d kHz\n", r->audio->samplerate);
      LOG_OUTPUT("    Channels:   %d\n", r->audio->channels);
      break;

    default:
      LOG_OUTPUT("  Type: Unknown\n");
      break;
  }

  for (i = 0; i < r->nthumbnails; i++) {
    Buffer *dbuf;
    MediaScanImage *thumb = r->_thumbs[i];
    if (!thumb->_dbuf)
      continue;
    dbuf = (Buffer *)thumb->_dbuf;
    LOG_OUTPUT("    Thumbnail:  %d x %d %s (%d bytes)\n", thumb->width, thumb->height, thumb->codec, buffer_len(dbuf));

#ifdef DUMP_THUMBNAILS
    {
      static int tcount = 1;
      FILE *tfp;
      char file[MAX_PATH];
      if (!strcmp("JPEG", thumb->codec))
        sprintf(file, "thumb%d.jpg", tcount++);
      else
        sprintf(file, "thumb%d.png", tcount++);
      tfp = fopen(file, "wb");
      fwrite(buffer_ptr(dbuf), 1, buffer_len(dbuf), tfp);
      fclose(tfp);
      LOG_OUTPUT("      Saved to: %s\n", file);
    }
#endif
  }
}                               /* ms_dump_result() */

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

  hdl->scan(s);

  // Close the file if we opened it
  if (!opened) {
    fclose(s->fp);
    s->fp = NULL;
  }
  
  return;
}
*/
