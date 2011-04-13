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

  for (i = 0; i < r->nthumbnails; i++) {
    int len;
    const uint8_t *data = ms_result_get_thumbnail(r, i, &len);
    if (len) {
      SV *thumb = newSVpvn(data, len); // XXX is there a way to just use the original pointer and avoid Perl copying data?
      //SvREADONLY_on(thumb); // Enable this if we can use the original pointer
      av_push(RETVAL, thumb);
    }
  }

}
OUTPUT:
  RETVAL
