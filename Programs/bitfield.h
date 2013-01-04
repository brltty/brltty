/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2013 by The BRLTTY Developers.
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
getSameEndian16 (uint16_t from) {
  return from;
}

static inline uint16_t
getOtherEndian16 (uint16_t from) {
  const unsigned char *bytes = (const unsigned char *)&from;

  return (bytes[0] << 0x08)
       | (bytes[1] << 0x00);
}

static inline void
putSameEndian16 (uint16_t *to, uint16_t from) {
  *to = getSameEndian16(from);
}

static inline void
putOtherEndian16 (uint16_t *to, uint16_t from) {
  *to = getOtherEndian16(from);
}

static inline uint32_t
getSameEndian32 (uint32_t from) {
  return from;
}

static inline uint32_t
getOtherEndian32 (uint32_t from) {
  const unsigned char *bytes = (const unsigned char *)&from;

  return (bytes[0] << 0x18)
       | (bytes[1] << 0x10)
       | (bytes[2] << 0x08)
       | (bytes[3] << 0x00);
}

static inline void
putSameEndian32 (uint32_t *to, uint32_t from) {
  *to = getSameEndian32(from);
}

static inline void
putOtherEndian32 (uint32_t *to, uint32_t from) {
  *to = getOtherEndian32(from);
}

#ifdef WORDS_BIGENDIAN
#  define getLittleEndian16 getOtherEndian16
#  define putLittleEndian16 putOtherEndian16

#  define getBigEndian16 getSameEndian16
#  define putBigEndian16 putSameEndian16

#  define getLittleEndian32 getOtherEndian32
#  define putLittleEndian32 putOtherEndian32

#  define getBigEndian32 getSameEndian32
#  define putBigEndian32 putSameEndian32

#else /* WORDS_BIGENDIAN */
#  define getLittleEndian16 getSameEndian16
#  define putLittleEndian16 putSameEndian16

#  define getBigEndian16 getOtherEndian16
#  define putBigEndian16 putOtherEndian16

#  define getLittleEndian32 getSameEndian32
#  define putLittleEndian32 putSameEndian32

#  define getBigEndian32 getOtherEndian32
#  define putBigEndian32 putOtherEndian32

#endif /* WORDS_BIGENDIAN */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BRLTTY_INCLUDED_BITFIELD */
