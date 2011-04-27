#include <libmediascan.h>

#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "buffer.h"
#include "image.h"
#include "image_bmp.h"

typedef struct BMPData {
  int flipped;
  int bpp;
  int compression;
  int palette_colors[256];
  Buffer *buf;
  FILE *fp;
} BMPData;

// 16-bit color masks and shifts, default is 5-5-5
static uint32_t masks[3] = { 0x7c00, 0x3e0, 0x1f };
static uint32_t shifts[3] = { 10, 5, 0 };
static uint32_t ncolors[3] = { (1 << 5) - 1, (1 << 5) - 1, (1 << 5) - 1 };

int image_bmp_read_header(MediaScanImage *i, MediaScanResult *r) {
  int offset, palette_colors;
  BMPData *bmp = (BMPData *) calloc(sizeof(BMPData), 1);

  bmp->buf = (Buffer *)r->_buf;
  bmp->fp = r->_fp;
  i->_bmp = (void *)bmp;
  LOG_MEM("new BMPData @ %p\n", i->_bmp);

  buffer_consume(bmp->buf, 10);

  offset = buffer_get_int_le(bmp->buf);
  buffer_consume(bmp->buf, 4);
  i->width = buffer_get_int_le(bmp->buf);
  i->height = buffer_get_int_le(bmp->buf);
  buffer_consume(bmp->buf, 2);
  bmp->bpp = buffer_get_short_le(bmp->buf);
  bmp->compression = buffer_get_int_le(bmp->buf);

  LOG_DEBUG("BMP offset %d, width %d, height %d, bpp %d, compression %d\n",
            offset, i->width, i->height, bmp->bpp, bmp->compression);

  if (bmp->compression > 3) {   // JPEG/PNG
    LOG_WARN("Unsupported BMP compression type: %d (%s)\n", bmp->compression, r->path);
    return 0;
  }

  // Negative height indicates a flipped image
  if (i->height < 0) {
    bmp->flipped = 1;
    i->height = abs(i->height);
  }

  // Not used during reading, but lets output PNG be correct
  i->channels = 4;

  // Skip BMP size, resolution
  buffer_consume(bmp->buf, 12);

  palette_colors = buffer_get_int_le(bmp->buf);

  // Skip number of important colors
  buffer_consume(bmp->buf, 4);

  // < 16-bit always has a palette
  if (!palette_colors && bmp->bpp < 16) {
    switch (bmp->bpp) {
      case 8:
        palette_colors = 256;
        break;
      case 4:
        palette_colors = 16;
        break;
      case 1:
        palette_colors = 2;
        break;
    }
  }

  LOG_DEBUG("palette_colors %d\n", palette_colors);
  if (palette_colors) {
    // Read palette
    int x;

    if (palette_colors > 256) {
      LOG_WARN("Cannot read BMP with palette > 256 colors (%s)\n", r->path);
      return 0;
    }

    for (x = 0; x < palette_colors; x++) {
      int b = buffer_get_char(bmp->buf);
      int g = buffer_get_char(bmp->buf);
      int r = buffer_get_char(bmp->buf);
      buffer_consume(bmp->buf, 1);

      bmp->palette_colors[x] = COL(r, g, b);
      LOG_DEBUG("palette %d = %08x\n", x, bmp->palette_colors[x]);
    }
  }
  else if (bmp->compression == BMP_BI_BITFIELDS) {
    int pos, bit, x;

    if (bmp->bpp == 16) {
      // Read 16-bit bitfield masks
      for (x = 0; x < 3; x++) {
        masks[x] = buffer_get_int_le(bmp->buf);

        // Determine shift value
        pos = 0;
        bit = masks[x] & -masks[x];
        while (bit) {
          pos++;
          bit >>= 1;
        }
        shifts[x] = pos - 1;

        // green can be 6 bits
        if (x == 1) {
          if (masks[1] == 0x7e0)
            ncolors[1] = (1 << 6) - 1;
          else
            ncolors[1] = (1 << 5) - 1;
        }

        LOG_DEBUG("16bpp mask %d: %08x >> %d, ncolors %d\n", x, masks[x], shifts[x], ncolors[x]);
      }
    }
    else {                      // 32-bit bitfields
      // Read 32-bit bitfield masks
      for (x = 0; x < 3; x++) {
        masks[x] = buffer_get_int_le(bmp->buf);

        // Determine shift value
        pos = 0;
        bit = masks[x] & -masks[x];
        while (bit) {
          pos++;
          bit >>= 1;
        }
        shifts[x] = pos - 1;

        LOG_DEBUG("32bpp mask %d: %08x >> %d\n", x, masks[x], shifts[x]);
      }
    }
  }

  // XXX make sure to skip to offset

  return 1;
}

int image_bmp_load(MediaScanImage *i) {
  int offset = 0;
  int paddingbits = 0;
  int mask = 0;
  int j, x, y, blen;
  int starty, lasty, incy, linebytes;
  unsigned char *bptr;
  BMPData *bmp = (BMPData *) i->_bmp;

  // XXX If reusing the object a second time, reset buffer

  // Calculate bits of padding per line
  paddingbits = 32 - (i->width * bmp->bpp) % 32;
  if (paddingbits == 32)
    paddingbits = 0;

  // Bytes per line
  linebytes = ((i->width * bmp->bpp) + paddingbits) / 8;

  // No padding if RLE compressed
  if (paddingbits && (bmp->compression == BMP_BI_RLE4 || bmp->compression == BMP_BI_RLE8))
    paddingbits = 0;

  // XXX Don't worry about RLE support yet
  if (bmp->compression == BMP_BI_RLE4 || bmp->compression == BMP_BI_RLE8) {
    LOG_WARN("BMP RLE compression is not supported\n");
    image_bmp_destroy(i);
    return 0;
  }

  LOG_DEBUG("linebits %d, paddingbits %d, linebytes %d\n", i->width * bmp->bpp, paddingbits, linebytes);

  bptr = buffer_ptr(bmp->buf);
  blen = buffer_len(bmp->buf);

  // Allocate storage for decompressed image
  image_alloc_pixbuf(i, i->width, i->height);

  if (bmp->flipped) {
    starty = 0;
    lasty = i->height;
    incy = 1;
  }
  else {
    starty = i->height - 1;
    lasty = -1;
    incy = -1;
  }

  y = starty;

  if (bmp->bpp == 1)
    mask = 0x80;
  else if (bmp->bpp == 4)
    mask = 0xF0;

  while (y != lasty) {
    for (x = 0; x < i->width; x++) {
      if (blen <= 0 || blen < bmp->bpp / 8) {
        // Load more from the buffer
        if (blen < 0)
          blen = 0;

        buffer_consume(bmp->buf, buffer_len(bmp->buf) - blen);

        if (!buffer_check_load(bmp->buf, bmp->fp, i->channels, BUF_SIZE)) {
          image_bmp_destroy(i);
          LOG_WARN("Unable to read entire BMP file (%s)\n", i->path);
          return 0;
        }

        bptr = buffer_ptr(bmp->buf);
        blen = buffer_len(bmp->buf);
        offset = 0;
      }

      j = x + (y * i->width);

      switch (bmp->bpp) {
        case 32:               // XXX how to detect alpha channel?
          //im->pixbuf[i] = COL_FULL(bptr[offset+2], bptr[offset+1], bptr[offset], bptr[offset+3]);
          i->_pixbuf[j] = COL(bptr[offset + 2], bptr[offset + 1], bptr[offset]);
          offset += 4;
          blen -= 4;
          linebytes -= 4;
          break;

        case 24:               // 24-bit BGR
          i->_pixbuf[j] = COL(bptr[offset + 2], bptr[offset + 1], bptr[offset]);
          offset += 3;
          blen -= 3;
          linebytes -= 3;
          break;

        case 16:
          {
            int p = (bptr[offset + 1] << 8) | bptr[offset];

            /*
               LOG_DEBUG("p %x (r %02x g %02x b %02x)\n", p,
               ((p & masks[0]) >> shifts[0]) * 255 / ncolors[0],
               ((p & masks[1]) >> shifts[1]) * 255 / ncolors[1],
               ((p & masks[2]) >> shifts[2]) * 255 / ncolors[2]);
             */

            i->_pixbuf[j] = COL(((p & masks[0]) >> shifts[0]) * 255 /
                                ncolors[0],
                                ((p & masks[1]) >> shifts[1]) * 255 /
                                ncolors[1], ((p & masks[2]) >> shifts[2]) * 255 / ncolors[2]
              );

            offset += 2;
            blen -= 2;
            linebytes -= 2;
            break;
          }

        case 8:
          i->_pixbuf[j] = bmp->palette_colors[bptr[offset]];
          offset++;
          blen--;
          linebytes--;
          break;

        case 4:
          // uncompressed
          if (mask == 0xF0) {
            i->_pixbuf[j] = bmp->palette_colors[(bptr[offset] & mask) >> 4];
            mask = 0xF;
          }
          else {
            i->_pixbuf[j] = bmp->palette_colors[(bptr[offset] & mask)];
            offset++;
            blen--;
            linebytes--;
            mask = 0xF0;
          }
          break;

        case 1:
          i->_pixbuf[j] = bmp->palette_colors[(bptr[offset] & mask) ? 1 : 0];
          mask >>= 1;
          if (!mask) {
            offset++;
            blen--;
            linebytes--;
            mask = 0x80;
          }
          break;
      }

      //LOG_DEBUG("x %d / y %d, linebytes left %d, pix %08x\n", x, y, linebytes, i->_pixbuf[j]);
    }

    if (linebytes) {
      //LOG_DEBUG("Consuming %d bytes of padding\n", linebytes);

      if (blen < linebytes) {
        // Load more from the buffer
        buffer_consume(bmp->buf, buffer_len(bmp->buf) - blen);
        if (!buffer_check_load(bmp->buf, bmp->fp, i->channels, BUF_SIZE)) {
          image_bmp_destroy(i);
          LOG_WARN("Unable to read entire BMP file (%s)\n", i->path);
          return 0;
        }
        bptr = buffer_ptr(bmp->buf);
        blen = buffer_len(bmp->buf);
        offset = 0;
      }

      offset += linebytes;
      blen -= linebytes;

      // Reset mask for next line
      if (bmp->bpp == 4)
        mask = 0xF0;
      else if (bmp->bpp == 1)
        mask = 0x80;
    }

    linebytes = ((i->width * bmp->bpp) + paddingbits) / 8;

    y += incy;
  }

  // Set channels to 4 so we write a color PNG, unless bpp is 1
  // XXX channels is always 4...
  if (bmp->bpp > 1)
    i->channels = 4;

  return 1;
}

void image_bmp_destroy(MediaScanImage *i) {
  if (i->_bmp) {
    LOG_MEM("destroy BMPData @ %p\n", i->_bmp);
    free(i->_bmp);
    i->_bmp = NULL;
  }
}
