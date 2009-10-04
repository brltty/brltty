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
  KBF_Dot1,
  KBF_Dot2,
  KBF_Dot3,
  KBF_Dot4,
  KBF_Dot5,
  KBF_Dot6,
  KBF_Dot7,
  KBF_Dot8,

  KBF_Space,
  KBF_Shift,
  KBF_Uppercase,
  KBF_Control,
  KBF_Meta,

  KBF_None /* must be last */
} KeyboardFunction;

typedef struct {
  const char *name;
  int bit;
} KeyboardFunctionEntry;

extern const KeyboardFunctionEntry keyboardFunctionTable[KBF_None];

typedef struct {
  struct {
    KeySetMask mask;
    unsigned char count;
    unsigned char keys[10];
  } modifiers;

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

  unsigned char *keyMap;
  int superimposedBits;
} KeyContext;

struct KeyTableStruct {
  wchar_t *title;

  wchar_t **noteTable;
  unsigned int noteCount;

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
