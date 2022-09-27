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

#ifndef BRLTTY_INCLUDED_SCR_TERMINAL
#define BRLTTY_INCLUDED_SCR_TERMINAL

#include <sys/ipc.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

extern int makeTerminalKey (key_t *key, const char *path);

typedef enum {
  TERMINAL_MESSAGE_INPUT_TEXT      = 't', // driver->emulator - UTF-8
  TERMINAL_MESSAGE_SEGMENT_UPDATED = 'u', // emulator->driver - no content
} TerminalMessageType;

extern int getMessageQueue (int *queue, key_t key);

typedef struct {
  unsigned char red;
  unsigned char green;
  unsigned char blue;
} ScreenSegmentColor;

typedef struct {
  uint32_t text;

  ScreenSegmentColor foreground;
  ScreenSegmentColor background;

  unsigned char blink:1;
  unsigned char underline:1;
} ScreenSegmentCharacter;

typedef struct {
  unsigned int headerSize;
  unsigned int segmentSize;

  unsigned int characterSize;
  unsigned int charactersOffset;

  unsigned int screenHeight;
  unsigned int screenWidth;

  unsigned int cursorRow;
  unsigned int cursorColumn;
} ScreenSegmentHeader;

extern int getScreenSegment (int *identifier, key_t key);
extern ScreenSegmentHeader *attachScreenSegment (int identifier);
extern int detachScreenSegment (ScreenSegmentHeader *segment);

extern ScreenSegmentHeader *getScreenSegmentForKey (key_t key);
extern ScreenSegmentHeader *getScreenSegmentForPath (const char *path);

extern ScreenSegmentCharacter *getScreenStart (ScreenSegmentHeader *segment);
extern ScreenSegmentCharacter *getScreenRow (ScreenSegmentHeader *segment, unsigned int row, ScreenSegmentCharacter **end);
extern ScreenSegmentCharacter *getScreenCharacter (ScreenSegmentHeader *segment, unsigned int row, unsigned int column, ScreenSegmentCharacter **end);
extern const ScreenSegmentCharacter *getScreenEnd (ScreenSegmentHeader *segment);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BRLTTY_INCLUDED_SCR_TERMINAL */
