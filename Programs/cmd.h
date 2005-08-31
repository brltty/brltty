/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2005 by The BRLTTY Team. All rights reserved.
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

#ifndef BRLTTY_INCLUDED_CMD
#define BRLTTY_INCLUDED_CMD

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef struct {
  int code;
  const char *name;
  const char *description;
} CommandEntry;

extern const CommandEntry commandTable[];

extern void describeCommand (int command, char *buffer, int size);

typedef struct {
  int command;
  int timer;
  int started;
} RepeatState;
extern void resetRepeatState (RepeatState *state);
extern int handleRepeatFlags (
  int command,
  RepeatState *state,
  int updateInterval,
  int repeatDelay,
  int repeatInterval
);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BRLTTY_INCLUDED_CMD */
