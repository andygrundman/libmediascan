#include "EXTERN.h"
#include "perl.h"
#include "XSUB.h"

#include "ppport.h"
#include "xs_object_magic.h"

#include <libmediascan.h>

// Include the XS::Object::Magic code inline to get
// around some problems on Windows
#include "Magic.c"

// Enable for debug output
#define MEDIA_SCAN_DEBUG

#ifdef MEDIA_SCAN_DEBUG
# define DEBUG_TRACE(...) PerlIO_printf(PerlIO_stderr(), __VA_ARGS__)
#else
# define DEBUG_TRACE(...)
#endif

#define my_hv_store(a,b,c)     hv_store(a,b,strlen(b),c,0)
#define my_hv_store_ent(a,b,c) hv_store_ent(a,b,c,0)
#define my_hv_fetch(a,b)       hv_fetch(a,b,strlen(b),0)
#define my_hv_exists(a,b)      hv_exists(a,b,strlen(b))
#define my_hv_exists_ent(a,b)  hv_exists_ent(a,b,0)
#define my_hv_delete(a,b)      hv_delete(a,b,strlen(b),0)

static void
_on_result(MediaScan *s, MediaScanResult *result, void *userdata)
{
  HV *selfh = (HV *)userdata;
  SV *obj = NULL;
  SV *callback = NULL;
  
  if (!my_hv_exists(selfh, "on_result"))
    return;
  
  callback = *(my_hv_fetch(selfh, "on_result"));
  obj = newRV_noinc(newSVpvn("", 0));
  
  switch (result->type) {
    case TYPE_VIDEO:
      sv_bless(obj, gv_stashpv("Media::Scan::Video", 0));
      break;
    
    case TYPE_AUDIO:
      sv_bless(obj, gv_stashpv("Media::Scan::Audio", 0));
      break;
    
    case TYPE_IMAGE:
      sv_bless(obj, gv_stashpv("Media::Scan::Image", 0));
      break;
    
    default:
      break;
  }
  
  xs_object_magic_attach_struct(aTHX_ SvRV(obj), (void *)result);
  
  {
    dSP;
    PUSHMARK(SP);
    XPUSHs(obj);
    PUTBACK;
    
    call_sv(callback, G_VOID | G_DISCARD | G_EVAL);
    
    SPAGAIN;
    if (SvTRUE(ERRSV)) {
      warn("Error in on_result callback (ignored): %s", SvPV_nolen(ERRSV));
      POPs;
    }
  }
}

static void
_on_error(MediaScan *s, MediaScanError *error, void *userdata)
{
  HV *selfh = (HV *)userdata;
  SV *obj = NULL;
  SV *callback = NULL;
  
  if (!my_hv_exists(selfh, "on_error"))
    return;
  
  callback = *(my_hv_fetch(selfh, "on_error"));
  obj = newRV_noinc(newSVpvn("", 0));
  sv_bless(obj, gv_stashpv("Media::Scan::Error", 0));
  xs_object_magic_attach_struct(aTHX_ SvRV(obj), (void *)error);
  
  {
    dSP;
    PUSHMARK(SP);
    XPUSHs(obj);
    PUTBACK;
    call_sv(callback, G_VOID | G_DISCARD | G_EVAL);
    
    SPAGAIN;
    if (SvTRUE(ERRSV)) {
      warn("Error in on_error callback (ignored): %s", SvPV_nolen(ERRSV));
      POPs;
    }
  }
}

static void
_on_progress(MediaScan *s, MediaScanProgress *progress, void *userdata)
{
  HV *selfh = (HV *)userdata;
  SV *obj = NULL;
  SV *callback = NULL;
  
  if (!my_hv_exists(selfh, "on_progress"))
    return;
  
  callback = *(my_hv_fetch(selfh, "on_progress"));
  obj = newRV_noinc(newSVpvn("", 0));
  sv_bless(obj, gv_stashpv("Media::Scan::Progress", 0));
  xs_object_magic_attach_struct(aTHX_ SvRV(obj), (void *)progress);
  
  {
    dSP;
    PUSHMARK(SP);
    XPUSHs(obj);
    PUTBACK;
    call_sv(callback, G_VOID | G_DISCARD | G_EVAL);
    
    SPAGAIN;
    if (SvTRUE(ERRSV)) {
      warn("Error in on_progress callback (ignored): %s", SvPV_nolen(ERRSV));
      POPs;
    }
  }
}

static void
_on_finish(MediaScan *s, void *userdata)
{
  HV *selfh = (HV *)userdata;
  SV *callback = NULL;

  if (!my_hv_exists(selfh, "on_finish"))
    return;

  callback = *(my_hv_fetch(selfh, "on_finish"));

  {
    dSP;
    call_sv(callback, G_VOID | G_DISCARD | G_EVAL);

    SPAGAIN;
    if (SvTRUE(ERRSV)) {
      warn("Error in on_finish callback (ignored): %s", SvPV_nolen(ERRSV));
      POPs;
    }
  }
}

MODULE = Media::Scan		PACKAGE = Media::Scan		

void
xs_new(SV *self)
CODE:
{
  MediaScan *s = ms_create();
  xs_object_magic_attach_struct(aTHX_ SvRV(self), s);
}

void
set_log_level(MediaScan *, int level)
CODE:
{
  ms_set_log_level(level);
}

void
set_progress_interval(MediaScan *s, int seconds)
CODE:
{
  ms_set_progress_interval(s, seconds);
}

void
xs_scan(SV *self)
CODE:
{
  int i;
  MediaScan *s = xs_object_magic_get_struct_rv(aTHX_ self);
  HV *selfh = (HV *)SvRV(self);
  AV *paths, *ignore, *thumbnails;
  int async;
  
  // Set log level
  ms_set_log_level( SvIV(*(my_hv_fetch(selfh, "loglevel"))) );
  
  // Set paths to scan
  paths = (AV *)SvRV(*(my_hv_fetch(selfh, "paths")));
  for (i = 0; i < av_len(paths) + 1; i++) {
    SV **path = av_fetch(paths, i, 0);
    if (path != NULL && SvPOK(*path))
      ms_add_path(s, SvPVX(*path));
  }
  
  // Set extensions to ignore
  ignore = (AV *)SvRV(*(my_hv_fetch(selfh, "ignore")));
  for (i = 0; i < av_len(ignore) + 1; i++) {
    SV **ext = av_fetch(ignore, i, 0);
    if (ext != NULL && SvPOK(*ext))
      ms_add_ignore_extension(s, SvPVX(*ext));
  }
  
  // Set thumbnail specs
  // Array of hashes: { format => 'AUTO|JPEG|PNG', width => 100, height => 100, keep_aspect => 1, bgcolor => 0xffffff, quality => 90 },
  thumbnails = (AV *)SvRV(*(my_hv_fetch(selfh, "thumbnails")));
  for (i = 0; i < av_len(thumbnails) + 1; i++) {
    SV **spec_sv = av_fetch(thumbnails, i, 0);
    if (spec_sv != NULL && SvROK(*spec_sv)) {
      HV *spec = (HV *)SvRV(*spec_sv);
      
      // Defaults
      enum thumb_format format = THUMB_AUTO;
      int width = 0;
      int height = 0;
      int keep_aspect = 1;
      uint32_t bgcolor = 0;
      int quality = 90;
      
      if (my_hv_exists(spec, "format")) {
        SV *f = *(my_hv_fetch(spec, "format"));
        if (SvPOK(f)) {
          const char *fs = SvPVX(f);
          format = !strcmp(fs, "JPEG") ? THUMB_JPEG : !strcmp(fs, "PNG") ? THUMB_PNG : THUMB_AUTO;
        }
      }
      if (my_hv_exists(spec, "width")) {
        SV *u = *(my_hv_fetch(spec, "width"));
        if (SvIOK(u))
          width = SvUV(u);
      }
      if (my_hv_exists(spec, "height")) {
        SV *u = *(my_hv_fetch(spec, "height"));
        if (SvIOK(u))
          height = SvUV(u);
      }
      if (my_hv_exists(spec, "keep_aspect")) {
        SV *u = *(my_hv_fetch(spec, "keep_aspect"));
        if (SvIOK(u))
          keep_aspect = SvUV(u) == 1 ? 1 : 0;
      }
      if (my_hv_exists(spec, "bgcolor")) {
        SV *u = *(my_hv_fetch(spec, "bgcolor"));
        if (SvIOK(u))
          bgcolor = SvUV(u);
      }
      if (my_hv_exists(spec, "quality")) {
        SV *u = *(my_hv_fetch(spec, "quality"));
        if (SvIOK(u))
          quality = SvUV(u);
      }
      
      ms_add_thumbnail_spec(s, format, width, height, keep_aspect, bgcolor, quality);
    }
  }
  
  // Set async or sync operation
  async = SvIV(*(my_hv_fetch(selfh, "async")));
  ms_set_async(s, async ? 1 : 0);
  
  // Set cachedir
  if (my_hv_exists(selfh, "cachedir")) {
    SV **cachedir = my_hv_fetch(selfh, "cachedir");
    if (cachedir != NULL && SvPOK(*cachedir))
      ms_set_cachedir(s, SvPVX(*cachedir));
  }

  // Set callbacks
  ms_set_result_callback(s, _on_result);
  ms_set_error_callback(s, _on_error);
  ms_set_progress_callback(s, _on_progress);
  ms_set_finish_callback(s, _on_finish);
  
  ms_set_userdata(s, (void *)selfh);
  
  ms_scan(s);
}

int
async_fd(MediaScan *s)
CODE:
{
  RETVAL = ms_async_fd(s);
}
OUTPUT:
  RETVAL

void
async_process(MediaScan *s)
CODE:
{
  ms_async_process(s);
}

void
DESTROY(MediaScan *s)
CODE:
{
  ms_destroy(s);
}

INCLUDE: xs/Error.xs
INCLUDE: xs/Progress.xs
INCLUDE: xs/Result.xs
INCLUDE: xs/Video.xs
INCLUDE: xs/Image.xs