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

  for (i = 0; i < r->nthumbnails; i++) {
    MediaScanImage *thumb = ms_result_get_thumbnail(r, i);
    uint32_t len;
    const uint8_t *data = ms_result_get_thumbnail_data(r, i, &len);
    if (len) {
      HV *thumbhv = newHV();

      my_hv_store(thumbhv, "codec", newSVpv(thumb->codec, 0));
      my_hv_store(thumbhv, "width", newSVuv(thumb->width));
      my_hv_store(thumbhv, "height", newSVuv(thumb->height));
      my_hv_store(thumbhv, "data", newSVpvn(data, len));

      av_push(RETVAL, newRV_noinc((SV *)thumbhv));
    }
  }
}
OUTPUT:
  RETVAL
