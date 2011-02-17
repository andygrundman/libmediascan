#include "EXTERN.h"
#include "perl.h"
#include "XSUB.h"

#include "ppport.h"
#include "xs_object_magic.h"

#include <libmediascan.h>

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
_on_result(MediaScan *s, MediaScanResult *result)
{

}

static void
_on_error(MediaScan *s, MediaScanError *error)
{

}

static void
_on_progress(MediaScan *s, MediaScanProgress *progress)
{

}

MODULE = Media::Scan		PACKAGE = Media::Scan		

void
xs_new(SV *self)
CODE:
{
  MediaScan *s = ms_create();
  xs_object_magic_attach_struct(aTHX_ SvRV(self), s);
  
  DEBUG_TRACE("new %p\n", s);
}

void
set_log_level(MediaScan *, int level)
CODE:
{
  ms_set_log_level(level);
}

void
xs_scan(SV *self)
CODE:
{
  int i;
  MediaScan *s = xs_object_magic_get_struct_rv(aTHX_ self);
  HV *selfh = (HV *)SvRV(self);
  
  // Set log level
  ms_set_log_level( SvIV(*(my_hv_fetch(selfh, "loglevel"))) );
  
  // Set paths to scan
  AV *paths = (AV *)SvRV(*(my_hv_fetch(selfh, "paths")));
  for (i = 0; i < av_len(paths) + 1; i++) {
    SV **path = av_fetch(paths, i, 0);
    if (path != NULL && SvPOK(*path))
      ms_add_path(s, SvPVX(*path));
  }
  
  // Set extensions to ignore
  AV *ignore = (AV *)SvRV(*(my_hv_fetch(selfh, "ignore")));
  for (i = 0; i < av_len(ignore) + 1; i++) {
    SV **ext = av_fetch(ignore, i, 0);
    if (ext != NULL && SvPOK(*ext))
      ms_add_ignore_extension(s, SvPVX(*ext));
  }
  
  // Set async or sync operation
  int async = SvIV(*(my_hv_fetch(selfh, "async")));
  ms_set_async(s, async ? 1 : 0);
  
  // Set callbacks
  ms_set_result_callback(s, _on_result);
  ms_set_error_callback(s, _on_error);
  ms_set_progress_callback(s, _on_progress);
  
  ms_scan(s);
}

void
DESTROY(MediaScan *s)
CODE:
{
  DEBUG_TRACE("DESTROY %p\n", s);
  
  ms_destroy(s);
}
