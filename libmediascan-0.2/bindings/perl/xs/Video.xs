MODULE = Media::Scan		PACKAGE = Media::Scan::Video

SV *
codec(MediaScanResult *r)
CODE:
{
  RETVAL = newSVpv(r->video->codec, 0);
}
OUTPUT:
  RETVAL

int
width(MediaScanResult *r)
CODE:
{
  RETVAL = r->video->width;
}
OUTPUT:
  RETVAL


int
height(MediaScanResult *r)
CODE:
{
  RETVAL = r->video->height;
}
OUTPUT:
  RETVAL

SV *
fps(MediaScanResult *r)
CODE:
{
  RETVAL = newSVpvf("%.2f", r->video->fps);
}
OUTPUT:
  RETVAL
