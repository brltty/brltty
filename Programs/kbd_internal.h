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

#ifndef BRLTTY_INCLUDED_KBD_INTERNAL
#define BRLTTY_INCLUDED_KBD_INTERNAL

#include "ktb_keyboard.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef struct {
  KeyEventHandler *handleKeyEvent;
  KeyboardProperties requiredProperties;
} KeyboardCommonData;

typedef struct {
  int code;
  int press;
} KeyEventEntry;

typedef struct {
  KeyboardCommonData *kcd;
  KeyboardProperties actualProperties;

  KeyEventEntry *keyEventBuffer;
  unsigned int keyEventLimit;
  unsigned int keyEventCount;

  unsigned justModifiers:1;

  unsigned int handledKeysSize;
  unsigned char handledKeysMask[1];
} KeyboardInstanceData;

extern void handleKeyEvent (KeyboardInstanceData *kid, int code, int press);
extern KeyboardInstanceData *newKeyboardInstanceData (KeyboardCommonData *kcd);
extern void deallocateKeyboardInstanceData (KeyboardInstanceData *kid);

extern int monitorKeyboards (KeyboardCommonData *kcd);
extern int forwardKeyEvent (int code, int press);

extern const KeyValue keyCodeMap[];
extern const int keyCodeLimit;

#define BEGIN_KEY_CODE_MAP const KeyValue keyCodeMap[] = {
#define END_KEY_CODE_MAP }; const int keyCodeLimit = ARRAY_COUNT(keyCodeMap);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BRLTTY_INCLUDED_KBD_INTERNAL */
