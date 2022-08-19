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

#include "log.h"
#include "pty_shared.h"

key_t
ptyMakeSegmentKey (const char *tty) {
  return ftok(tty, 'p');
}

int
ptyGetSegmentIdentifier (key_t key, int *identifier) {
  int result = shmget(key, 0, 0);
  int ok = result != -1;

  if (ok) {
    *identifier = result;
  } else {
    logSystemError("shmget");
  }

  return ok;
}

void *
ptyAttachSegment (int identifier) {
  void *address = shmat(identifier, NULL, 0);
  if (address != (void *)-1) return address;

  logSystemError("shmat");
  return NULL;
}

void *
ptyGetSegment (const char *tty) {
  key_t key = ptyMakeSegmentKey(tty);
  int identifier;

  if (ptyGetSegmentIdentifier(key, &identifier)) {
    void *address = ptyAttachSegment(identifier);
    if (address) return address;
  }

  return NULL;
}

int
ptyDetachSegment (void *address) {
  if (shmdt(address) != -1) return 1;
  logSystemError("shmdt");
  return 0;
}

PtyCharacter *
ptyGetScreenStart (PtyHeader *header) {
  void *address = header;
  address += header->charactersOffset;
  return address;
}

PtyCharacter *
ptyGetRow (PtyHeader *header, unsigned int row, PtyCharacter **end) {
  void *character = ptyGetScreenStart(header);
  unsigned int width = header->screenWidth * header->characterSize;
  character += row * width;

  if (end) {
    *end = character + width;
  }

  return character;
}

PtyCharacter *
ptyGetCharacter (PtyHeader *header, unsigned int row, unsigned int column, PtyCharacter **end) {
  void *address = ptyGetRow(header, row, end);
  address += column * header->characterSize;
  return address;
}

const PtyCharacter *
ptyGetScreenEnd (PtyHeader *header) {
  return ptyGetRow(header, header->screenHeight, NULL);
}
