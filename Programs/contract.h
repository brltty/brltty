/*
 * BRLTTY - A background process providing access to the Linux console (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2004 by The BRLTTY Team. All rights reserved.
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

#ifndef _CONTRACT_H
#define _CONTRACT_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

extern void *compileContractionTable (const char *fileName);
extern int destroyContractionTable (void *contractionTable);
extern int contractText (
  void *contractionTable, /* Pointer to translation table */
  const unsigned char *inputBuffer, /* What is to be translated */
  int *inputLength, /* Its length */
  unsigned char *outputBuffer, /* Where the translation is to go */
  int *outputLength, /* length of this area */
  int *offsetsMap, /* Array of offsets of translated chars in source */
  int cursorOffset /* Position of coursor in source */
);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _CONTRACT_H */
