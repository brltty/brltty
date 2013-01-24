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

#ifndef BRLTTY_INCLUDED_KTB_INTERNAL
#define BRLTTY_INCLUDED_KTB_INTERNAL

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define MAX_MODIFIERS_PER_COMBINATION 10

typedef struct {
  const char *name;
  int bit;
} KeyboardFunction;

extern const KeyboardFunction keyboardFunctionTable[];
extern unsigned char keyboardFunctionCount;

typedef enum {
  KCF_IMMEDIATE_KEY = 0X01
} KeyCombinationFlag;

typedef struct {
  unsigned char flags;
  unsigned char anyKeyCount;
  unsigned char modifierCount;
  unsigned char modifierPositions[MAX_MODIFIERS_PER_COMBINATION];
  KeyValue modifierKeys[MAX_MODIFIERS_PER_COMBINATION];
  KeyValue immediateKey;
} KeyCombination;

typedef enum {
  KBF_HIDDEN   = 0X01,
  KBF_OFFSET   = 0X02,
  KBF_COLUMN   = 0X04,
  KBF_ROW      = 0X08,
  KBF_RANGE    = 0X10,
  KBF_ROUTE    = 0X20,
  KBF_KEYBOARD = 0X40
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
  const KeyboardFunction *keyboardFunction;
} MappedKeyEntry;

typedef struct {
  wchar_t *title;

  struct {
    KeyBinding *table;
    unsigned int size;
    unsigned int count;
    const KeyBinding **sorted;
  } keyBindings;

  struct {
    HotkeyEntry *table;
    unsigned int count;
    const HotkeyEntry **sorted;
  } hotkeys;

  struct {
    MappedKeyEntry *table;
    unsigned int count;
    const MappedKeyEntry **sorted;
    int superimpose;
  } mappedKeys;
} KeyContext;

struct KeyTableStruct {
  wchar_t *title;

  wchar_t **noteTable;
  unsigned int noteCount;

  const KeyNameEntry **keyNameTable;
  unsigned int keyNameCount;

  KeyContext *keyContextTable;
  unsigned int keyContextCount;

  struct {
    unsigned char persistent;
    unsigned char next;
    unsigned char current;
  } context;

  KeyValue *pressedKeys;
  unsigned int pressedSize;
  unsigned int pressedCount;

  int command;
  unsigned immediate:1;

  const unsigned char *logKeyEvents;
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
