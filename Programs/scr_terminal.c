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
#include <errno.h>
#include <sys/msg.h>
#include <sys/shm.h>

#include "log.h"
#include "scr_terminal.h"

int
makeTerminalKey (key_t *key, const char *path) {
  key_t result = ftok(path, 't');
  int gotKey = result != -1;

  if (gotKey) {
    *key = result;
  } else {
    logSystemError("ftok");
  }

  return gotKey;
}

int
getMessageQueue (int *queue, key_t key) {
  int result = msgget(key, 0);
  int foundQueue = result != -1;

  if (foundQueue) {
    *queue = result;
  } else if (errno != ENOENT) {
    logSystemError("msgget");
  }

  return foundQueue;
}

int
getScreenSegment (int *identifier, key_t key) {
  int result = shmget(key, 0, 0);
  int foundSegment = result != -1;

  if (foundSegment) {
    *identifier = result;
  } else if (errno != ENOENT) {
    logSystemError("shmget");
  }

  return foundSegment;
}

ScreenSegmentHeader *
attachScreenSegment (int identifier) {
  void *address = shmat(identifier, NULL, 0);
  if (address != (void *)-1) return address;

  logSystemError("shmat");
  return NULL;
}

int
detachScreenSegment (ScreenSegmentHeader *segment) {
  if (shmdt(segment) != -1) return 1;
  logSystemError("shmdt");
  return 0;
}

ScreenSegmentHeader *
getScreenSegmentForKey (key_t key) {
  int identifier;

  if (getScreenSegment(&identifier, key)) {
    ScreenSegmentHeader *segment = attachScreenSegment(identifier);
    if (segment) return segment;
  }

  return NULL;
}

ScreenSegmentHeader *
getScreenSegmentForPath (const char *path) {
  key_t key;
  if (!makeTerminalKey(&key, path)) return NULL;
  return getScreenSegmentForKey(key);
}

ScreenSegmentCharacter *
getScreenStart (ScreenSegmentHeader *segment) {
  void *address = segment;
  address += segment->charactersOffset;
  return address;
}

ScreenSegmentCharacter *
getScreenRow (ScreenSegmentHeader *segment, unsigned int row, ScreenSegmentCharacter **end) {
  void *character = getScreenStart(segment);
  unsigned int width = segment->screenWidth * segment->characterSize;
  character += row * width;

  if (end) {
    *end = character + width;
  }

  return character;
}

ScreenSegmentCharacter *
getScreenCharacter (ScreenSegmentHeader *segment, unsigned int row, unsigned int column, ScreenSegmentCharacter **end) {
  void *address = getScreenRow(segment, row, end);
  address += column * segment->characterSize;
  return address;
}

const ScreenSegmentCharacter *
getScreenEnd (ScreenSegmentHeader *segment) {
  return getScreenRow(segment, segment->screenHeight, NULL);
}
