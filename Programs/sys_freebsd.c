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
#include <errno.h>
#include <dlfcn.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/kbio.h>
#include <sys/soundcard.h>

#include "misc.h"
#include "system.h"

char *
getBootParameters (void) {
  return NULL;
}

#define SHARED_OBJECT_LOAD_FLAGS (RTLD_NOW | RTLD_GLOBAL)
#include "sys_shlib_dlfcn.h"

#define BEEP_DIVIDEND 1193180

int
canBeep (void) {
  if (getConsole() != -1) return 1;
  return 0;
}

int
timedBeep (unsigned short frequency, unsigned short milliseconds) {
  int console = getConsole();
  if (console != -1) {
    if (ioctl(console, KDMKTONE, ((milliseconds << 0X10) | (BEEP_DIVIDEND / frequency))) != -1) return 1;
    LogError("ioctl KDMKTONE");
  }
  return 0;
}

int
startBeep (unsigned short frequency) {
  int console = getConsole();
  if (console != -1) {
    if (ioctl(console, KIOCSOUND, BEEP_DIVIDEND/frequency) != -1) return 1;
    LogError("ioctl KIOCSOUND");
  }
  return 0;
}

int
stopBeep (void) {
  int console = getConsole();
  if (console != -1) {
    if (ioctl(console, KIOCSOUND, 0) != -1) return 1;
    LogError("ioctl KIOCSOUND");
  }
  return 0;
}

#ifdef ENABLE_PCM_TUNES
#include "sys_pcm_dsp.h"
#endif /* ENABLE_PCM_TUNES */

#ifdef ENABLE_MIDI_TUNES
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
#endif /* ENABLE_MIDI_TUNES */

int
enablePorts (int errorLevel, unsigned short int base, unsigned short int count) {
  LogPrint(errorLevel, "I/O ports not supported.");
  return 0;
}

int
disablePorts (unsigned short int base, unsigned short int count) {
  return 0;
}

unsigned char
readPort1 (unsigned short int port) {
  return 0;
}

void
writePort1 (unsigned short int port, unsigned char value) {
}
