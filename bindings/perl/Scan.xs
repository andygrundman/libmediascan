#include "EXTERN.h"
#include "perl.h"
#include "XSUB.h"

#include "ppport.h"

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


MODULE = Media::Scan		PACKAGE = Media::Scan		

void
xs_new(SV *self)
PREINIT:
{
  MediaScan *scan;
}
CODE:
{
  scan = ms_create();
  xs_object_magic_attach_struct(aTHX_ SvRV(self), scan);
  
  DEBUG_TRACE("new %p\n", scan);
}

void
scan(SV *self)
CODE:
{
  MediaScan *scan = xs_object_magic_get_struct_rv(aTHX_ self);
  
  // Set paths to scan
  AV *paths = (AV *)SvRV(*(my_hv_fetch(self, "paths")));
  for (int i = 0; i < av_len(paths) + 1; i++) {
    SV *path = av_fetch(paths, i, 0);
    if (path != NULL && SvPOK(*path))
      ms_add_path(scan, SvPVX(*path));
  }
  
  // Set extensions to ignore
  AV *ignore = (AV *)SvRV(*(my_hv_fetch(self, "ignore")));
  for (int i = 0; i < av_len(ignore) + 1; i++) {
    SV *ext = av_fetch(ignore, i, 0);
    if (ext != NULL && SvPOK(*ext))
      ms_add_ignore_extension(scan, SvPVX(*ext));
  }
  
  // Set async or sync operation
  int async = SvIV(*(my_hv_fetch(self, "async")));
  ms_set_async(scan, async ? 1 : 0);
  
  // Set callbacks
  ms_set_result_callback(scan, _on_result);
  ms_set_error_callback(scan, _on_error);
  ms_set_progress_callback(scan, _on_progress);
  
  ms_scan(scan);
}

void
DESTROY(MediaScan *scan)
CODE:
{
  DEBUG_TRACE("DESTROY %p\n", scan);
  
  ms_destroy(scan);
}
