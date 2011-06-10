// Derived from:

/* $OpenBSD: buffer.h,v 1.17 2008/05/08 06:59:01 markus Exp $ */

/*
 * Author: Tatu Ylonen <ylo@cs.hut.fi>
 * Copyright (c) 1995 Tatu Ylonen <ylo@cs.hut.fi>, Espoo, Finland
 *                    All rights reserved
 * Code for manipulating FIFO buffers.
 *
 * As far as I am concerned, the code I have written for this software
 * can be used freely for any purpose.  Any derived versions of this
 * software must be clearly marked as such, and if the derived work is
 * incompatible with the protocol description in the RFC file, it must be
 * called by a name other than "ssh" or "Secure Shell".
 */

#ifndef BUFFER_H
#define BUFFER_H

#include <stdint.h>

#ifndef roundup
#define	roundup(x, y)	((((x)+((y)-1))/(y))*(y)) /* to any y */
#endif

#ifndef MAX
#define MAX(X,Y) ((X) > (Y) ? (X) : (Y))
#endif

#ifndef MIN
#define MIN(X,Y) ((X) < (Y) ? (X) : (Y))
#endif

typedef struct {
  unsigned char *buf;           /* Buffer for data. */
  unsigned int alloc;           /* Number of bytes allocated for data. */
  unsigned int offset;          /* Offset of first byte containing data. */
  unsigned int end;             /* Offset of last byte containing data. */
  unsigned int cache;           /* bit cache for buffer_get_bits */
  unsigned int ncached;         /* Number of bits in cache */
} Buffer;

enum utf16_byteorder {
  UTF16_BYTEORDER_ANY,
  UTF16_BYTEORDER_BE,
  UTF16_BYTEORDER_LE
};

void buffer_init(Buffer *buffer, uint32_t len);
void buffer_init_or_clear(Buffer *buffer, uint32_t len);
void buffer_free(Buffer *buffer);
void buffer_clear(Buffer *buffer);
void buffer_append(Buffer *buffer, const void *data, uint32_t len);
static int buffer_compact(Buffer *buffer);
void *buffer_append_space(Buffer *buffer, uint32_t len);
int buffer_check_alloc(Buffer *buffer, uint32_t len);
uint32_t buffer_len(Buffer *buffer);
int buffer_get_ret(Buffer *buffer, void *buf, uint32_t len);
void buffer_get(Buffer *buffer, void *buf, uint32_t len);
int buffer_consume_ret(Buffer *buffer, uint32_t bytes);
void buffer_consume(Buffer *buffer, uint32_t bytes);
int buffer_consume_end_ret(Buffer *buffer, uint32_t bytes);
void buffer_consume_end(Buffer *buffer, uint32_t bytes);
void *buffer_ptr(Buffer *buffer);
void buffer_dump(Buffer *buffer, uint32_t len);
int buffer_get_char_ret(char *ret, Buffer *buffer);
int buffer_get_char(Buffer *buffer);
uint32_t get_u32le(const void *vp);
int buffer_get_int_le_ret(uint32_t *ret, Buffer *buffer);
uint32_t buffer_get_int_le(Buffer *buffer);
uint32_t get_u32(const void *vp);
int buffer_get_int_ret(uint32_t *ret, Buffer *buffer);
uint32_t buffer_get_int(Buffer *buffer);
uint32_t get_u24(const void *vp);
int buffer_get_int24_ret(uint32_t *ret, Buffer *buffer);
uint32_t buffer_get_int24(Buffer *buffer);
uint32_t get_u24le(const void *vp);
int buffer_get_int24_le_ret(uint32_t *ret, Buffer *buffer);
uint32_t buffer_get_int24_le(Buffer *buffer);
uint64_t get_u64le(const void *vp);
int buffer_get_int64_le_ret(uint64_t *ret, Buffer *buffer);
uint64_t buffer_get_int64_le(Buffer *buffer);
uint64_t get_u64(const void *vp);
int buffer_get_int64_ret(uint64_t *ret, Buffer *buffer);
uint64_t buffer_get_int64(Buffer *buffer);
uint16_t get_u16le(const void *vp);
int buffer_get_short_le_ret(uint16_t *ret, Buffer *buffer);
uint16_t buffer_get_short_le(Buffer *buffer);
uint16_t get_u16(const void *vp);
int buffer_get_short_ret(uint16_t *ret, Buffer *buffer);
uint16_t buffer_get_short(Buffer *buffer);
void buffer_put_char(Buffer *buffer, int value);
uint32_t buffer_get_utf8(Buffer *buffer, Buffer *utf8, uint32_t len_hint);
uint32_t buffer_get_latin1_as_utf8(Buffer *buffer, Buffer *utf8, uint32_t len_hint);
uint32_t buffer_get_utf16_as_utf8(Buffer *buffer, Buffer *utf8, uint32_t len, uint8_t byteorder);
#ifdef HAS_GUID
void buffer_get_guid(Buffer *buffer, GUID *g);
#endif
int buffer_get_float32_le_ret(float *ret, Buffer *buffer);
float buffer_get_float32_le(Buffer *buffer);
float get_f32le(const void *vp);
int buffer_get_float32_ret(float *ret, Buffer *buffer);
float buffer_get_float32(Buffer *buffer);
float get_f32(const void *vp);
double buffer_get_ieee_float(Buffer *buffer);
void put_u16(void *vp, uint16_t v);
void put_u32(void *vp, uint32_t v);
uint32_t buffer_get_bits(Buffer *buffer, uint32_t bits);
uint32_t buffer_get_syncsafe(Buffer *buffer, uint8_t bytes);
int buffer_check_load(Buffer *buffer, FILE *fp, int min_wanted, int max_wanted);

#endif
