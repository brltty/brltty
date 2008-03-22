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

#ifndef BRLTTY_INCLUDED_CTB
#define BRLTTY_INCLUDED_CTB

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef struct ContractionTableStruct ContractionTable;

#define CTB_NO_OFFSET -1
#define CTB_NO_CURSOR -1

extern ContractionTable *compileContractionTable (const char *fileName);
extern int destroyContractionTable (ContractionTable *contractionTable);
extern int contractText (
  ContractionTable *contractionTable, /* Pointer to translation table */
  const wchar_t *inputBuffer, /* What is to be translated */
  int *inputLength, /* Its length */
  unsigned char *outputBuffer, /* Where the translation is to go */
  int *outputLength, /* length of this area */
  int *offsetsMap, /* Array of offsets of translated chars in source */
  int cursorOffset /* Position of coursor in source */
);

extern void fixContractionTablePath (char **path);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BRLTTY_INCLUDED_CTB */
