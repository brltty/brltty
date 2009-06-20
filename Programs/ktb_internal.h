/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2009 by The BRLTTY Developers.
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

#ifndef BRLTTY_INCLUDED_KTB_INTERNAL
#define BRLTTY_INCLUDED_KTB_INTERNAL

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "keysets.h"

typedef enum {
  IFN_Space,

  IFN_Dot1,
  IFN_Dot2,
  IFN_Dot3,
  IFN_Dot4,
  IFN_Dot5,
  IFN_Dot6,
  IFN_Dot7,
  IFN_Dot8,

  IFN_Shift,
  IFN_Upper,
  IFN_Control,
  IFN_Meta,

  InputFunctionCount
} InputFunction;

typedef struct {
  const char *name;
  int bit;
} InputFunctionEntry;

extern const InputFunctionEntry inputFunctionTable[InputFunctionCount];

typedef struct {
  KeySetMask modifiers;
  unsigned char set;
  unsigned char key;
} KeyCombination;

typedef struct {
  int command;
  KeyCombination keys;
} KeyBinding;

typedef struct {
  wchar_t *name;

  KeyBinding *keyBindingTable;
  unsigned int keyBindingsSize;
  unsigned int keyBindingCount;
  const KeyBinding **sortedKeyBindings;

  unsigned char inputKeys[InputFunctionCount];
} KeyContext;

struct KeyTableStruct {
  wchar_t *title;

  const KeyNameEntry **keyNameTable;
  unsigned int keyNameCount;

  KeyContext *keyContextTable;
  unsigned int keyContextCount;

  unsigned char persistentContext;
  unsigned char currentContext;

  KeySet keys;
  int command;
  unsigned immediate:1;
};

extern int compareKeyBindings (const KeyBinding *binding1, const KeyBinding *binding2);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BRLTTY_INCLUDED_KTB_INTERNAL */
