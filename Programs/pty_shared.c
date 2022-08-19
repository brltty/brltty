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
ptyMakeSharedSegmentKey (const char *tty) {
  return ftok(tty, 'p');
}

int
ptyGetSharedSegmentIdentifier (key_t key, int *identifier) {
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
ptyAttachSharedSegment (int identifier) {
  void *address = shmat(identifier, NULL, 0);
  if (address != (void *)-1) return address;

  logSystemError("shmat");
  return NULL;
}

void *
ptyGetSharedSegment (const char *tty) {
  key_t key = ptyMakeSharedSegmentKey(tty);
  int identifier;

  if (ptyGetSharedSegmentIdentifier(key, &identifier)) {
    void *address = ptyAttachSharedSegment(identifier);
    if (address) return address;
  }

  return NULL;
}

int
ptyDetachSharedSegment (void *address) {
  if (shmdt(address) != -1) return 1;
  logSystemError("shmdt");
  return 0;
}

PtySharedSegmentCharacter *
ptyGetSharedSegmentScreenStart (PtySharedSegmentHeader *header) {
  void *address = header;
  address += header->charactersOffset;
  return address;
}

PtySharedSegmentCharacter *
ptyGetSharedSegmentRow (PtySharedSegmentHeader *header, unsigned int row, PtySharedSegmentCharacter **end) {
  void *character = ptyGetSharedSegmentScreenStart(header);
  unsigned int width = header->screenWidth * header->characterSize;
  character += row * width;

  if (end) {
    *end = character + width;
  }

  return character;
}

PtySharedSegmentCharacter *
ptyGetSharedSegmentCharacter (PtySharedSegmentHeader *header, unsigned int row, unsigned int column, PtySharedSegmentCharacter **end) {
  void *address = ptyGetSharedSegmentRow(header, row, end);
  address += column * header->characterSize;
  return address;
}

const PtySharedSegmentCharacter *
ptyGetSharedSegmentScreenEnd (PtySharedSegmentHeader *header) {
  return ptyGetSharedSegmentRow(header, header->screenHeight, NULL);
}
