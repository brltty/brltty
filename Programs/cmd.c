/*
 * BRLTTY - A background process providing access to the Linux console (when in
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
  int blk = command & BRL_MSK_BLK;
  int arg = command & BRL_MSK_ARG;
  int cmd = blk | arg;
  const CommandEntry *candidate = NULL;
  const CommandEntry *last = NULL;

  {
    const CommandEntry *current = commandTable;
    while (current->name) {
      if ((current->code & BRL_MSK_BLK) == blk) {
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

      case BRL_BLK_PASSCHAR:
        number = cmd - candidate->code;
        break;

      case BRL_BLK_PASSDOTS: {
        unsigned char dots[] = {BRL_DOT1, BRL_DOT2, BRL_DOT3, BRL_DOT4, BRL_DOT5, BRL_DOT6, BRL_DOT7, BRL_DOT8};
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

    if ((blk == 0) && (command & BRL_FLG_TOGGLE_MASK)) {
      char *description = buffer + offset;
      const char *oldVerb = "toggle";
      int oldLength = strlen(oldVerb);
      if (strncmp(description, oldVerb, oldLength) == 0) {
        const char *newVerb = "set";
        int newLength = strlen(newVerb);
        memmove(description+newLength, description+oldLength, strlen(description+oldLength)+1);
        memcpy(description, newVerb, newLength);
        if (command & BRL_FLG_TOGGLE_ON) {
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
