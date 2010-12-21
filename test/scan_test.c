#include <libmediascan.h>

static int
data_callback(ScanData s)
{
  if (s->error) {
    fprintf(stderr, "  Error: %d\n", s->error);
  }
  else {
    fprintf(stderr, "  Format: %s, %d kbps, %d ms\n", s->type_name, s->bitrate, s->duration_ms);
    
    int i;
    for (i = 0; i < s->nstreams; i++) {
      StreamData st = &s->streams[i];
      
      switch (st->type) {
        case TYPE_VIDEO:
          fprintf(stderr, "    Video: %s, %d kbps, %dx%d, %.2f fps\n",
            st->codec_name, st->bitrate, st->width, st->height, st->fps);
          break;
        case TYPE_AUDIO:
          fprintf(stderr, "    Audio: %s, %d kbps, %d Hz, %d channels, %d bps\n",
            st->codec_name, st->bitrate, st->samplerate, st->channels, st->bit_depth);
          break;
        // XXX subtitles?
        default:
          fprintf(stderr, "    Unknown\n");
          break;
      }
    }
  }
  return 1;
}
  
int
main(int argc, char *argv[])
{  
  //av_log_set_level(AV_LOG_DEBUG);
  
  mediascan_scan_tree(argv[1], /*USE_EXTENSION*/ 0, data_callback);
  
  return 0;
}
