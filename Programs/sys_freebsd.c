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
#include <machine/speaker.h>
#include <sys/soundcard.h>

#include "misc.h"
#include "system.h"

#include "sys_boot_none.h"

#define SHARED_OBJECT_LOAD_FLAGS (RTLD_NOW | RTLD_GLOBAL)
#include "sys_shlib_dlfcn.h"

static int
getSpeaker (void) {
  static int speaker = -1;
  if (speaker == -1) {
    if ((speaker = open("/dev/speaker", O_WRONLY)) == -1) LogError("speaker open");
  }
  return speaker;
}

int
canBeep (void) {
  if (getSpeaker()) return 1;
  return 0;
}

int
timedBeep (unsigned short frequency, unsigned short milliseconds) {
  int speaker = getSpeaker();
  if (speaker != -1) {
    tone_t tone;
    memset(&tone, 0, sizeof(tone));
    tone.frequency = frequency;
    tone.duration = (milliseconds + 9) / 10;
    if (ioctl(speaker, SPKRTONE, &tone) != -1) return 1;
    LogError("speaker tone");
  }
  return 0;
}

int
startBeep (unsigned short frequency) {
  return 0;
}

int
stopBeep (void) {
  return 0;
}

#ifdef ENABLE_PCM_TUNES
#include "sys_pcm_dsp.h"
#endif /* ENABLE_PCM_TUNES */

#ifdef ENABLE_MIDI_TUNES
#include "sys_midi_none.h"
#endif /* ENABLE_MIDI_TUNES */

#include "sys_ports_none.h"
