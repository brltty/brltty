/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2007 by The BRLTTY Developers.
 *
 * BRLTTY comes with ABSOLUTELY NO WARRANTY.
 *
 * This is free software, placed under the terms of the
 * GNU General Public License, as published by the Free Software
 * Foundation.  Please see the file COPYING for details.
 *
 * Web Page: http://mielke.cc/brltty/
 *
 * This software is maintained by Dave Mielke <dave@mielke.cc>.
 */

#ifndef BRLTTY_INCLUDED_KEYS
#define BRLTTY_INCLUDED_KEYS

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef enum {
  KBD_TYPE_Any = 0,
  KBD_TYPE_USB,
  KBD_TYPE_Bluetooth
} KeyboardType;

typedef struct {
  const char *device;
  KeyboardType type;
  int vendor;
  int product;
} KeyboardProperties;
#define KEYBOARD_PROPERTIES_INITIALIZER {.type=KBD_TYPE_Any}

extern int checkKeyboardProperties (const KeyboardProperties *required, const KeyboardProperties *actual);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BRLTTY_INCLUDED_KEYS */
