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

#ifndef BRLTTY_INCLUDED_CMD
#define BRLTTY_INCLUDED_CMD

#include "timing.h"
#ifdef ENABLE_API
#include "brlapi_keycodes.h"
#endif /* ENABLE_API */

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

extern const CommandEntry commandTable[];
extern const CommandEntry *getCommandEntry (int code);

typedef enum {
  CDO_IncludeName    = 0X1,
  CDO_IncludeOperand = 0X2,
  CDO_DefaultOperand = 0X4
} CommandDescriptionOption;

extern size_t describeCommand (int command, char *buffer, size_t size, CommandDescriptionOption options);
extern void logCommand (int command);
extern void logTransformedCommand (int oldCommand, int newCommand);

typedef struct {
  const char *name;
  int bit;
} CommandModifierEntry;

extern const CommandModifierEntry commandModifierTable_braille[];
extern const CommandModifierEntry commandModifierTable_character[];
extern const CommandModifierEntry commandModifierTable_input[];
extern const CommandModifierEntry commandModifierTable_keyboard[];
extern const CommandModifierEntry commandModifierTable_line[];
extern const CommandModifierEntry commandModifierTable_motion[];
extern const CommandModifierEntry commandModifierTable_toggle[];

typedef struct {
  int command;
  int timeout;
  int started;
  TimeValue time;
} RepeatState;
extern void resetRepeatState (RepeatState *state);
extern void handleRepeatFlags (int *command, RepeatState *state, int panning, int delay, int interval);

#ifdef ENABLE_API
extern brlapi_keyCode_t cmdBrlttyToBrlapi (int command, int retainDots);
extern int cmdBrlapiToBrltty (brlapi_keyCode_t code);
#endif /* ENABLE_API */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BRLTTY_INCLUDED_CMD */
