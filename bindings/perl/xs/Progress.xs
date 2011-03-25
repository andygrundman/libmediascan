MODULE = Media::Scan		PACKAGE = Media::Scan::Progress

SV *
phase(MediaScanProgress *p)
CODE:
{
  RETVAL = newSVpv(p->phase, 0);
}
OUTPUT:
  RETVAL

SV *
cur_item(MediaScanProgress *p)
CODE:
{
  RETVAL = newSVpv(p->cur_item, 0);
}
OUTPUT:
  RETVAL

int
dir_total(MediaScanProgress *p)
CODE:
{
  RETVAL = p->dir_total;
}
OUTPUT:
  RETVAL

int
dir_done(MediaScanProgress *p)
CODE:
{
  RETVAL = p->dir_done;
}
OUTPUT:
  RETVAL

int
file_total(MediaScanProgress *p)
CODE:
{
  RETVAL = p->file_total;
}
OUTPUT:
  RETVAL

int
file_done(MediaScanProgress *p)
CODE:
{
  RETVAL = p->file_done;
}
OUTPUT:
  RETVAL

int
eta(MediaScanProgress *p)
CODE:
{
  RETVAL = p->eta;
}
OUTPUT:
  RETVAL

SV *
rate(MediaScanProgress *p)
CODE:
{
  RETVAL = newSVnv(p->rate);
}
OUTPUT:
  RETVAL
