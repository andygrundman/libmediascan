
#include <libmediascan.h>
#include <stdlib.h>
#include <string.h>

#include <gif_lib.h>

#include "common.h"
#include "buffer.h"
#include "image.h"
#include "image_gif.h"

typedef struct GIFData {
  Buffer *buf;
  FILE *fp;
  GifFileType *gif;
} GIFData;

static int InterlacedOffset[] = { 0, 4, 2, 1 };
static int InterlacedJumps[] = { 8, 8, 4, 2 };

static int image_gif_read_buf(GifFileType * gif, GifByteType * data, int len) {
  MediaScanImage *i = (MediaScanImage *)gif->UserData;
  GIFData *g = (GIFData *)i->_gif;

  //LOG_DEBUG("GIF read_buf wants %ld bytes, %d in buffer\n", len, buffer_len(g->buf));

  if (!buffer_check_load(g->buf, g->fp, len, BUF_SIZE))
    goto eof;

  memcpy(data, buffer_ptr(g->buf), len);
  buffer_consume(g->buf, len);
  goto ok;

eof:
  LOG_ERROR("Not enough GIF data (%s)\n", i->path);
  return 0;

ok:
  return len;
}

int image_gif_read_header(MediaScanImage *i, MediaScanResult *r, int is_gif89) {
  GIFData *g = malloc(sizeof(GIFData));
  i->_gif = (void *)g;
  LOG_MEM("new GIFData @ %p\n", i->_gif);

  g->buf = (Buffer *)r->_buf;
  g->fp = r->_fp;
  g->gif = DGifOpen(i, image_gif_read_buf, NULL);

  if (g->gif == NULL) {
    LOG_ERROR("Unable to open GIF file (%s)\n", i->path);
    image_gif_destroy(i);
    return 0;
  }

  i->width = g->gif->SWidth;
  i->height = g->gif->SHeight;
  r->mime_type = "image/gif";

  // Check for DLNA compatibility
  if (is_gif89 && i->width <= 1600 && i->height <= 1200) {
    r->dlna_profile = "GIF_LRG";
  }

  return 1;
}

int image_gif_load(MediaScanImage *i) {
  int x, y, ofs;
  GifRecordType RecordType;
  GifPixelType *line = NULL;
  int ExtFunction = 0;
  GifByteType *ExtData;
  SavedImage *sp;
  SavedImage temp_save;
  int BackGround = 0;
  int trans_index = 0;          // transparent index if any
  ColorMapObject *ColorMap;
  GifColorType *ColorMapEntry;
  int ret = 1;

  GIFData *g = (GIFData *)i->_gif;

  temp_save.ExtensionBlocks = NULL;
  temp_save.ExtensionBlockCount = 0;

  // XXX If reusing the object a second time, start over

  do {
    if (DGifGetRecordType(g->gif, &RecordType) == GIF_ERROR) {
      LOG_ERROR("Unable to read GIF file (%s)\n", i->path);
      goto err;
    }

    switch (RecordType) {
      case IMAGE_DESC_RECORD_TYPE:
        if (DGifGetImageDesc(g->gif) == GIF_ERROR) {
          LOG_ERROR("Unable to read GIF file (%s)\n", i->path);
          goto err;
        }

        sp = &g->gif->SavedImages[g->gif->ImageCount - 1];

        i->width = sp->ImageDesc.Width;
        i->height = sp->ImageDesc.Height;

        BackGround = g->gif->SBackGroundColor;  // XXX needed?
        ColorMap = g->gif->Image.ColorMap ? g->gif->Image.ColorMap : g->gif->SColorMap;

        if (ColorMap == NULL) {
          LOG_ERROR("Image::Scale GIF image has no colormap (%s)\n", i->path);
          goto err;
        }

        // Allocate storage for decompressed image
        if (!i->_pixbuf_size)
          image_alloc_pixbuf(i, i->width, i->height);

        line = (GifPixelType *) malloc(i->width * sizeof(GifPixelType));
        LOG_MEM("new GIF line buffer @ %p\n", line);

        if (g->gif->Image.Interlace) {
          int j;
          for (j = 0; j < 4; j++) {
            for (x = InterlacedOffset[j]; x < i->height; x += InterlacedJumps[j]) {
              ofs = x * i->width;
              if (DGifGetLine(g->gif, line, 0) != GIF_OK) {
                LOG_ERROR("Unable to read GIF file (%s)\n", i->path);
                goto err;
              }

              for (y = 0; y < i->width; y++) {
                ColorMapEntry = &ColorMap->Colors[line[y]];
                i->_pixbuf[ofs++] = COL_FULL(ColorMapEntry->Red,
                                             ColorMapEntry->Green,
                                             ColorMapEntry->Blue, trans_index == line[y] ? 0 : 255);
              }
            }
          }
        }
        else {
          ofs = 0;
          for (x = 0; x < i->height; x++) {
            if (DGifGetLine(g->gif, line, 0) != GIF_OK) {
              LOG_ERROR("Unable to read GIF file (%s)\n", i->path);
              goto err;
            }

            for (y = 0; y < i->width; y++) {
              ColorMapEntry = &ColorMap->Colors[line[y]];
              i->_pixbuf[ofs++] = COL_FULL(ColorMapEntry->Red,
                                           ColorMapEntry->Green, ColorMapEntry->Blue, trans_index == line[y] ? 0 : 255);
            }
          }
        }

        LOG_MEM("destroy GIF line buffer @ %p\n", line);
        free(line);
        break;

      case EXTENSION_RECORD_TYPE:
        if (DGifGetExtension(g->gif, &ExtFunction, &ExtData) == GIF_ERROR) {
          LOG_ERROR("Image::Scale unable to read GIF file (%s)\n", i->path);
          goto err;
        }

        if (ExtFunction == 0xF9) { // transparency info
          if (ExtData[1] & 1)
            trans_index = ExtData[4];
          else
            trans_index = -1;
          i->has_alpha = 1;
          LOG_DEBUG("GIF transparency index: %d\n", trans_index);
        }

        while (ExtData != NULL) {
          /* Create an extension block with our data */
          if (GifAddExtensionBlock(&g->gif->ExtensionBlockCount, &g->gif->ExtensionBlocks, ExtFunction, ExtData[0], &ExtData[1]) == GIF_ERROR) {
            LOG_ERROR("Unable to read GIF file (%s)\n", i->path);
            goto err;
          }

          if (DGifGetExtensionNext(g->gif, &ExtData) == GIF_ERROR) {
            LOG_ERROR("Unable to read GIF file (%s)\n", i->path);
            goto err;
          }

          ExtFunction = 0;
        }
        break;

      case TERMINATE_RECORD_TYPE:
      default:
        break;
    }
  } while (RecordType != TERMINATE_RECORD_TYPE);

  goto out;

err:
  image_gif_destroy(i);
  ret = 0;

out:
  if (temp_save.ExtensionBlocks)
    FreeExtension(&temp_save);

  return ret;
}

void image_gif_destroy(MediaScanImage *i) {
  if (i->_gif) {
    GIFData *g = (GIFData *)i->_gif;

    if (DGifCloseFile(g->gif, NULL) != GIF_OK) {
      LOG_ERROR("Unable to close GIF file (%s)\n", i->path);
    }

    LOG_MEM("destroy GIFData @ %p\n", i->_gif);
    free(i->_gif);
    i->_gif = NULL;
  }
}
