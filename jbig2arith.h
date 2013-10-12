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

#ifndef JBIG2ENC_JBIG2ENC_H__
#define JBIG2ENC_JBIG2ENC_H__

#if defined(sun)
#include <sys/types.h>
#else
#include <stdint.h>
#endif

#include "jbvector.h"

#define JBIG2_MAX_CTX 65536
#define JBIG2_OUTPUTBUFFER_SIZE 20 * 1024

#ifdef _MSC_VER
#define __restrict__ __restrict
#endif

//#define JBIG2_DEBUGGING
//#define CODER_DEBUGGING
//#define SYM_DEBUGGING
//#define SYMBOL_COMPRESSION_DEBUGGING

// -----------------------------------------------------------------------------
// This is the context for the arithmetic encoder used in JBIG2. The coder is a
// state machine and there are many different states used - one for coding
// images, many more for coding numbers etc.
//
// When outputting data, the bytes are collected into chunks of size
// JBIG2_OUTPUTBUFFER_SIZE. These are chained in a linked list.
// -----------------------------------------------------------------------------
struct jbig2enc_ctx {
  // these are the current state of the arithmetic coder
  uint32_t c;
  uint16_t a;
  uint8_t ct, b;
  int bp;

  // This is a list of output chunks, not including the current one
  jbvector<uint8_t *> *output_chunks;
  uint8_t *outbuf;  // this is the current output chunk
  int outbuf_used;  // number of bytes used in outbuf
  uint8_t context[JBIG2_MAX_CTX];  // state machine context for encoding images
  uint8_t intctx[13][512];  // 512 bytes of context indexes for each of 13 different int decodings
                            // this data is also used for refinement coding
  uint8_t *iaidctx;  // size of this context not known at construction time
};

// these are the proc numbers for encoding different classes of integers
enum {
  JBIG2_IAAI = 0,
  JBIG2_IADH,
  JBIG2_IADS,
  JBIG2_IADT,
  JBIG2_IADW,
  JBIG2_IAEX,
  JBIG2_IAFS,
  JBIG2_IAIT,
  JBIG2_IARDH,
  JBIG2_IARDW,
  JBIG2_IARDX,
  JBIG2_IARDY,
  JBIG2_IARI
};

// -----------------------------------------------------------------------------
// Returns the number of bytes of output in the given context
//
// Before doing this you should make sure that the coder is _flush()'ed
// -----------------------------------------------------------------------------
unsigned jbig2enc_datasize(const struct jbig2enc_ctx *ctx);

// -----------------------------------------------------------------------------
// Writes the output of the given context to a buffer. The buffer must be at
// least long enough to contain all the data (see _datasize)
// -----------------------------------------------------------------------------
void jbig2enc_tobuffer(const struct jbig2enc_ctx *__restrict__ ctx,
                       uint8_t *__restrict__ buffer);

// -----------------------------------------------------------------------------
// This function takes almost the same arguments as _image, above. But in this
// case the data pointer points to packed data.
//
// This is designed for Leptonica's 1bpp packed format images. Each row is some
// number of 32-bit words.
//
// *The pad bits at the end of each line must be zero.*
// -----------------------------------------------------------------------------
void jbig2enc_bitimage(struct jbig2enc_ctx *__restrict__ ctx,
                       const uint8_t *__restrict__ data, int mx, int my,
                       bool duplicate_line_removal);


// -----------------------------------------------------------------------------
// Init a new context
// -----------------------------------------------------------------------------
void jbig2enc_init(struct jbig2enc_ctx *ctx);

// -----------------------------------------------------------------------------
// Destroy a context
// -----------------------------------------------------------------------------
void jbig2enc_dealloc(struct jbig2enc_ctx *ctx);

// -----------------------------------------------------------------------------
// Flush any remaining arithmetic encoder context to the output.
// -----------------------------------------------------------------------------
void jbig2enc_final(struct jbig2enc_ctx *ctx);

#endif  // EXPERIMENTAL_USERS_AGL_JBIG2ENC_JBIG2ENC_H__
