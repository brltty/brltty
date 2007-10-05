/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2007 by The BRLTTY Developers.
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

#include "prologue.h"

#include <string.h>
#include <errno.h>
#include <fcntl.h>

#include "misc.h"
#include "system.h"

char *
getProgramPath (void) {
  char *path = NULL;
  size_t size = 0X80;
  char *buffer = NULL;

  while (1) {
    buffer = reallocWrapper(buffer, size<<=1);

    {
      int length = readlink("/proc/self/path/a.out", buffer, size);

      if (length == -1) {
        if (errno != ENOENT) LogError("readlink");
        break;
      }

      if (length < size) {
        buffer[length] = 0;
        path = strdupWrapper(buffer);
        break;
      }
    }
  }

  free(buffer);
  return path;
}

#include "sys_boot_none.h"

#include "sys_exec_unix.h"

#include "sys_mount_none.h"

#ifdef ENABLE_SHARED_OBJECTS
#define SHARED_OBJECT_LOAD_FLAGS (RTLD_NOW | RTLD_GLOBAL)
#include "sys_shlib_dlfcn.h"
#endif /* ENABLE_SHARED_OBJECTS */

#include <sys/kbio.h>
#include <sys/kbd.h>

#if 0
static int
getKeyboard (void) {
  static int keyboard = -1;
  if (keyboard == -1) {
    if ((keyboard = open("/dev/kbd", O_WRONLY)) != -1) {
      LogPrint(LOG_DEBUG, "keyboard opened: fd=%d", keyboard);
    } else {
      LogError("keyboard open");
    }
  }
  return keyboard;
}

int
canBeep (void) {
  return getKeyboard() != -1;
}

int
asynchronousBeep (unsigned short frequency, unsigned short milliseconds) {
  return 0;
}

int
synchronousBeep (unsigned short frequency, unsigned short milliseconds) {
  return 0;
}

int
startBeep (unsigned short frequency) {
  int keyboard = getKeyboard();
  if (keyboard != -1) {
    int command = KBD_CMD_BELL;
    if (ioctl(keyboard, KIOCCMD, &command) != -1) return 1;
    LogError("ioctl KIOCCMD KBD_CMD_BELL");
  }
  return 0;
}

int
stopBeep (void) {
  int keyboard = getKeyboard();
  if (keyboard != -1) {
    int command = KBD_CMD_NOBELL;
    if (ioctl(keyboard, KIOCCMD, &command) != -1) return 1;
    LogError("ioctl KIOCCMD KBD_CMD_NOBELL");
  }
  return 0;
}

void
endBeep (void) {
}
#else /* beep support */
#include "sys_beep_none.h"
#endif /* beep support */

#ifdef ENABLE_PCM_SUPPORT
#define PCM_AUDIO_DEVICE_PATH "/dev/audio"
#include "sys_pcm_audio.h"
#endif /* ENABLE_PCM_SUPPORT */

#ifdef ENABLE_MIDI_SUPPORT
#include "sys_midi_none.h"
#endif /* ENABLE_MIDI_SUPPORT */

#include "sys_ports_none.h"
