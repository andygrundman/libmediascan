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
total(MediaScanProgress *p)
CODE:
{
  RETVAL = p->total;
}
OUTPUT:
  RETVAL

int
done(MediaScanProgress *p)
CODE:
{
  RETVAL = p->done;
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

int
rate(MediaScanProgress *p)
CODE:
{
  RETVAL = p->rate;
}
OUTPUT:
  RETVAL
