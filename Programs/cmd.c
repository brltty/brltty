/*
 * BRLTTY - A background process providing access to the Linux console (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2004 by The BRLTTY Team. All rights reserved.
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

#include "brldefs.h"
#include "cmd.h"

const CommandEntry commandTable[] = {
#ifdef ENABLE_LEARN_MODE
  #include "cmds.auto.h"
#endif /* ENABLE_LEARN_MODE */
  {EOF, NULL, NULL}
};

void
describeCommand (int command, char *buffer, int size) {
  int blk = command & VAL_BLK_MASK;
  int arg = command & VAL_ARG_MASK;
  int cmd = blk | arg;
  const CommandEntry *candidate = NULL;
  const CommandEntry *last = NULL;

  {
    const CommandEntry *current = commandTable;
    while (current->name) {
      if ((current->code & VAL_BLK_MASK) == blk) {
        if (!last || (last->code < current->code)) last = current;
        if (!candidate || (candidate->code < cmd)) candidate = current;
      }

      ++current;
    }
  }
  if (candidate)
    if (candidate->code != cmd)
      if ((blk == 0) || (candidate->code < last->code))
        candidate = NULL;

  if (!candidate) {
    snprintf(buffer, size, "unknown: %06X", command);
  } else if ((candidate == last) && (blk != 0)) {
    unsigned int number;
    switch (blk) {
      default:
        number = cmd - candidate->code + 1;
        break;

      case VAL_PASSCHAR:
        number = cmd - candidate->code;
        break;

      case VAL_PASSDOTS: {
        unsigned char dots[] = {B1, B2, B3, B4, B5, B6, B7, B8};
        int dot;
        number = 0;
        for (dot=0; dot<sizeof(dots); ++dot) {
          if (arg & dots[dot]) {
            number = (number * 10) + (dot + 1);
          }
        }
        break;
      }
    }
    snprintf(buffer, size, "%s[%d]: %s",
             candidate->name, number, candidate->description);
  } else {
    int offset;
    snprintf(buffer, size, "%s: %n%s",
             candidate->name, &offset, candidate->description);

    if ((blk == 0) && (command & VAL_TOGGLE_MASK)) {
      char *description = buffer + offset;
      const char *oldVerb = "toggle";
      int oldLength = strlen(oldVerb);
      if (strncmp(description, oldVerb, oldLength) == 0) {
        const char *newVerb = "set";
        int newLength = strlen(newVerb);
        memmove(description+newLength, description+oldLength, strlen(description+oldLength)+1);
        memcpy(description, newVerb, newLength);
        if (command & VAL_TOGGLE_ON) {
          char *end = strrchr(description, '/');
          if (end) *end = 0;
        } else {
          char *target = strrchr(description, ' ');
          if (target) {
            const char *source = strchr(++target, '/');
            if (source) {
              memmove(target, source+1, strlen(source));
            }
          }
        }
      }
    }
  }
}
