/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2006 by The BRLTTY Developers.
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
#include <sys/ioctl.h>
#include <linux/kd.h>

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
      int length = readlink("/proc/self/exe", buffer, size);

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

char *
getBootParameters (const char *name) {
  size_t nameLength = strlen(name);
  const char *path = "/proc/cmdline";
  FILE *file = fopen(path, "r");
  if (file) {
    char *parameters = strdupWrapper("");
    char buffer[0X1000];
    char *line = fgets(buffer, sizeof(buffer), file);
    if (line) {
      char *token;
      while ((token = strtok(line, " \n"))) {
        line = NULL;
        if ((strncmp(token, name, nameLength) == 0) &&
            (token[nameLength] == '=')) {
          free(parameters);
          parameters = strdupWrapper(token + nameLength + 1);
        }
      }
    }
    fclose(file);
    return parameters;
  }
  return NULL;
}

#include "sys_exec_unix.h"

#define SHARED_OBJECT_LOAD_FLAGS (RTLD_NOW | RTLD_GLOBAL)
#include "sys_shlib_dlfcn.h"

#define BEEP_DIVIDEND 1193180

int
canBeep (void) {
  return getConsole() != -1;
}

int
synchronousBeep (unsigned short frequency, unsigned short milliseconds) {
  return 0;
}

int
asynchronousBeep (unsigned short frequency, unsigned short milliseconds) {
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

void
endBeep (void) {
}

#ifdef ENABLE_PCM_SUPPORT
#if defined(USE_PCM_SUPPORT_OSS)
#define PCM_OSS_DEVICE_PATH "/dev/dsp"
#include "sys_pcm_oss.h"
#elif defined(USE_PCM_SUPPORT_ALSA)
#include "sys_pcm_alsa.h"
#else /* USE_PCM_SUPPORT_ */
#warning PCM interface either unspecified or unsupported
#include "sys_pcm_none.h"
#endif /* USE_PCM_SUPPORT_ */
#endif /* ENABLE_PCM_SUPPORT */

#ifdef ENABLE_MIDI_SUPPORT
#if defined(USE_MIDI_SUPPORT_OSS)
#define MIDI_OSS_DEVICE_PATH "/dev/sequencer";
#include "sys_midi_oss.h"
#elif defined(USE_MIDI_SUPPORT_ALSA)
#include "sys_midi_alsa.h"
#else /* USE_MIDI_SUPPORT_ */
#warning MIDI interface either unspecified or unsupported
#include "sys_midi_none.h"
#endif /* USE_MIDI_SUPPORT_ */
#endif /* ENABLE_MIDI_SUPPORT */

#include "sys_ports_glibc.h"
