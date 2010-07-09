/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2010 by The BRLTTY Developers.
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

#include "prologue.h"

#include <string.h>

#include "ses.h"
#include "defaults.h"
#include "misc.h"

static const SessionEntry initialSessionEntry = {
  .number = 0,

  .trackCursor = DEFAULT_TRACK_CURSOR,
  .hideCursor = DEFAULT_HIDE_CURSOR,

  .ptrx = -1,
  .ptry = -1
};

static SessionEntry **sessionArray = NULL;
static unsigned int sessionLimit = 0;
static unsigned int sessionCount = 0;

SessionEntry *
getSessionEntry (int number) {
  int first = 0;
  int last = sessionCount - 1;

  while (first <= last) {
    int current = (first + last) / 2;
    SessionEntry *entry = sessionArray[current];
    if (number == entry->number) return entry;

    if (number < entry->number) {
      last = current - 1;
    } else {
      first = current + 1;
    }
  }

  if (sessionCount == sessionLimit) {
    sessionLimit = sessionLimit? sessionLimit<<1: 0X10;
    sessionArray = reallocWrapper(sessionArray, ARRAY_SIZE(sessionArray, sessionLimit));
  }

  {
    SessionEntry *entry = mallocWrapper(sizeof(*entry));

    *entry = initialSessionEntry;
    entry->number = number;

    memmove(&sessionArray[first+1], &sessionArray[first],
            ARRAY_SIZE(sessionArray, sessionCount-first));
    sessionArray[first] = entry;
    sessionCount += 1;

    return entry;
  }
}

void
deallocateSessionEntries (void) {
  if (sessionArray) {
    while (sessionCount > 0) free(sessionArray[--sessionCount]);
    free(sessionArray);
    sessionArray = NULL;
  } else {
    sessionCount = 0;
  }
  sessionLimit = 0;
}
