MODULE = Media::Scan		PACKAGE = Media::Scan::Result

int
type(MediaScanResult *r)
CODE:
{
  RETVAL = r->type;
}
OUTPUT:
  RETVAL

SV *
path(MediaScanResult *r)
CODE:
{
  RETVAL = newSVpv(r->path, 0);
}
OUTPUT:
  RETVAL

SV *
mime_type(MediaScanResult *r)
CODE:
{
  RETVAL = newSVpv(r->mime_type, 0);
}
OUTPUT:
  RETVAL

SV *
dlna_profile(MediaScanResult *r)
CODE:
{
  RETVAL = newSVpv(r->dlna_profile, 0);
}
OUTPUT:
  RETVAL

int
size(MediaScanResult *r)
CODE:
{
  RETVAL = r->size;
}
OUTPUT:
  RETVAL

int
mtime(MediaScanResult *r)
CODE:
{
  RETVAL = r->mtime;
}
OUTPUT:
  RETVAL

int
bitrate(MediaScanResult *r)
CODE:
{
  RETVAL = r->bitrate;
}
OUTPUT:
  RETVAL

int
duration_ms(MediaScanResult *r)
CODE:
{
  RETVAL = r->duration_ms;
}
OUTPUT:
  RETVAL

SV *
hash(MediaScanResult *r)
CODE:
{
  RETVAL = newSVpvf("%08x", r->hash);
}
OUTPUT:
  RETVAL

AV *
thumbnails(MediaScanResult *r)
CODE:
{
  int i;
  RETVAL = newAV();
  
  // XXX refactor, return hashes
  switch (r->type) {
    case TYPE_IMAGE:
      for (i = 0; i < r->image->nthumbnails; i++) {
        int len;
        const uint8_t *data = ms_result_get_thumbnail(r, i, &len);
        if (len) {
          SV *thumb = newSVpvn(data, len); // XXX is there a way to just use the original pointer and avoid Perl copying data?
          SvREADONLY_on(thumb);
          av_push(RETVAL, thumb);
        }
      }
      break;
    
    case TYPE_VIDEO:
      for (i = 0; i < r->video->nthumbnails; i++) {
        int len;
        const uint8_t *data = ms_result_get_thumbnail(r, i, &len);
        if (len) {
          SV *thumb = newSVpvn(data, len);
          SvREADONLY_on(thumb);
          av_push(RETVAL, thumb);
        }
      }
      break;
    
    default:
      break;
  }
}
OUTPUT:
  RETVAL
