/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2015 by The BRLTTY Developers.
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

#ifndef BRLTTY_INCLUDED_CMD_TYPES
#define BRLTTY_INCLUDED_CMD_TYPES

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef struct {
  const char *name;
  const char *description;
  int code;
  unsigned isToggle:1;
  unsigned isMotion:1;
  unsigned isRouting:1;
  unsigned isColumn:1;
  unsigned isRow:1;
  unsigned isOffset:1;
  unsigned isRange:1;
  unsigned isInput:1;
  unsigned isCharacter:1;
  unsigned isBraille:1;
  unsigned isKeyboard:1;
} CommandEntry;

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BRLTTY_INCLUDED_CMD_TYPES */
