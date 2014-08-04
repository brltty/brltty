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

typedef struct KeyboardMonitorExtensionStruct KeyboardMonitorExtension;

struct KeyboardMonitorDataStruct {
  KeyboardMonitorExtension *kmx;
  unsigned isActive:1;

  KeyboardProperties requiredProperties;
  Queue *instanceQueue;

  KeyEventHandler *handleKeyEvent;
};

typedef struct {
  int code;
  int press;
} KeyEventEntry;

typedef struct KeyboardInstanceExtensionStruct KeyboardInstanceExtension;

typedef struct {
  KeyboardMonitorData *kmd;
  KeyboardInstanceExtension *kix;

  KeyboardProperties actualProperties;

  struct {
    KeyEventEntry *buffer;
    unsigned int size;
    unsigned int count;
  } events;

  struct {
    unsigned modifiersOnly:1;
    unsigned int size;
    unsigned char mask[0];
  } deferred;
} KeyboardInstanceData;

extern void handleKeyEvent (KeyboardInstanceData *kid, int code, int press);

extern KeyboardInstanceData *newKeyboardInstance (KeyboardMonitorData *kmd);
extern void destroyKeyboardInstance (KeyboardInstanceData *kid);

extern int monitorKeyboards (KeyboardMonitorData *kmd);
extern int forwardKeyEvent (KeyboardInstanceData *kid, int code, int press);

extern int newKeyboardMonitorExtension (KeyboardMonitorExtension **kmx);
extern void destroyKeyboardMonitorExtension (KeyboardMonitorExtension *kmx);

extern int newKeyboardInstanceExtension (KeyboardInstanceExtension **kix);
extern void destroyKeyboardInstanceExtension (KeyboardInstanceExtension *kix);

extern const KeyValue keyCodeMap[];
extern const unsigned int keyCodeCount;

#define BEGIN_KEY_CODE_MAP const KeyValue keyCodeMap[] = {
#define END_KEY_CODE_MAP }; const unsigned int keyCodeCount = ARRAY_COUNT(keyCodeMap);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BRLTTY_INCLUDED_KEYBOARD_INTERNAL */
