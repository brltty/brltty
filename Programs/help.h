/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2006 by The BRLTTY Developers.
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

#ifndef BRLTTY_INCLUDED_HELP
#define BRLTTY_INCLUDED_HELP

#include "prologue.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <inttypes.h>

/* help.h - describes the helpfile format
 * $Id: help.h,v 1.3 1996/09/24 01:04:26 nn201 Exp $
 */

/* The compiled helpfile (brlttydev.hlp) has the following structure:
 *   1. number of pages (short int numpages)
 *   2. page info table: array of numpages pageinfo objects (described below)
 *   3. numpages blocks of help data, space padded to give a constant line
 *      length, no line termination character
 */

typedef struct {
  unsigned char pages;
  unsigned char unused;
} PACKED HelpFileHeader;

typedef struct {
  uint16_t height;
  uint16_t width;
} PACKED HelpPageEntry;

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BRLTTY_INCLUDED_HELP */
