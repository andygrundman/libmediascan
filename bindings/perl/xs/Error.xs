MODULE = Media::Scan		PACKAGE = Media::Scan::Error

int
error_code(MediaScanError *e)
CODE:
{
  RETVAL = e->error_code;
}
OUTPUT:
  RETVAL

int
averror(MediaScanError *e)
CODE:
{
  RETVAL = e->averror;
}
OUTPUT:
  RETVAL

SV *
path(MediaScanError *e)
CODE:
{
  RETVAL = newSVpv(e->path, 0);
}
OUTPUT:
  RETVAL

SV *
error_string(MediaScanError *e)
CODE:
{
  RETVAL = newSVpv(e->error_string, 0);
}
OUTPUT:
  RETVAL
