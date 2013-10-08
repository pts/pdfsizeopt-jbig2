// Copyright 2006 Google Inc. All Rights Reserved.
// Author: agl@imperialviolet.org (Adam Langley)
//
// Copyright (C) 2006 Google Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "jbig2arith.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#define u64 uint64_t
#define u32 uint32_t
#define u16 uint16_t
#define u8  uint8_t

// C++ doesn't have C99 restricted pointers, but GCC does allow __restrict__
#if !defined(WIN32)
#define restrict __restrict__
#else
#define restrict
#endif

// -----------------------------------------------------------------------------
// This the structure for a single state of the adaptive arithmetic compressor
// -----------------------------------------------------------------------------
struct context {
  u16 qe;
  u8 mps, lps;
};

// -----------------------------------------------------------------------------
// And this is the table of states for that adaptive compressor
// -----------------------------------------------------------------------------
struct context ctbl[] = {
  // This is the standard state table from
  // Table E.1 of the standard. The switch has been omitted and
  // those states are included below
#define STATETABLE \
  {0x5601, F( 1), SWITCH(F( 1))},\
  {0x3401, F( 2), F( 6)},\
  {0x1801, F( 3), F( 9)},\
  {0x0ac1, F( 4), F(12)},\
  {0x0521, F( 5), F(29)},\
  {0x0221, F(38), F(33)},\
  {0x5601, F( 7), SWITCH(F( 6))},\
  {0x5401, F( 8), F(14)},\
  {0x4801, F( 9), F(14)},\
  {0x3801, F(10), F(14)},\
  {0x3001, F(11), F(17)},\
  {0x2401, F(12), F(18)},\
  {0x1c01, F(13), F(20)},\
  {0x1601, F(29), F(21)},\
  {0x5601, F(15), SWITCH(F(14))},\
  {0x5401, F(16), F(14)},\
  {0x5101, F(17), F(15)},\
  {0x4801, F(18), F(16)},\
  {0x3801, F(19), F(17)},\
  {0x3401, F(20), F(18)},\
  {0x3001, F(21), F(19)},\
  {0x2801, F(22), F(19)},\
  {0x2401, F(23), F(20)},\
  {0x2201, F(24), F(21)},\
  {0x1c01, F(25), F(22)},\
  {0x1801, F(26), F(23)},\
  {0x1601, F(27), F(24)},\
  {0x1401, F(28), F(25)},\
  {0x1201, F(29), F(26)},\
  {0x1101, F(30), F(27)},\
  {0x0ac1, F(31), F(28)},\
  {0x09c1, F(32), F(29)},\
  {0x08a1, F(33), F(30)},\
  {0x0521, F(34), F(31)},\
  {0x0441, F(35), F(32)},\
  {0x02a1, F(36), F(33)},\
  {0x0221, F(37), F(34)},\
  {0x0141, F(38), F(35)},\
  {0x0111, F(39), F(36)},\
  {0x0085, F(40), F(37)},\
  {0x0049, F(41), F(38)},\
  {0x0025, F(42), F(39)},\
  {0x0015, F(43), F(40)},\
  {0x0009, F(44), F(41)},\
  {0x0005, F(45), F(42)},\
  {0x0001, F(45), F(43)},
#undef F
#define F(x) x
#define SWITCH(x) (x + 46)
  STATETABLE
#undef SWITCH
#undef F

#define F(x) (x + 46)
#define SWITCH(x) ((x) - 46)
  STATETABLE
#undef SWITCH
#undef F
};

#if __GNUC__ >= 4
#define BRANCH_OPT
#endif

// GCC peephole optimisations
#ifdef BRANCH_OPT
#define likely(x)       __builtin_expect((x),1)
#define unlikely(x)     __builtin_expect((x),0)
#else
#define likely(x)       x
#define unlikely(x)     x
#endif

// see comments in .h file
void
jbig2enc_init(struct jbig2enc_ctx *ctx) {
  memset(ctx->context, 0, JBIG2_MAX_CTX);
  memset(ctx->intctx, 0, 13 * 512);
  ctx->a = 0x8000;
  ctx->c = 0;
  ctx->ct = 12;
  ctx->bp = -1;
  ctx->b = 0;
  ctx->outbuf_used = 0;
  ctx->outbuf = (u8 *) malloc(JBIG2_OUTPUTBUFFER_SIZE);
  ctx->output_chunks = new jbvector<uint8_t *>;
  ctx->iaidctx = NULL;
}

#if JBIG2_COMPILE_UNUSED
// see comments in .h file
void
jbig2enc_reset(struct jbig2enc_ctx *ctx) {
  ctx->a = 0x8000;
  ctx->c = 0;
  ctx->ct = 12;
  ctx->bp = -1;
  ctx->b = 0;
  free(ctx->iaidctx);
  ctx->iaidctx = NULL;
  memset(ctx->context, 0, JBIG2_MAX_CTX);
  memset(ctx->intctx, 0, 13 * 512);
}
#endif

#if JBIG2_COMPILE_UNUSED
// see comments in .h file
void
jbig2enc_flush(struct jbig2enc_ctx *ctx) {
  ctx->outbuf_used = 0;

  for (jbvector<uint8_t *>::iterator i = ctx->output_chunks->begin();
       i != ctx->output_chunks->end(); ++i) {
    free(*i);
  }
  ctx->output_chunks->clear();
  ctx->bp = -1;
}
#endif

// see comments in .h file
void
jbig2enc_dealloc(struct jbig2enc_ctx *ctx) {
  for (jbvector<uint8_t *>::iterator i = ctx->output_chunks->begin();
       i != ctx->output_chunks->end(); ++i) {
    free(*i);
  }
  delete ctx->output_chunks;
  free(ctx->outbuf);
  free(ctx->iaidctx);
}

// -----------------------------------------------------------------------------
// Emit a byte from the compressor by appending to the current output buffer.
// If the buffer is full, allocate a new one
// -----------------------------------------------------------------------------
static void inline
emit(struct jbig2enc_ctx *restrict ctx) {
  if (unlikely(ctx->outbuf_used == JBIG2_OUTPUTBUFFER_SIZE)) {
    ctx->output_chunks->push_back(ctx->outbuf);
    ctx->outbuf = (u8 *) malloc(JBIG2_OUTPUTBUFFER_SIZE);
    ctx->outbuf_used = 0;
  }

  ctx->outbuf[ctx->outbuf_used++] = ctx->b;
}

// -----------------------------------------------------------------------------
// The BYTEOUT procedure from the standard
// -----------------------------------------------------------------------------
static void
byteout(struct jbig2enc_ctx *restrict ctx) {
  if (ctx->b == 0xff) goto rblock;

  if (ctx->c < 0x8000000) goto lblock;
  ctx->b += 1;
  if (ctx->b != 0xff) goto lblock;
  ctx->c &= 0x7ffffff;

rblock:
  if (ctx->bp >= 0) {
#ifdef TRACE
    printf("emit %x\n", ctx->b);
#endif
    emit(ctx);
  }
  ctx->b = ctx->c >> 20;
  ctx->bp++;
  ctx->c &= 0xfffff;
  ctx->ct = 7;
  return;

lblock:
  if (ctx->bp >= 0) {
#ifdef TRACE
    printf("emit %x\n", ctx->b);
#endif
    emit(ctx);
  }
  ctx->b = ctx->c >> 19;
  ctx->bp++;
  ctx->c &= 0x7ffff;
  ctx->ct = 8;
  return;
}

// -----------------------------------------------------------------------------
// A merging of the ENCODE, CODELPS and CODEMPS procedures from the standard
// -----------------------------------------------------------------------------
static void
encode_bit(struct jbig2enc_ctx *restrict ctx, u8 *restrict context, u32 ctxnum, u8 d) {
  const u8 i = context[ctxnum];
  const u8 mps = i > 46 ? 1 : 0;
  const u16 qe = ctbl[i].qe;

#ifdef CODER_DEBUGGING
    fprintf(stderr, "B: %d %d %d %d\n", ctxnum, qe, ctx->a, d);
#endif

#ifdef TRACE
  static int ec = 0;
  printf("%d\t%d %d %x %x %x %d %x %d\n", ec++, i, mps, qe, ctx->a, ctx->c, ctx->ct, ctx->b, ctx->bp);
#endif

  if (unlikely(d != mps)) goto codelps;
#ifdef SURPRISE_MAP
  {
  u8 b = static_cast<unsigned char>
    (((static_cast<float>(qe) / 0xac02) * 255));
  write(3, &b, 1);
  }
#endif
  ctx->a -= qe;
  if (unlikely((ctx->a & 0x8000) == 0)) {
    if (unlikely(ctx->a < qe)) {
      ctx->a = qe;
    } else {
      ctx->c += qe;
    }
    context[ctxnum] = ctbl[i].mps;
    goto renorme;
  } else {
    ctx->c += qe;
  }

  return;

codelps:
#ifdef SURPRISE_MAP
  {
  u8 b = static_cast<unsigned char>
    ((1.0f - (static_cast<float>(qe) / 0xac02)) * 255);
  write(3, &b, 1);
  }
#endif
  ctx->a -= qe;
  if (ctx->a < qe) {
    ctx->c += qe;
  } else {
    ctx->a = qe;
  }
  context[ctxnum] = ctbl[i].lps;

renorme:
  do {
    ctx->a <<= 1;
    ctx->c <<= 1;
    ctx->ct -= 1;
    if (unlikely(!ctx->ct)) {
      byteout(ctx);
    }
  } while ((ctx->a & 0x8000) == 0);
}

// -----------------------------------------------------------------------------
// The FINALISE procudure from the standard
// -----------------------------------------------------------------------------
static void
encode_final(struct jbig2enc_ctx *restrict ctx) {
  // SETBITS
  const u32 tempc = ctx->c + ctx->a;
  ctx->c |= 0xffff;
  if (ctx->c >= tempc) {
    ctx->c -= 0x8000;
  }

  ctx->c <<= ctx->ct;
  byteout(ctx);
  ctx->c <<= ctx->ct;
  byteout(ctx);
  emit(ctx);
  if (ctx->b != 0xff) {
#ifdef TRACE
    printf("emit 0xff\n");
#endif
    ctx->b = 0xff;
    emit(ctx);
  }
#ifdef TRACE
  printf("emit 0xac\n");
#endif
  ctx->b = 0xac;
  emit(ctx);
}

// see comments in .h file
void
jbig2enc_final(struct jbig2enc_ctx *restrict ctx) {
  encode_final(ctx);
}

// see comments in .h file
unsigned
jbig2enc_datasize(const struct jbig2enc_ctx *ctx) {
  return JBIG2_OUTPUTBUFFER_SIZE * ctx->output_chunks->size() + ctx->outbuf_used;
}

// see comments in .h file
void
jbig2enc_tobuffer(const struct jbig2enc_ctx *restrict ctx, u8 *restrict buffer) {
  int j = 0;
  for (jbvector<u8 *>::const_iterator i = ctx->output_chunks->begin();
       i != ctx->output_chunks->end(); ++i) {
    memcpy(&buffer[j], *i, JBIG2_OUTPUTBUFFER_SIZE);
    j += JBIG2_OUTPUTBUFFER_SIZE;
  }

  memcpy(&buffer[j], ctx->outbuf, ctx->outbuf_used);
}

// This is the context used for the TPGD bits
#define TPGDCTX 0x9b25

// -----------------------------------------------------------------------------
// This is designed for Leptonica's 1bpp packed format images. Each row is some
// number of 32-bit words. Pixels are in native-byte-order in each word.
// -----------------------------------------------------------------------------
void
jbig2enc_bitimage(struct jbig2enc_ctx *restrict ctx, const u8 *restrict idata,
                  int mx, int my, bool duplicate_line_removal) {
  const u32 *restrict data = (u32 *) idata;
  u8 *const context = ctx->context;
  const unsigned words_per_row = (mx + 31) / 32;
  const unsigned bytes_per_row = words_per_row * 4;

  u8 ltp = 0, sltp = 0;

  for (int y = 0; y < my; ++y) {
    int x = 0;

    // the c* values store the context bits for each row. The template is fixed
    // as template 0 with the floating bits in the default locations.
    u16 c1, c2, c3;
    // the w* values contain words from each of the rows: w1 is from two rows
    // up etc. The next bit to roll onto the context values are kept at the top
    // of these words.
    u32 w1, w2, w3;
    w1 = w2 = w3 = 0;

    if (y >= 2) w1 = data[(y - 2) * words_per_row];
    if (y >= 1) {
      w2 = data[(y - 1) * words_per_row];

      if (duplicate_line_removal) {
        // it's possible that the last row was the same as this row
        if (memcmp(&data[y * words_per_row], &data[(y - 1) * words_per_row],
                   bytes_per_row) == 0) {
          sltp = ltp ^ 1;
          ltp = 1;
        } else {
          sltp = ltp;
          ltp = 0;
        }
      }
    }
    if (duplicate_line_removal) {
      encode_bit(ctx, context, TPGDCTX, sltp);
      if (ltp) continue;
    }
    w3 = data[y * words_per_row];

    // the top three bits are the start of the context c1
    c1 = w1 >> 29;
    c2 = w2 >> 28;
    // and we need to remove the used bits from the w* vars
    w1 <<= 3;
    w2 <<= 4;
    c3 = 0;
    for (x = 0; x < mx; ++x) {
      const u16 tval = (c1 << 11) | (c2 << 4) | c3;
      const u8 v = (w3 & 0x80000000) >> 31;

      //fprintf(stderr, "%d %d %d %d\n", x, y, tval, v);
      encode_bit(ctx, context, tval, v);
      c1 <<= 1;
      c2 <<= 1;
      c3 <<= 1;
      c1 |= (w1 & 0x80000000) >> 31;
      c2 |= (w2 & 0x80000000) >> 31;
      c3 |= v;
      const int m = x % 32;
      if (m == 28 && y >= 2) {
        // need to roll in another word from two lines up
        const unsigned wordno = (x / 32) + 1;
        if (wordno >= words_per_row) {
          w1 = 0;
        } else {
          w1 = data[(y - 2) * words_per_row + wordno];
        }
      } else {
        w1 <<= 1;
      }

      if (m == 27 && y >= 1) {
        // need to roll in another word from the last line
        const unsigned wordno = (x / 32) + 1;
        if (wordno >= words_per_row) {
          w2 = 0;
        } else {
          w2 = data[(y - 1) * words_per_row + wordno];
        }
      } else {
        w2 <<= 1;
      }

      if (m == 31) {
        // need to roll in another word from this line
        const unsigned wordno = (x / 32) + 1;
        if (wordno >= words_per_row) {
          w3 = 0;
        } else {
          w3 = data[y * words_per_row + wordno];
        }
      } else {
        w3 <<= 1;
      }

      c1 &= 31;
      c2 &= 127;
      c3 &= 15;
    }
  }
}
