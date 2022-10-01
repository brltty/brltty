/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2022 by The BRLTTY Developers.
 *
 * BRLTTY comes with ABSOLUTELY NO WARRANTY.
 *
 * This is free software, placed under the terms of the
 * GNU Lesser General Public License, as published by the Free Software
 * Foundation; either version 2.1 of the License, or (at your option) any
 * later version. Please see the file LICENSE-LGPL for details.
 *
 * Web Page: http://brltty.app/
 *
 * This software is maintained by Dave Mielke <dave@mielke.cc>.
 */

#include "prologue.h"

#include <string.h>
#include <sys/stat.h>
#include <sys/msg.h>
#include <sys/shm.h>

#include "log.h"
#include "scr_emulator.h"

static const int ipcCreationFlags = IPC_CREAT | IPC_EXCL | S_IRUSR | S_IWUSR;

int
destroyMessageQueue (int queue) {
  if (msgctl(queue, IPC_RMID, NULL) != -1) return 1;
  logSystemError("msgctl[IPC_RMID]");
  return 0;
}

int
createMessageQueue (int *queue, key_t key) {
  int q;

  if (getMessageQueue(&q, key)) {
    destroyMessageQueue(q);
  }

  if ((q = msgget(key, ipcCreationFlags)) != -1) {
    if (queue) *queue = q;
    return 1;
  } else {
    logSystemError("msgget");
  }

  return 0;
}

int
destroyScreenSegment (int identifier) {
  if (shmctl(identifier, IPC_RMID, NULL) != -1) return 1;
  logSystemError("shmctl[IPC_RMID]");
  return 0;
}

void
moveScreenCharacters (ScreenSegmentCharacter *to, const ScreenSegmentCharacter *from, size_t count) {
  if (count && (from != to)) {
    memmove(to, from, (count * sizeof(*from)));
  }
}

void
setScreenCharacters (ScreenSegmentCharacter *from, const ScreenSegmentCharacter *to, const ScreenSegmentCharacter *character) {
  while (from < to) *from++ = *character;
}

void
propagateScreenCharacter (ScreenSegmentCharacter *from, const ScreenSegmentCharacter *to) {
  setScreenCharacters(from+1, to, from);
}

static void
initializeScreenCharacters (ScreenSegmentCharacter *from, const ScreenSegmentCharacter *to) {
  const ScreenSegmentCharacter initializer = {
    .text = ' ',
    .foreground = SCREEN_SEGMENT_COLOR_WHITE,
    .background = SCREEN_SEGMENT_COLOR_BLACK,
    .alpha = UINT8_MAX,
  };

  setScreenCharacters(from, to, &initializer);
}

ScreenSegmentHeader *
createScreenSegment (int *id, key_t key, int columns, int rows) {
  size_t size = sizeof(ScreenSegmentHeader) + (sizeof(ScreenSegmentCharacter) * columns * rows);
  int identifier;

  if (getScreenSegment(&identifier, key)) {
    destroyScreenSegment(identifier);
  }

  if ((identifier = shmget(key, size, ipcCreationFlags)) != -1) {
    ScreenSegmentHeader *segment = attachScreenSegment(identifier);

    if (segment) {
      segment->headerSize = sizeof(*segment);
      segment->segmentSize = size;

      segment->characterSize = sizeof(ScreenSegmentCharacter);
      segment->charactersOffset = segment->headerSize;

      segment->screenHeight = rows;
      segment->screenWidth = columns;

      segment->cursorRow = 0;
      segment->cursorColumn = 0;

      segment->screenNumber = 0;
      segment->commonFlags = 0;
      segment->privateFlags = 0;

      {
        ScreenSegmentCharacter *from = getScreenStart(segment);
        const ScreenSegmentCharacter *to = getScreenEnd(segment);
        initializeScreenCharacters(from, to);
      }

      if (id) *id = identifier;
      return segment;
    }

    destroyScreenSegment(identifier);
  } else {
    logSystemError("shmget");
  }

  return NULL;
}
