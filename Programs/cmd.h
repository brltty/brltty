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

#ifndef BRLTTY_INCLUDED_CMD
#define BRLTTY_INCLUDED_CMD

#include "cmd_types.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

extern const CommandEntry commandTable[];
extern const CommandEntry *getCommandEntry (int code);

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

typedef enum {
  CDO_IncludeName    = 0X1,
  CDO_IncludeOperand = 0X2,
  CDO_DefaultOperand = 0X4
} CommandDescriptionOption;

extern size_t describeCommand (int command, char *buffer, size_t size, CommandDescriptionOption options);
extern void logCommand (int command);
extern void logTransformedCommand (int oldCommand, int newCommand);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BRLTTY_INCLUDED_CMD */
