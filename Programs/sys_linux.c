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
#include "sys_linux.h"

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

static void
enableBeeps (void) {
  static int status = 0;
  installKernelModule("pcspkr", &status);
}

int
canBeep (void) {
  enableBeeps();
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

int
installKernelModule (const char *name, int *status) {
  if (!status || !*status) {
    const char *command = "modprobe";
    char buffer[0X100];
    if (status) ++*status;

    {
      const char *path = "/proc/sys/kernel/modprobe";
      FILE *stream = fopen(path, "r");

      if (stream) {
        char *line = fgets(buffer, sizeof(buffer), stream);

        if (line) {
          size_t length = strlen(line);
          if (length && (line[length-1] == '\n')) line[--length] = 0;
          if (length) command = line;
        }

        fclose(stream);
      } else {
        LogPrint(LOG_WARNING, "cannot open %s: %s", path, strerror(errno));
      }
    }

    {
      const char *const arguments[] = {command, "-q", name, NULL};
      int ok = executeHostCommand(arguments) == 0;

      if (!ok) {
        LogPrint(LOG_WARNING, "kernel module not installed: %s", name);
        return 0;
      }

      if (status) ++*status;
    }
  }

  return 1;
}

int
openCharacterDevice (const char *path, int flags, int major, int minor) {
  int descriptor;

  LogPrint(LOG_DEBUG, "opening device: %s", path);
  if ((descriptor = open(path, flags)) == -1) {
    int create = errno == ENOENT;

    LogPrint(create? LOG_WARNING: LOG_ERR, 
             "cannot open device: %s: %s",
             path, strerror(errno));

    if (create) {
      mode_t mode = S_IFCHR | S_IRUSR | S_IWUSR;

      LogPrint(LOG_NOTICE, "creating device: %s mode=%06o major=%d minor=%d",
               path, mode, major, minor);
      if (mknod(path, mode, makedev(major, minor)) == -1) {
        LogPrint(LOG_ERR, "cannot create device: %s: %s",
                 path, strerror(errno));
      } else if ((descriptor = open(path, flags)) == -1) {
        LogPrint(LOG_ERR, "cannot open device: %s: %s",
                 path, strerror(errno));

        if (unlink(path) == -1) {
          LogPrint(LOG_ERR, "cannot remove device: %s: %s",
                   path, strerror(errno));
        } else {
          LogPrint(LOG_NOTICE, "device removed: %s", path);
        }
      }
    }
  }

  return descriptor;
}

#ifdef HAVE_LINUX_INPUT_H
#include <linux/input.h>
#endif /* HAVE_LINUX_INPUT_H */

#ifdef HAVE_LINUX_UINPUT_H
#include <linux/uinput.h>
#endif /* HAVE_LINUX_UINPUT_H */

int
getUinputDevice (void) {
  static int uinput = -1;

#ifdef HAVE_LINUX_UINPUT_H
  if (uinput == -1) {
    int device;
    LogPrint(LOG_DEBUG, "opening uinput");

    {
      static int status = 0;
      installKernelModule("uinput", &status);
    }

    if ((device = openCharacterDevice("/dev/uinput", O_WRONLY, 10, 223)) != -1) {
      struct uinput_user_dev description;
      LogPrint(LOG_DEBUG, "uinput opened: fd=%d", device);
      
      memset(&description, 0, sizeof(description));
      strcpy(description.name, "brltty");

      if (write(device, &description, sizeof(description)) != -1) {
        ioctl(device, UI_SET_EVBIT, EV_KEY);
        ioctl(device, UI_SET_EVBIT, EV_REP);

        {
          int key;
          for (key=KEY_RESERVED; key<=KEY_MAX; key++) {
            ioctl(device, UI_SET_KEYBIT, key);
          }
        }

        if (ioctl(device, UI_DEV_CREATE) != -1) {
          uinput = device;
        } else {
          LogError("ioctl[UI_DEV_CREATE]");
        }
      } else {
        LogError("write(struct uinput_user_dev)");
      }

      if (device != uinput) {
        close(device);
        LogPrint(LOG_DEBUG, "uinput closed");
      }
    }
  }
#endif /* HAVE_LINUX_UINPUT_H */

  return uinput;
}

static BITMASK(pressedKeys, KEY_MAX+1);

int
writeKeyEvent (int key, int press) {
  int device = getUinputDevice();

  if (device != -1) {
#ifdef HAVE_LINUX_INPUT_H
    struct input_event event;

    event.type = EV_KEY;
    event.code = key;
    event.value = press;

    if (write(device, &event, sizeof(event)) != -1) {
      if (press) {
        BITMASK_SET(pressedKeys, key);
      } else {
        BITMASK_CLEAR(pressedKeys, key);
      }
      return 1;
    }

    LogError("write(struct input_event)");
#endif /* HAVE_LINUX_INPUT_H */
  }

  return 0;
}

void
releaseAllKeys (void) {
  int key;
  for (key=0; key<=KEY_MAX; ++key) {
    if (BITMASK_TEST(pressedKeys, key)) {
      if (!writeKeyEvent(key, 0)) break;
    }
  }
}
