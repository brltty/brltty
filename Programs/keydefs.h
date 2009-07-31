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

#ifndef BRLTTY_INCLUDED_KEYDEFS
#define BRLTTY_INCLUDED_KEYDEFS

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define KEYS_PER_SET 0X100

typedef struct {
  const char *name;
  unsigned char set;
  unsigned char key;
} KeyNameEntry;

#define KEY_NAME_TABLE(name) keyNameTable_##name
#define KEY_NAME_TABLE_DECLARATION(name) const KeyNameEntry KEY_NAME_TABLE(name)[]
#define KEY_NAME_ENTRY(keyNumber,keyName) {.key=keyNumber, .name=keyName}
#define KEY_SET_ENTRY(setNumber,keyName) {.set=setNumber, .name=keyName}
#define LAST_KEY_NAME_ENTRY {.name=NULL}
#define BEGIN_KEY_NAME_TABLE(name) static KEY_NAME_TABLE_DECLARATION(name) = {
#define END_KEY_NAME_TABLE LAST_KEY_NAME_ENTRY};

#define KEY_NAME_TABLES(name) keyNameTables_##name
#define KEY_NAME_TABLES_REFERENCE const KeyNameEntry *const *
#define KEY_NAME_TABLES_DECLARATION(name) const KeyNameEntry *const KEY_NAME_TABLES(name)[]
#define LAST_KEY_NAME_TABLE NULL
#define BEGIN_KEY_NAME_TABLES(name) static KEY_NAME_TABLES_DECLARATION(name) = {
#define END_KEY_NAME_TABLES LAST_KEY_NAME_TABLE};

typedef struct KeyTableStruct KeyTable;

typedef enum {
  KTS_UNBOUND,
  KTS_MODIFIERS,
  KTS_COMMAND
} KeyTableState;

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BRLTTY_INCLUDED_KEYDEFS */
