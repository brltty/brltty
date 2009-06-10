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

typedef struct {
  KeySetMask modifiers;
  unsigned char set;
  unsigned char key;
} KeyCombination;

typedef struct {
  KeyCombination keys;
  int command;
} KeyBinding;

typedef uint32_t KeyTableOffset;

typedef struct {
  KeyTableOffset bindingsTable;
  uint32_t bindingsCount;
} KeyTableHeader;

struct KeyTableStruct {
  union {
    KeyTableHeader *fields;
    const unsigned char *bytes;
  } header;

  size_t size;

  KeySet keys;
  int command;
};

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BRLTTY_INCLUDED_KTB_INTERNAL */
