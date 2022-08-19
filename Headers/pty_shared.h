/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2022 by The BRLTTY Developers.
 *
 * BRLTTY comes with ABSOLUTELY NO WARRANTY.
 *
 * This is free software, placed under the terms of the
 * GNU Lesser General Public License, as published by the Free Software
 * Foundation; either version 2.1 of the License, or (at your option) any
 * later version. Please see the file LICENSE-LGPL for details.
 *
 * Web Page: http://brltty.app/
 *
 * This software is maintained by Dave Mielke <dave@mielke.cc>.
 */

#ifndef BRLTTY_INCLUDED_PTY_SHARED
#define BRLTTY_INCLUDED_PTY_SHARED

#include <sys/shm.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef struct {
  wchar_t text;

  unsigned char foreground;
  unsigned char background;

  unsigned char blink:1;
  unsigned char bold:1;
  unsigned char underline:1;
  unsigned char reverse:1;
  unsigned char standout:1;
  unsigned char dim:1;
} PtyCharacter;

typedef struct {
  unsigned int headerSize;
  unsigned int segmentSize;

  unsigned int characterSize;
  unsigned int charactersOffset;

  unsigned int screenHeight;
  unsigned int screenWidth;

  unsigned int cursorRow;
  unsigned int cursorColumn;
} PtyHeader;

extern key_t ptyMakeSegmentKey (const char *tty);
extern int ptyGetSegmentIdentifier (key_t key, int *identifier);
extern void *ptyAttachSegment (int identifier);
extern void *ptyGetSegment (const char *tty);
extern int ptyDetachSegment (void *address);

extern PtyCharacter *ptyGetScreenStart (PtyHeader *header);
extern PtyCharacter *ptyGetRow (PtyHeader *header, unsigned int row, PtyCharacter **end);
extern PtyCharacter *ptyGetCharacter (PtyHeader *header, unsigned int row, unsigned int column, PtyCharacter **end);
extern const PtyCharacter *ptyGetScreenEnd (PtyHeader *header);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BRLTTY_INCLUDED_PTY_SHARED */
