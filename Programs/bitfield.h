/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2014 by The BRLTTY Developers.
 *
 * BRLTTY comes with ABSOLUTELY NO WARRANTY.
 *
 * This is free software, placed under the terms of the
 * GNU General Public License, as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any
 * later version. Please see the file LICENSE-GPL for details.
 *
 * Web Page: http://mielke.cc/brltty/
 *
 * This software is maintained by Dave Mielke <dave@mielke.cc>.
 */

#ifndef BRLTTY_INCLUDED_BITFIELD
#define BRLTTY_INCLUDED_BITFIELD

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "prologue.h"

#define HIGH_NIBBLE(byte) ((byte) & 0XF0)
#define LOW_NIBBLE(byte) ((byte) & 0XF)

static inline void
swapBytes (unsigned char *byte1, unsigned char *byte2) {
  unsigned char byte = *byte1;
  *byte1 = *byte2;
  *byte2 = byte;
}

typedef union {
  uint8_t bytes[0];
  uint16_t u16;
  uint32_t u32;
  uint64_t u64;
} BytesOverlay;

#define DEFINE_PHYSICAL_ENDIAN_FUNCTIONS(bits) \
  static inline uint##bits##_t \
  getNativeEndian##bits (uint##bits##_t from) { \
    return from; \
  } \
  static inline uint##bits##_t \
  getOtherEndian##bits (uint##bits##_t from) { \
    BytesOverlay overlay = {.u##bits = from}; \
    uint8_t *first = overlay.bytes; \
    uint8_t *second = first + (bits / 8); \
    do { \
      swapBytes(first++, --second); \
    } while (first != second); \
    return overlay.u##bits; \
  } \
  static inline void \
  putNativeEndian##bits (uint##bits##_t *to, uint##bits##_t from) { \
    *to = getNativeEndian##bits(from); \
  } \
  static inline void \
  putOtherEndian##bits (uint##bits##_t *to, uint##bits##_t from) { \
    *to = getOtherEndian##bits(from); \
  }

DEFINE_PHYSICAL_ENDIAN_FUNCTIONS(16)
DEFINE_PHYSICAL_ENDIAN_FUNCTIONS(32)
DEFINE_PHYSICAL_ENDIAN_FUNCTIONS(64)

#ifdef WORDS_BIGENDIAN
#  define getLittleEndian16 getOtherEndian16
#  define putLittleEndian16 putOtherEndian16

#  define getBigEndian16 getNativeEndian16
#  define putBigEndian16 putNativeEndian16

#  define getLittleEndian32 getOtherEndian32
#  define putLittleEndian32 putOtherEndian32

#  define getBigEndian32 getNativeEndian32
#  define putBigEndian32 putNativeEndian32

#  define getLittleEndian64 getOtherEndian64
#  define putLittleEndian64 putOtherEndian64

#  define getBigEndian64 getNativeEndian64
#  define putBigEndian64 putNativeEndian64

#else /* WORDS_BIGENDIAN */
#  define getLittleEndian16 getNativeEndian16
#  define putLittleEndian16 putNativeEndian16

#  define getBigEndian16 getOtherEndian16
#  define putBigEndian16 putOtherEndian16

#  define getLittleEndian32 getNativeEndian32
#  define putLittleEndian32 putNativeEndian32

#  define getBigEndian32 getOtherEndian32
#  define putBigEndian32 putOtherEndian32

#  define getLittleEndian64 getNativeEndian64
#  define putLittleEndian64 putNativeEndian64

#  define getBigEndian64 getOtherEndian64
#  define putBigEndian64 putOtherEndian64

#endif /* WORDS_BIGENDIAN */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BRLTTY_INCLUDED_BITFIELD */
