#include <libmediascan.h>

#include "tap.h"
#include "common.h"

#define TEST_COUNT 14
  
int
main(int argc, char *argv[])
{ 
  plan(TEST_COUNT);
  
  av_log_set_level(AV_LOG_ERROR);
  
  // Get path to this binary
  char *bin = _findbin(argv[0]);
  
  // Test scanning a single file
  {
    char *file = _abspath(bin, "../data/bars.flv");
    ScanData s = mediascan_scan_file(file, 0);
    
    ok(s->error == 0, "scan_file s->error ok");
    is(s->path, file, "scan_file s->path ok");
    ok(s->flags == 0, "scan_file s->flags ok");
    ok(s->type == TYPE_VIDEO, "scan_file s->type is TYPE_VIDEO");
    is(s->type_name, "FLV format", "scan_file s->type_name ok");
    ok(s->bitrate == 507, "scan_file s->bitrate ok");
    ok(s->duration_ms == 6000, "scan_file s->duration_ms ok");
    
    ok(s->nstreams == 2, "scan_file s->nstreams ok");
    ok(s->streams[0].type == TYPE_VIDEO, "scan_file s->streams[0].type ok");
    is(s->streams[0].codec_name, "vp6f", "scan_file s->streams[0].codec_name ok");
    ok(s->streams[0].bitrate == 409, "scan_file s->streams[0].bitrate ok");
    ok(s->streams[0].width == 360, "scan_file s->streams[0].width ok");
    ok(s->streams[0].height == 288, "scan_file s->streams[0].height ok");
    ok(s->streams[0].fps == 0, "scan_file s->streams[0].fps ok");
    
    // XXX AVMetadata
    
    free(file);
  }
  
  free(bin);
  
  return exit_status();
}
