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

int
getMidiDevice (int errorLevel, MidiBufferFlusher flushBuffer) {
  LogPrint(errorLevel, "MIDI device not supported.");
  return -1;
}

void
setMidiInstrument (unsigned char channel, unsigned char instrument) {
}

void
beginMidiBlock (int descriptor) {
}

void
endMidiBlock (int descriptor) {
}

void
startMidiNote (unsigned char channel, unsigned char note, unsigned char volume) {
}

void
stopMidiNote (unsigned char channel) {
}

void
insertMidiWait (int duration) {
}
