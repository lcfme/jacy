// Copyright 2011 Google Inc. All Rights Reserved.
//
// Use of this source code is governed by a BSD-style license
// that can be found in the COPYING file in the root of the source
// tree. An additional intellectual property rights grant can be found
// in the file PATENTS. All contributing project authors may
// be found in the AUTHORS file in the root of the source tree.
// -----------------------------------------------------------------------------
//
// Bit writing and boolean coder
//
// Author: Skal (pascal.massimino@gmail.com)

#ifndef WEBP_UTILS_BIT_WRITER_H_
#define WEBP_UTILS_BIT_WRITER_H_

#include "../webp/types.h"

#ifdef __cplusplus
extern "C" {
#endif

//------------------------------------------------------------------------------
// Bit-writing

typedef struct DEDUP_vP8_BitWriter DEDUP_vP8_BitWriter;
struct DEDUP_vP8_BitWriter {
  int32_t  range_;      // range-1
  int32_t  value_;
  int      run_;        // number of outstanding bits
  int      nb_bits_;    // number of pending bits
  uint8_t* buf_;        // internal buffer. Re-allocated regularly. Not owned.
  size_t   pos_;
  size_t   max_pos_;
  int      error_;      // true in case of error
};

// Initialize the object. Allocates some initial memory based on expected_size.
int DEDUP_vP8_BitWriterInit(DEDUP_vP8_BitWriter* const bw, size_t expected_size);
// Finalize the bitstream coding. Returns a pointer to the internal buffer.
uint8_t* DEDUP_vP8_BitWriterFinish(DEDUP_vP8_BitWriter* const bw);
// Release any pending memory and zeroes the object. Not a mandatory call.
// Only useful in case of error, when the internal buffer hasn't been grabbed!
void DEDUP_vP8_BitWriterWipeOut(DEDUP_vP8_BitWriter* const bw);

int DEDUP_vP8_PutBit(DEDUP_vP8_BitWriter* const bw, int bit, int prob);
int DEDUP_vP8_PutBitUniform(DEDUP_vP8_BitWriter* const bw, int bit);
void DEDUP_vP8_PutBits(DEDUP_vP8_BitWriter* const bw, uint32_t value, int nb_bits);
void DEDUP_vP8_PutSignedBits(DEDUP_vP8_BitWriter* const bw, int value, int nb_bits);

// Appends some bytes to the internal buffer. Data is copied.
int DEDUP_vP8_BitWriterAppend(DEDUP_vP8_BitWriter* const bw,
                       const uint8_t* data, size_t size);

// return approximate write position (in bits)
static WEBP_INLINE uint64_t DEDUP_vP8_BitWriterPos(const DEDUP_vP8_BitWriter* const bw) {
  const uint64_t nb_bits = 8 + bw->nb_bits_;   // bw->nb_bits_ is <= 0, note
  return (bw->pos_ + bw->run_) * 8 + nb_bits;
}

// Returns a pointer to the internal buffer.
static WEBP_INLINE uint8_t* DEDUP_vP8_BitWriterBuf(const DEDUP_vP8_BitWriter* const bw) {
  return bw->buf_;
}
// Returns the size of the internal buffer.
static WEBP_INLINE size_t DEDUP_vP8_BitWriterSize(const DEDUP_vP8_BitWriter* const bw) {
  return bw->pos_;
}

//------------------------------------------------------------------------------
// DEDUP_vP8_LBitWriter

#if defined(__x86_64__) || defined(_M_X64)   // 64bit
typedef uint64_t vp8l_atype_t;   // accumulator type
typedef uint32_t vp8l_wtype_t;   // writing type
#define WSWAP HToLE32
#define DEDUP_vP8_L_WRITER_BYTES    4   // sizeof(vp8l_wtype_t)
#define DEDUP_vP8_L_WRITER_BITS     32  // 8 * sizeof(vp8l_wtype_t)
#define DEDUP_vP8_L_WRITER_MAX_BITS 64  // 8 * sizeof(vp8l_atype_t)
#else
typedef uint32_t vp8l_atype_t;
typedef uint16_t vp8l_wtype_t;
#define WSWAP HToLE16
#define DEDUP_vP8_L_WRITER_BYTES    2
#define DEDUP_vP8_L_WRITER_BITS     16
#define DEDUP_vP8_L_WRITER_MAX_BITS 32
#endif

typedef struct {
  vp8l_atype_t bits_;   // bit accumulator
  int          used_;   // number of bits used in accumulator
  uint8_t*     buf_;    // start of buffer
  uint8_t*     cur_;    // current write position
  uint8_t*     end_;    // end of buffer

  // After all bits are written (DEDUP_vP8_LBitWriterFinish()), the caller must observe
  // the state of error_. A value of 1 indicates that a memory allocation
  // failure has happened during bit writing. A value of 0 indicates successful
  // writing of bits.
  int error_;
} DEDUP_vP8_LBitWriter;

static WEBP_INLINE size_t DEDUP_vP8_LBitWriterNumBytes(DEDUP_vP8_LBitWriter* const bw) {
  return (bw->cur_ - bw->buf_) + ((bw->used_ + 7) >> 3);
}

// Returns false in case of memory allocation error.
int DEDUP_vP8_LBitWriterInit(DEDUP_vP8_LBitWriter* const bw, size_t expected_size);
// Finalize the bitstream coding. Returns a pointer to the internal buffer.
uint8_t* DEDUP_vP8_LBitWriterFinish(DEDUP_vP8_LBitWriter* const bw);
// Release any pending memory and zeroes the object.
void DEDUP_vP8_LBitWriterWipeOut(DEDUP_vP8_LBitWriter* const bw);

// Internal function for DEDUP_vP8_LPutBits flushing 32 bits from the written state.
void DEDUP_vP8_LPutBitsFlushBits(DEDUP_vP8_LBitWriter* const bw);

// PutBits internal function used in the 16 bit vp8l_wtype_t case.
void DEDUP_vP8_LPutBitsInternal(DEDUP_vP8_LBitWriter* const bw, uint32_t bits, int n_bits);

// This function writes bits into bytes in increasing addresses (little endian),
// and within a byte least-significant-bit first.
// This function can write up to 32 bits in one go, but DEDUP_vP8_LBitReader can only
// read 24 bits max (DEDUP_vP8_L_MAX_NUM_BIT_READ).
// DEDUP_vP8_LBitWriter's error_ flag is set in case of  memory allocation error.
static WEBP_INLINE void DEDUP_vP8_LPutBits(DEDUP_vP8_LBitWriter* const bw,
                                    uint32_t bits, int n_bits) {
  if (sizeof(vp8l_wtype_t) == 4) {
    if (n_bits > 0) {
      if (bw->used_ >= 32) {
        DEDUP_vP8_LPutBitsFlushBits(bw);
      }
      bw->bits_ |= (vp8l_atype_t)bits << bw->used_;
      bw->used_ += n_bits;
    }
  } else {
    DEDUP_vP8_LPutBitsInternal(bw, bits, n_bits);
  }
}

//------------------------------------------------------------------------------

#ifdef __cplusplus
}    // extern "C"
#endif

#endif  /* WEBP_UTILS_BIT_WRITER_H_ */
