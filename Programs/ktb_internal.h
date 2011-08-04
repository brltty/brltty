/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2011 by The BRLTTY Developers.
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

#define MAX_MODIFIERS_PER_COMBINATION 10

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

  KeyboardFunctionCount /* must be last */
} KeyboardFunction;

typedef struct {
  const char *name;
  int bit;
} KeyboardFunctionEntry;

extern const KeyboardFunctionEntry keyboardFunctionTable[KeyboardFunctionCount];

typedef enum {
  KCF_IMMEDIATE_KEY = 0X01
} KeyCombinationFlag;

typedef struct {
  unsigned char flags;
  unsigned char modifierCount;
  unsigned char modifierPositions[MAX_MODIFIERS_PER_COMBINATION];
  KeyValue modifierKeys[MAX_MODIFIERS_PER_COMBINATION];
  KeyValue immediateKey;
} KeyCombination;

typedef enum {
  KBF_HIDDEN = 0X01,
  KBF_OFFSET = 0X02,
  KBF_COLUMN = 0X04,
  KBF_ROW    = 0X08,
  KBF_RANGE  = 0X10,
  KBF_ROUTE  = 0X20
} KeyBindingFlag;

typedef struct {
  int command;
  KeyCombination combination;
  unsigned char flags;
} KeyBinding;

typedef enum {
  HKF_HIDDEN = 0X01
} HotkeyFlag;

typedef struct {
  KeyValue keyValue;
  int pressCommand;
  int releaseCommand;
  unsigned char flags;
} HotkeyEntry;

typedef struct {
  KeyValue keyValue;
  unsigned char keyboardFunction;
} MappedKeyEntry;

typedef struct {
  wchar_t *title;

  KeyBinding *keyBindingTable;
  unsigned int keyBindingsSize;
  unsigned int keyBindingCount;
  const KeyBinding **sortedKeyBindings;

  HotkeyEntry *hotkeyTable;
  unsigned int hotkeyCount;
  const HotkeyEntry **sortedHotkeyEntries;

  MappedKeyEntry *mappedKeyTable;
  unsigned int mappedKeyCount;
  const MappedKeyEntry **sortedMappedKeyEntries;
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

  KeyValue *pressedKeys;
  unsigned int pressedSize;
  unsigned int pressedCount;

  int command;
  unsigned immediate:1;
  unsigned logKeyEvents:1;
};

extern void copyKeyValues (KeyValue *target, const KeyValue *source, unsigned int count);
extern int compareKeyValues (const KeyValue *value1, const KeyValue *value2);

extern int findKeyValue (
  const KeyValue *values, unsigned int count,
  const KeyValue *target, unsigned int *position
);
extern int insertKeyValue (
  KeyValue **values, unsigned int *count, unsigned int *size,
  const KeyValue *value, unsigned int position
);
extern void removeKeyValue (KeyValue *values, unsigned int *count, unsigned int position);
extern int deleteKeyValue (KeyValue *values, unsigned int *count, const KeyValue *value);

extern int compareKeyBindings (const KeyBinding *binding1, const KeyBinding *binding2);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BRLTTY_INCLUDED_KTB_INTERNAL */
