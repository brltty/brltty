/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2010 by The BRLTTY Developers.
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

#define getSameEndian(from) (from)
#define getOtherEndian(from) ((((from) & 0XFF) << 8) | (((from) >> 8) & 0XFF))

#define putSameEndian(to, from) (*(to) = getSameEndian((from)))
#define putOtherEndian(to, from) putSameEndian((to), getOtherEndian((from)))

#ifdef WORDS_BIGENDIAN
#  define getLittleEndian getOtherEndian
#  define putLittleEndian putOtherEndian

#  define getBigEndian getSameEndian
#  define putBigEndian putSameEndian
#else /* WORDS_BIGENDIAN */
#  define getLittleEndian getSameEndian
#  define putLittleEndian putSameEndian

#  define getBigEndian getOtherEndian
#  define putBigEndian putOtherEndian
#endif /* WORDS_BIGENDIAN */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BRLTTY_INCLUDED_BITFIELD */
