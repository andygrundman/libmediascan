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
