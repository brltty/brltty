/*
 * BRLTTY - A background process providing access to the Linux console (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2003 by The BRLTTY Team. All rights reserved.
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

/*
 * unicode.h
 * $Id$
 * by Hans Schou
 */

typedef struct {
  unsigned int unum;
  const char *name;
} UnicodeEntry;

typedef struct {
  const char *name;
  const char *desc;
  unsigned int table[0X100];
} CodePage;

extern const UnicodeEntry *getUnicodeEntry (unsigned int unum);
extern const CodePage *getCodePage (const char *name);

extern const CodePage *const codePageTable[];
extern const unsigned int codePageCount;
