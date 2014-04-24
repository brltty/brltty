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

static inline uint16_t
getNativeEndian16 (uint16_t from) {
  return from;
}

static inline uint16_t
getOtherEndian16 (uint16_t from) {
  const unsigned char *bytes = (const unsigned char *)&from;

  return ((uint16_t)bytes[0] << 0x08)
       | ((uint16_t)bytes[1] << 0x00);
}

static inline void
putNativeEndian16 (uint16_t *to, uint16_t from) {
  *to = getNativeEndian16(from);
}

static inline void
putOtherEndian16 (uint16_t *to, uint16_t from) {
  *to = getOtherEndian16(from);
}

static inline uint32_t
getNativeEndian32 (uint32_t from) {
  return from;
}

static inline uint32_t
getOtherEndian32 (uint32_t from) {
  const unsigned char *bytes = (const unsigned char *)&from;

  return ((uint32_t)bytes[0] << 0x18)
       | ((uint32_t)bytes[1] << 0x10)
       | ((uint32_t)bytes[2] << 0x08)
       | ((uint32_t)bytes[3] << 0x00);
}

static inline void
putNativeEndian32 (uint32_t *to, uint32_t from) {
  *to = getNativeEndian32(from);
}

static inline void
putOtherEndian32 (uint32_t *to, uint32_t from) {
  *to = getOtherEndian32(from);
}

static inline uint64_t
getNativeEndian64 (uint64_t from) {
  return from;
}

static inline uint64_t
getOtherEndian64 (uint64_t from) {
  const unsigned char *bytes = (const unsigned char *)&from;

  return ((uint64_t)bytes[0] << 0x38)
       | ((uint64_t)bytes[1] << 0x30)
       | ((uint64_t)bytes[2] << 0x28)
       | ((uint64_t)bytes[3] << 0x20)
       | ((uint64_t)bytes[4] << 0x18)
       | ((uint64_t)bytes[5] << 0x10)
       | ((uint64_t)bytes[6] << 0x08)
       | ((uint64_t)bytes[7] << 0x00);
}

static inline void
putNativeEndian64 (uint64_t *to, uint64_t from) {
  *to = getNativeEndian64(from);
}

static inline void
putOtherEndian64 (uint64_t *to, uint64_t from) {
  *to = getOtherEndian64(from);
}

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

static inline void
swapBytes (unsigned char *byte1, unsigned char *byte2) {
  unsigned char byte = *byte1;
  *byte1 = *byte2;
  *byte2 = byte;
}

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BRLTTY_INCLUDED_BITFIELD */
