/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2026 by The BRLTTY Developers.
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

#ifndef BRLTTY_INCLUDED_SCR_EMULATOR
#define BRLTTY_INCLUDED_SCR_EMULATOR

#include "scr_terminal.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

extern void moveScreenCharacters (ScreenSegmentCharacter *to, const ScreenSegmentCharacter *from, size_t count);
extern void setScreenCharacters (ScreenSegmentCharacter *from, const ScreenSegmentCharacter *to, const ScreenSegmentCharacter *character);
extern void propagateScreenCharacter (ScreenSegmentCharacter *from, const ScreenSegmentCharacter *to);

typedef enum {
  SCI_OFF = 0X00,
  SCI_DIM = 0X55,
  SCI_REG = 0XAA,
  SCI_MAX = 0XFF,
} ScreenColorIntensity;

#define SCREEN_SEGMENT_COLOR(r, g, b) {.red=r, .green=g, .blue=b}
#define SCREEN_SEGMENT_COLOR_BLACK SCREEN_SEGMENT_COLOR(SCI_OFF, SCI_OFF, SCI_OFF)
#define SCREEN_SEGMENT_COLOR_WHITE SCREEN_SEGMENT_COLOR(SCI_REG, SCI_REG, SCI_REG)

extern void fillScreenRows (ScreenSegmentHeader *segment, unsigned int row, unsigned int count, const ScreenSegmentCharacter *character);
extern void moveScreenRows (ScreenSegmentHeader *segment, unsigned int from, unsigned int to, unsigned int count);
extern void scrollScreenRows (ScreenSegmentHeader *segment, unsigned int top, unsigned int size, unsigned int count, int down);

extern ScreenSegmentHeader *createScreenSegment (int *identifier, key_t key, int height, int width, int enableRowArray);
extern int destroyScreenSegment (int identifier);

extern int createMessageQueue (int *queue, key_t key);
extern int destroyMessageQueue (int queue);

/* SysV IPC objects (message queues, shared-memory segments) have no "die with
 * owning process" semantics and macOS's system-wide limits on them are tiny
 * (tens of messages/queues total). A terminal emulator killed uncleanly (a
 * crash, a force-quit, SIGKILL) leaks its queue and segment, and enough leaks
 * exhaust the whole system - after which unrelated, perfectly healthy
 * processes start blocking forever on IPC calls that have nothing to do with
 * the leaked ones. Since macOS has no portable way to enumerate all SysV IDs
 * on the system, each emulator instead registers what it created in a small
 * file-based registry and reaps entries left behind by dead PIDs at the start
 * of every new instance. */
extern void reapStaleTerminalResources (void);
extern void registerTerminalResources (int screenSegmentIdentifier, int messageQueueIdentifier);
extern void unregisterTerminalResources (void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BRLTTY_INCLUDED_SCR_EMULATOR */
