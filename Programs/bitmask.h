/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2008 by The BRLTTY Developers.
 *
 * BRLTTY comes with ABSOLUTELY NO WARRANTY.
 *
 * This is free software, placed under the terms of the
 * GNU General Public License, as published by the Free Software
 * Foundation.  Please see the file COPYING for details.
 *
 * Web Page: http://mielke.cc/brltty/
 *
 * This software is maintained by Dave Mielke <dave@mielke.cc>.
 */

#ifndef BRLTTY_INCLUDED_BITMASK
#define BRLTTY_INCLUDED_BITMASK

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* These macros are meant for internal use only. */
#define BITMASK_ELEMENT_SIZE(element) (sizeof(element) * 8)
#define BITMASK_INDEX(bit,size) ((bit) / (size))
#define BITMASK_SHIFT(bit,size) ((bit) % (size))
#define BITMASK_ELEMENT_COUNT(bits,size) (BITMASK_INDEX((bits)-1, (size)) + 1)
#define BITMASK_ELEMENT(name,bit) ((name)[BITMASK_INDEX((bit), BITMASK_ELEMENT_SIZE((name)[0]))])
#define BITMASK_BIT(name,bit) (1 << BITMASK_SHIFT((bit), BITMASK_ELEMENT_SIZE((name)[0])))

/* These macros are for public use. */
#define BITMASK(name,bits,type) unsigned type name[BITMASK_ELEMENT_COUNT((bits), BITMASK_ELEMENT_SIZE(type))]
#define BITMASK_SIZE(name) BITMASK_ELEMENT_SIZE((name))
#define BITMASK_SET(name,bit) (BITMASK_ELEMENT((name), (bit)) |= BITMASK_BIT((name), (bit)))
#define BITMASK_CLEAR(name,bit) (BITMASK_ELEMENT((name), (bit)) &= ~BITMASK_BIT((name), (bit)))
#define BITMASK_TEST(name,bit) (BITMASK_ELEMENT((name), (bit)) & BITMASK_BIT((name), (bit)))

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BRLTTY_INCLUDED_BITMASK */
