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
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/audioio.h>
#include <sys/time.h>
#include <dev/wscons/wsconsio.h>

#ifdef HAVE_FUNC_DLOPEN 
#  include <dlfcn.h>
#endif /* HAVE_FUNC_DLOPEN */

#include "misc.h"
#include "system.h"

#include "sys_boot_none.h"

#define SHARED_OBJECT_LOAD_FLAGS (DL_LAZY)
#include "sys_shlib_dlfcn.h"

int
canBeep (void) {
  if (getConsole() != -1) return 1;
  return 0;
}

int
timedBeep (unsigned short frequency, unsigned short milliseconds) {
  int console = getConsole();
  if (console != -1) {
    struct wskbd_bell_data bell;
    bell.which = WSKBD_BELL_DOALL;
    bell.pitch = frequency;
    bell.period = milliseconds;
    bell.volume = 100;
    if (!bell.period) {
      return 1;
    } else if (ioctl(console, WSKBDIO_COMPLEXBELL, &bell) != -1) {
      return 1;
    } else {
      LogPrint(LOG_WARNING, "ioctl WSKBDIO_COMPLEXBELL failed: %s", strerror(errno));
    }
  }
  return 0;
}

int
startBeep (unsigned short frequency) {
  return 0;
}

int
stopBeep (void) {
  int console = getConsole();
  if (console != -1) {
    struct wskbd_bell_data bell;
    bell.which = WSKBD_BELL_DOVOLUME | WSKBD_BELL_DOPERIOD;
    bell.volume = 0;
    bell.period = 0;
    if (ioctl(console, WSKBDIO_COMPLEXBELL, &bell) != -1) {
      return 1;
    } else {
      LogPrint(LOG_WARNING, "ioctl WSKBDIO_COMPLEXBELL failed: %s", strerror(errno));
    }
  }
  return 0;
}

#ifdef ENABLE_PCM_TUNES
#include "sys_pcm_audio.h"
#endif /* ENABLE_PCM_TUNES */

#ifdef ENABLE_MIDI_TUNES
#include "sys_midi_none.h"
#endif /* ENABLE_MIDI_TUNES */

#include "sys_ports_none.h"
