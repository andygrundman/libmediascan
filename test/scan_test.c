#include <libmediascan.h>

#include "tap.h"
#include "common.h"

#define TEST_COUNT 41

int
main(int argc, char *argv[])
{
  plan(TEST_COUNT);

  av_log_set_level(AV_LOG_ERROR);

  // Get path to this binary
  char *bin = _findbin(argv[0]);

  // Test scanning a single file
  {
    char *file = _abspath(bin, "../data/video/bars-vp6f-mp3.flv");
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
    ok(s->streams[1].type == TYPE_AUDIO, "scan_file s->streams[1].type ok");
    is(s->streams[1].codec_name, "mp3", "scan_file s->streams[1].codec_name ok");
    ok(s->streams[1].bitrate == 98, "scan_file s->streams[1].bitrate ok");
    ok(s->streams[1].samplerate == 44100, "scan_file s->streams[1].samplerate ok");
    ok(s->streams[1].channels == 2, "scan_file s->streams[1].channels ok");

    // AVMetadata, XXX use our own Metadata stuff
    int i = 0;
    AVMetadataTag *tag = NULL;
    while ((tag = av_metadata_get(s->metadata, "", tag, AV_METADATA_IGNORE_SUFFIX))) {
      switch (i) {
        case 0:
          is(tag->key, "duration", "scan_file s->metadata key %d ok", i);
          is(tag->value, "6", "scan_file s->metadata value %d ok", i);
          break;
        case 1:
          is(tag->key, "width", "scan_file s->metadata key %d ok", i);
          is(tag->value, "360", "scan_file s->metadata value %d ok", i);
          break;
        case 2:
          is(tag->key, "height", "scan_file s->metadata key %d ok", i);
          is(tag->value, "288", "scan_file s->metadata value %d ok", i);
          break;
        case 3:
          is(tag->key, "videodatarate", "scan_file s->metadata key %d ok", i);
          is(tag->value, "400", "scan_file s->metadata value %d ok", i);
          break;
        case 4:
          is(tag->key, "framerate", "scan_file s->metadata key %d ok", i);
          is(tag->value, "10", "scan_file s->metadata value %d ok", i);
          break;
        case 5:
          is(tag->key, "videocodecid", "scan_file s->metadata key %d ok", i);
          is(tag->value, "4", "scan_file s->metadata value %d ok", i);
          break;
        case 6:
          is(tag->key, "audiodatarate", "scan_file s->metadata key %d ok", i);
          is(tag->value, "96", "scan_file s->metadata value %d ok", i);
          break;
        case 7:
          is(tag->key, "audiodelay", "scan_file s->metadata key %d ok", i);
          is(tag->value, "0", "scan_file s->metadata value %d ok", i);
          break;
        case 8:
          is(tag->key, "audiocodecid", "scan_file s->metadata key %d ok", i);
          is(tag->value, "2", "scan_file s->metadata value %d ok", i);
          break;
        case 9:
          is(tag->key, "canSeekToEnd", "scan_file s->metadata key %d ok", i);
          is(tag->value, "true", "scan_file s->metadata value %d ok", i);
          break;
        default:
          fail("Invalid metadata");
          break;
      }
      i++;
    }

    mediascan_free_ScanData(s);
    free(file);
  }

  // Non-media file extension
  {
    ScanData s = mediascan_scan_file(bin, 0);
    ok(s == NULL, "scan_file on non-media file ok");
  }

  // Media extension but corrupt data
  {
    char *file = _abspath(bin, "../data/video/corrupt.mp4");
    ScanData s = mediascan_scan_file(file, 0);

    ok(s->error == SCAN_FILE_OPEN, "scan_file on corrupt mp4 file ok");

    mediascan_free_ScanData(s);
    free(file);
  }

  free(bin);

  return exit_status();
}
