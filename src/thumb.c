
#include <libmediascan.h>
#include <stdlib.h>
#include <string.h>

#ifdef WIN32
#include "mediascan_win32.h"
#endif

#include "common.h"
#include "image.h"
#include "error.h"
#include "thumb.h"
#include "image_jpeg.h"
#include "fixed.h"

MediaScanImage *
thumb_create_from_image(MediaScanImage *i, MediaScanThumbSpec *spec_orig)
{
  MediaScanImage *thumb;
  
  // Create a copy of the spec, so we can adjust width/height as needed
  MediaScanThumbSpec *spec = (MediaScanThumbSpec *)calloc(sizeof(MediaScanThumbSpec), 1);
  memcpy(spec, spec_orig, sizeof(MediaScanThumbSpec));
  LOG_MEM("new MediaScanThumbSpec @ %p\n", spec);
  
  thumb = image_create();
  thumb->path = i->path;
  
  // Determine thumb dimensions
  
  // If the image will be rotated 90 degrees, swap the target values
  if (i->orientation >= 5) {
    if (!spec->height) {
      // Only width was specified, but this will actually be the target height
      spec->height = spec->width;
      spec->width = 0;
    }
    else if (!spec->width) {
      // Only height was specified, but this will actually be the target width
      spec->width = spec->height;
      spec->height = 0;
    }
  }
  
  if (!spec->height) // Only width was specified
    spec->height = (int)((float)i->height / i->width * spec->width);
  else if (!spec->width) // Only height was specified
    spec->width = (int)((float)i->width / i->height * spec->height);
    
  LOG_DEBUG("Resizing from %d x %d -> %d x %d\n", i->width, i->height, spec->width, spec->height);
  
  thumb->width = spec->width;
  thumb->height = spec->height;
  
  // Load the source image into memory, we pass the spec to give a hint
  // to the loader when it can optimize the loaded size (JPEG)
  if ( !image_load(i, spec) )
    goto err;
  
  // Resize, will store uncompressed resize data in pixbuf
  if ( !thumb_resize(i, thumb, spec) )
    goto err;
  
  if (spec->format == THUMB_AUTO) {
    // Transparent source always gets output as PNG
    if (i->has_alpha)
      spec->format = THUMB_PNG;
    // Use PNG if any padding was applied so it will be transparent
    else if (spec->height_padding || spec->width_padding)
      spec->format = THUMB_PNG;
    else
      spec->format = THUMB_JPEG;
  }
  
  // Compress pixbuf data into thumb->data
  switch (spec->format) {  
    case THUMB_JPEG:
      thumb->codec = "JPEG";
      image_jpeg_compress(thumb, spec);
      break;
      
    case THUMB_PNG:
      thumb->codec = "PNG";
      image_png_compress(thumb, spec);
      break;
  }
  
  goto ok;
  
err:
  LOG_WARN("Thumbnail creation failed for %s\n", i->path);
  image_destroy(thumb);
  thumb = NULL;

ok:
  LOG_MEM("destroy MediaScanThumbSpec @ %p\n", spec);
  free(spec);
  
  return thumb;
}

void
thumb_bgcolor_fill(pix *buf, int size, pix bgcolor)
{
  int i;
  
  if (bgcolor != 0) {
    for (i = 0; i < size; i += sizeof(pix))
      memcpy( ((char *)buf) + i, &bgcolor, sizeof(pix) );
  }
  else {
    memset(buf, 0, size);
  }
}

int
thumb_resize(MediaScanImage *src, MediaScanImage *dst, MediaScanThumbSpec *spec)
{
  int ret = 1;
  
  // Special case for equal size without resizing
  if (src->width == dst->width && src->height == dst->height) {
    dst->_pixbuf = src->_pixbuf;
    dst->_pixbuf_is_copy = 1;
    goto out;
  }
  
  // Allocate space for the resized image
  image_alloc_pixbuf(dst, dst->width, dst->height);
  
  // Determine padding if necessary
  if (spec->keep_aspect) {
    float source_ar = 1.0f * src->width / src->height;
    float dest_ar   = 1.0f * dst->width / dst->height;
    
    if (source_ar >= dest_ar) {
      spec->height_padding = (int)((dst->height - (dst->width / source_ar)) / 2);
      spec->height_inner   = (int)(dst->width / source_ar);
    }
    else {
      spec->width_padding = (int)((dst->width - (dst->height * source_ar)) / 2);
      spec->width_inner   = (int)(dst->height * source_ar);
    }
    
    // Fill new space with the bgcolor or zeros
    thumb_bgcolor_fill(dst->_pixbuf, dst->_pixbuf_size, spec->bgcolor);
    
    LOG_DEBUG("thumb using width padding %d, inner width %d, height padding %d, inner height %d, bgcolor %x\n",
      spec->width_padding, spec->width_inner, spec->height_padding, spec->height_inner, spec->bgcolor);
  }
  
  thumb_resize_gd_fixed(src, dst, spec);
  
  // If the image was rotated, swap the width/height if necessary
  // This is needed for the save_*() functions to output the correct size
  if (src->orientation >= 5) {
    int tmp = dst->height;
    dst->height = dst->width;
    dst->width = tmp;
    
    LOG_DEBUG("Image was rotated, dst now %d x %d\n", dst->width, dst->height);
  }
  
out:
  return ret;
}

static inline pix
get_pix(MediaScanImage *i, int32_t x, int32_t y)
{
	return (i->_pixbuf[(y * i->width) + x]);
}

static inline void
put_pix(MediaScanImage *i, int32_t x, int32_t y, pix col)
{
	i->_pixbuf[(y * i->width) + x] = col;
}

static inline void
put_pix_rotated(MediaScanImage *i, int32_t x, int32_t y, int32_t rotated_width, pix col)
{
  i->_pixbuf[(y * rotated_width) + x] = col;
}

static inline void
get_rotated_coords(MediaScanImage *src, MediaScanImage *dst, int x, int y, int *ox, int *oy)
{
  switch (src->orientation) {
    case ORIENTATION_MIRROR_HORIZ: // 2
      *ox = dst->width - 1 - x;
      *oy = y;
      break;
    case ORIENTATION_180: // 3
      *ox = dst->width - 1 - x;
      *oy = dst->height - 1 - y;
      break;
    case ORIENTATION_MIRROR_VERT: // 4
      *ox = x;
      *oy = dst->height - 1 - y;
      break;
    case ORIENTATION_MIRROR_HORIZ_270_CCW: // 5
      *ox = y;
      *oy = x;
      break;
    case ORIENTATION_90_CCW: // 6
      *ox = dst->height - 1 - y;
      *oy = x;
      break;
    case ORIENTATION_MIRROR_HORIZ_90_CCW: // 7
      *ox = dst->height - 1 - y;
      *oy = dst->width - 1 - x;
      break;
    case ORIENTATION_270_CCW: // 8
      *ox = y;
      *oy = dst->width - 1 - x;
      break;
    default:
      if (x == 0 && y == 0)
        LOG_WARN("Cannot rotate image, unknown orientation value: %d (%s)\n", src->orientation, src->path);
      *ox = x;
      *oy = y;
      break;
  }
}

// This is a fixed-point resizer inspired by libgd's copyResampled function
void
thumb_resize_gd_fixed(MediaScanImage *src, MediaScanImage *dst, MediaScanThumbSpec *spec)
{
  int x, y;
  fixed_t sy1, sy2, sx1, sx2;
  int dstX = 0, dstY = 0, srcX = 0, srcY = 0;
  fixed_t width_scale, height_scale;
  
  int dstW = dst->width;
  int dstH = dst->height;
  int srcW = src->width;
  int srcH = src->height;
  
  if (spec->height_padding) {
    dstY = spec->height_padding;
    dstH = spec->height_inner;
  }
  
  if (spec->width_padding) {
    dstX = spec->width_padding;
    dstW = spec->width_inner;
  }
  
  width_scale = fixed_div(int_to_fixed(srcW), int_to_fixed(dstW));
  height_scale = fixed_div(int_to_fixed(srcH), int_to_fixed(dstH));
  
  for (y = dstY; (y < dstY + dstH); y++) {
    sy1 = fixed_mul(int_to_fixed(y - dstY), height_scale);
    sy2 = fixed_mul(int_to_fixed((y + 1) - dstY), height_scale);
    
    for (x = dstX; (x < dstX + dstW); x++) {
      fixed_t sx, sy;
  	  fixed_t spixels = 0;
  	  fixed_t red = 0, green = 0, blue = 0, alpha = 0;
  	  
  	  if (!src->has_alpha)
        alpha = FIXED_255;
  	  
      sx1 = fixed_mul(int_to_fixed(x - dstX), width_scale);
      sx2 = fixed_mul(int_to_fixed((x + 1) - dstX), width_scale);  	  
  	  sy = sy1;
  	  
  	  /*
      LOG_DEBUG("sx1 %f, sx2 %f, sy1 %f, sy2 %f\n",
        fixed_to_float(sx1), fixed_to_float(sx2), fixed_to_float(sy1), fixed_to_float(sy2));
      */
  	  
  	  do {
        fixed_t yportion;
        
        //LOG_DEBUG("  yportion(sy %f, sy1 %f, sy2 %f) = ", fixed_to_float(sy), fixed_to_float(sy1), fixed_to_float(sy2));
        
        if (fixed_floor(sy) == fixed_floor(sy1)) {
          yportion = FIXED_1 - (sy - fixed_floor(sy));
    		  if (yportion > sy2 - sy1) {
            yportion = sy2 - sy1;
    		  }
    		  sy = fixed_floor(sy);
    		}
    		else if (sy == fixed_floor(sy2)) {
          yportion = sy2 - fixed_floor(sy2);
        }
        else {
          yportion = FIXED_1;
        }
        
        //LOG_DEBUG("%f\n", fixed_to_float(yportion));
        
        sx = sx1;
        
        do {
          fixed_t xportion;
    		  fixed_t pcontribution;
    		  pix p;
    		  
    		  //LOG_DEBUG("  xportion(sx %f, sx1 %f, sx2 %f) = ", fixed_to_float(sx), fixed_to_float(sx1), fixed_to_float(sx2));
  		  
    		  if (fixed_floor(sx) == fixed_floor(sx1)) {
    	      xportion = FIXED_1 - (sx - fixed_floor(sx));
    	      if (xportion > sx2 - sx1)	{
              xportion = sx2 - sx1;
    			  }
    		    sx = fixed_floor(sx);
    		  }
    		  else if (sx == fixed_floor(sx2)) {
            xportion = sx2 - fixed_floor(sx2);
          }
    		  else {
    		    xportion = FIXED_1;
    		  }
    		  
    		  //LOG_DEBUG("%f\n", fixed_to_float(xportion));
  		  
    		  pcontribution = fixed_mul(xportion, yportion);
  		  
    		  p = get_pix(src, fixed_to_int(sx + srcX), fixed_to_int(sy + srcY));
    		  
    		  /*
    		  LOG_DEBUG("  merging with pix %d, %d: src %x (%d %d %d %d), pcontribution %f\n",
            fixed_to_int(sx + srcX), fixed_to_int(sy + srcY),
            p, COL_RED(p), COL_GREEN(p), COL_BLUE(p), COL_ALPHA(p), fixed_to_float(pcontribution));
          */
  		    
          red   += fixed_mul(int_to_fixed(COL_RED(p)), pcontribution);
          green += fixed_mul(int_to_fixed(COL_GREEN(p)), pcontribution);
    		  blue  += fixed_mul(int_to_fixed(COL_BLUE(p)), pcontribution);
    		  
    		  if (src->has_alpha)
    		    alpha += fixed_mul(int_to_fixed(COL_ALPHA(p)), pcontribution);
    		  
    		  spixels += pcontribution;
    		  sx += FIXED_1;
    		} while (sx < sx2);
    		
        sy += FIXED_1;
      } while (sy < sy2);
      
  	  // If rgba get too large for the fixed-point representation, fallback to the floating point routine
		  // This should only happen with very large images
		  if (red < 0 || green < 0 || blue < 0 || alpha < 0) {
        LOG_WARN("fixed-point overflow: %d %d %d %d\n", red, green, blue, alpha);
        // XXX fallback to floating point?
      }
      
      if (spixels != 0) {
        /*
        LOG_DEBUG("  rgba (%f %f %f %f) spixels %f\n",
          fixed_to_float(red), fixed_to_float(green), fixed_to_float(blue), fixed_to_float(alpha), fixed_to_float(spixels));
        */
        
        spixels = fixed_div(FIXED_1, spixels);
        
        red   = fixed_mul(red, spixels);
        green = fixed_mul(green, spixels);
        blue  = fixed_mul(blue, spixels);
        
        if (src->has_alpha)
          alpha = fixed_mul(alpha, spixels);
	    }
	    
	    /* Clamping to allow for rounding errors above */
      if (red > FIXED_255)   red = FIXED_255;
      if (green > FIXED_255) green = FIXED_255;
      if (blue > FIXED_255)  blue = FIXED_255;
      if (src->has_alpha && alpha > FIXED_255) alpha = FIXED_255;
      
      /*
      LOG_DEBUG("  -> %d, %d %x (%d %d %d %d)\n",
        x, y, COL_FULL(fixed_to_int(red), fixed_to_int(green), fixed_to_int(blue), fixed_to_int(alpha)),
        fixed_to_int(red), fixed_to_int(green), fixed_to_int(blue), fixed_to_int(alpha));
      */
      
      if (src->orientation != ORIENTATION_NORMAL) {
        int ox, oy; // new destination pixel coordinates after rotating
        
        get_rotated_coords(src, dst, x, y, &ox, &oy);
        
        if (src->orientation >= 5) {
          // 90 and 270 rotations, width/height are swapped so we have to use alternate put_pix method
          put_pix_rotated(
            dst, ox, oy, dst->height,
            COL_FULL(fixed_to_int(red), fixed_to_int(green), fixed_to_int(blue), fixed_to_int(alpha))
          );
        }
        else {
          put_pix(
            dst, ox, oy,
            COL_FULL(fixed_to_int(red), fixed_to_int(green), fixed_to_int(blue), fixed_to_int(alpha))
          );
        }
      }
      else {
        put_pix(
          dst, x, y,
          COL_FULL(fixed_to_int(red), fixed_to_int(green), fixed_to_int(blue), fixed_to_int(alpha))
        );
      }
	  }
	}
}
