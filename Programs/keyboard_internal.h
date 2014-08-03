/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2014 by The BRLTTY Developers.
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

#ifndef BRLTTY_INCLUDED_KEYBOARD_INTERNAL
#define BRLTTY_INCLUDED_KEYBOARD_INTERNAL

#include "queue.h"
#include "ktb_keyboard.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef struct {
  unsigned int referenceCount;
  unsigned isActive:1;
  Queue *instanceQueue;
  KeyEventHandler *handleKeyEvent;
  KeyboardProperties requiredProperties;
} KeyboardCommonData;

typedef struct {
  int code;
  int press;
} KeyEventEntry;

typedef struct KeyboardPlatformDataStruct KeyboardPlatformData;

typedef struct {
  KeyboardCommonData *kcd;
  KeyboardPlatformData *kpd;

  KeyboardProperties actualProperties;

  struct {
    KeyEventEntry *buffer;
    unsigned int size;
    unsigned int count;
  } events;

  unsigned justModifiers:1;

  struct {
    unsigned int size;
    unsigned char mask[0];
  } deferred;
} KeyboardInstanceData;

extern void handleKeyEvent (KeyboardInstanceData *kid, int code, int press);
extern void claimKeyboardCommonData (KeyboardCommonData *kcd);
extern void releaseKeyboardCommonData (KeyboardCommonData *kcd);
extern KeyboardInstanceData *newKeyboardInstanceData (KeyboardCommonData *kcd);
extern void deallocateKeyboardInstanceData (KeyboardInstanceData *kid);

extern int monitorKeyboards (KeyboardCommonData *kcd);
extern int forwardKeyEvent (int code, int press);
extern void deallocateKeyboardPlatformData (KeyboardPlatformData *kpd);

extern const KeyValue keyCodeMap[];
extern const int keyCodeLimit;

#define BEGIN_KEY_CODE_MAP const KeyValue keyCodeMap[] = {
#define END_KEY_CODE_MAP }; const int keyCodeLimit = ARRAY_COUNT(keyCodeMap);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BRLTTY_INCLUDED_KEYBOARD_INTERNAL */
