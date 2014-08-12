/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2014 by The BRLTTY Developers.
 *
 * BRLTTY comes with ABSOLUTELY NO WARRANTY.
 *
 * This is free software, placed under the terms of the
 * GNU General Public License, as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any
 * later version. Please see the file LICENSE-GPL for details.
 *
 * Web Page: http://mielke.cc/brltty/
 *
 * This software is maintained by Dave Mielke <dave@mielke.cc>.
 */

#include "prologue.h"

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/stat.h>

#ifdef HAVE_LINUX_INPUT_H
#include <linux/input.h>
#endif /* HAVE_LINUX_INPUT_H */

#include "log.h"
#include "file.h"
#include "device.h"
#include "async_wait.h"
#include "hostcmd.h"
#include "bitmask.h"
#include "system.h"
#include "system_linux.h"

#ifdef HAVE_LINUX_UINPUT_H
#include <linux/uinput.h>

struct UinputObjectStruct {
  int fileDescriptor;
  BITMASK(pressedKeys, KEY_MAX+1, char);
};

static void
releasePressedKeys (UinputObject *uinput) {
  unsigned int key;

  for (key=0; key<=KEY_MAX; key+=1) {
    if (BITMASK_TEST(uinput->pressedKeys, key)) {
      if (!writeKeyEvent(uinput, key, 0)) break;
    }
  }
}
#endif /* HAVE_LINUX_UINPUT_H */

int
installKernelModule (const char *name, int *status) {
  if (status && *status) return *status == 2;

  {
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
        logMessage(LOG_WARNING, "cannot open %s: %s", path, strerror(errno));
      }
    }

    {
      const char *const arguments[] = {command, "-q", name, NULL};
      int ok = executeHostCommand(arguments) == 0;

      if (!ok) {
        logMessage(LOG_WARNING, "kernel module not installed: %s", name);
        return 0;
      }

      if (status) ++*status;
    }
  }

  return 1;
}

static int
openDevice (const char *path, mode_t flags, int allowModeSubset) {
  int descriptor;

  if ((descriptor = open(path, flags)) != -1) goto opened;
  if (!allowModeSubset) goto failed;
  if ((flags & O_ACCMODE) != O_RDWR) goto failed;
  flags &= ~O_ACCMODE;

  {
    int error = errno;

    if (errno == EACCES) goto tryWriteOnly;
    if (errno == EROFS) goto tryReadOnly;
    goto failed;

  tryWriteOnly:
    if ((descriptor = open(path, (flags | O_WRONLY))) != -1) goto opened;

  tryReadOnly:
    if ((descriptor = open(path, (flags | O_RDONLY))) != -1) goto opened;

    errno = error;
  }

failed:
  logMessage(LOG_DEBUG, "cannot open device: %s: %s", path, strerror(errno));
  return -1;

opened:
  logMessage(LOG_DEBUG, "device opened: %s: fd=%d", path, descriptor);
  return descriptor;
}

int
openCharacterDevice (const char *name, int flags, int major, int minor) {
  char *path = getDevicePath(name);
  int descriptor;

  if (!path) {
    descriptor = -1;
  } else if ((descriptor = openDevice(path, flags, 1)) == -1) {
    if (errno == ENOENT) {
      free(path);

      if ((path = makeWritablePath(locatePathName(name)))) {
        if ((descriptor = openDevice(path, flags, 0)) == -1) {
          if (errno == ENOENT) {
            mode_t mode = S_IFCHR | S_IRUSR | S_IWUSR;

            if (mknod(path, mode, makedev(major, minor)) == -1) {
              logMessage(LOG_DEBUG, "cannot create device: %s: %s", path, strerror(errno));
            } else {
              logMessage(LOG_DEBUG, "device created: %s mode=%06o major=%d minor=%d",
                         path, mode, major, minor);

              descriptor = openDevice(path, flags, 0);
            }
          }
        }
      }
    }
  }

  if (descriptor != -1) {
    int ok = 0;
    struct stat status;

    if (fstat(descriptor, &status) == -1) {
      logMessage(LOG_DEBUG, "cannot fstat device: %d [%s]: %s",
                 descriptor, path, strerror(errno));
    } else if (!S_ISCHR(status.st_mode)) {
      logMessage(LOG_DEBUG, "not a character device: %s: fd=%d", path, descriptor);
    } else {
      ok = 1;
    }

    if (!ok) {
      close(descriptor);
      logMessage(LOG_DEBUG, "device closed: %s: fd=%d", path, descriptor);
      descriptor = -1;
    }
  }

  if (path) free(path);
  return descriptor;
}

UinputObject *
newUinputObject (void) {
  UinputObject *uinput = NULL;

#ifdef HAVE_LINUX_UINPUT_H
  if ((uinput = malloc(sizeof(*uinput)))) {
    const char *device;

    memset(uinput, 0, sizeof(*uinput));

    {
      static int status = 0;
      int wait = !status;

      if (!installKernelModule("uinput", &status)) wait = 0;
      if (wait) asyncWait(500);
    }

    {
      static const char *const names[] = {"uinput", "input/uinput", NULL};

      device = resolveDeviceName(names, "uinput");
    }

    if (device) {
      if ((uinput->fileDescriptor = openCharacterDevice(device, O_WRONLY, 10, 223)) != -1) {
        struct uinput_user_dev description;
        
        memset(&description, 0, sizeof(description));
        strncpy(description.name, PACKAGE_TARNAME, sizeof(description.name));

        if (write(uinput->fileDescriptor, &description, sizeof(description)) != -1) {
          logMessage(LOG_DEBUG, "uinput opened: %s fd=%d",
                     device, uinput->fileDescriptor);

          return uinput;
        } else {
          logSystemError("write(struct uinput_user_dev)");
        }

        close(uinput->fileDescriptor);
      } else {
        logMessage(LOG_DEBUG, "cannot open uinput device: %s: %s", device, strerror(errno));
      }
    }

    free(uinput);
    uinput = NULL;
  } else {
    logMallocError();
  }
#endif /* HAVE_LINUX_UINPUT_H */

  return uinput;
}

void
destroyUinputObject (UinputObject *uinput) {
#ifdef HAVE_LINUX_UINPUT_H
  releasePressedKeys(uinput);
  close(uinput->fileDescriptor);
  free(uinput);
#endif /* HAVE_LINUX_UINPUT_H */
}

int
createUinputDevice (UinputObject *uinput) {
#ifdef HAVE_LINUX_UINPUT_H
  if (ioctl(uinput->fileDescriptor, UI_DEV_CREATE) != -1) return 1;
  logSystemError("ioctl[UI_DEV_CREATE]");
#endif /* HAVE_LINUX_UINPUT_H */

  return 0;
}

int
enableUinputEventType (UinputObject *uinput, int type) {
#ifdef HAVE_LINUX_UINPUT_H
  if (ioctl(uinput->fileDescriptor, UI_SET_EVBIT, type) != -1) return 1;
  logSystemError("ioctl[UI_SET_EVBIT]");
#endif /* HAVE_LINUX_UINPUT_H */

  return 0;
}

int
writeInputEvent (UinputObject *uinput, uint16_t type, uint16_t code, int32_t value) {
#ifdef HAVE_LINUX_UINPUT_H
  struct input_event event;

  memset(&event, 0, sizeof(event));
  gettimeofday(&event.time, NULL);
  event.type = type;
  event.code = code;
  event.value = value;

  if (write(uinput->fileDescriptor, &event, sizeof(event)) != -1) return 1;
  logSystemError("write(struct input_event)");
  logMessage(LOG_WARNING, "input event: type=%d code=%d value=%d",
             event.type, event.code, event.value);
#endif /* HAVE_LINUX_UINPUT_H */

  return 0;
}

static int
writeSynReport (UinputObject *uinput) {
  return writeInputEvent(uinput, EV_SYN, SYN_REPORT, 0);
}

int
enableUinputKey (UinputObject *uinput, int key) {
#ifdef HAVE_LINUX_UINPUT_H
  if (ioctl(uinput->fileDescriptor, UI_SET_KEYBIT, key) != -1) return 1;
  logSystemError("ioctl[UI_SET_KEYBIT]");
#endif /* HAVE_LINUX_UINPUT_H */

  return 0;
}

int
writeKeyEvent (UinputObject *uinput, int key, int press) {
#ifdef HAVE_LINUX_UINPUT_H
  if (writeInputEvent(uinput, EV_KEY, key, press)) {
    if (press) {
      BITMASK_SET(uinput->pressedKeys, key);
    } else {
      BITMASK_CLEAR(uinput->pressedKeys, key);
    }

    if (writeSynReport(uinput)) {
      return 1;
    }
  }
#endif /* HAVE_LINUX_UINPUT_H */

  return 0;
}

int
writeRepeatDelay (UinputObject *uinput, int delay) {
  if (writeInputEvent(uinput, EV_REP, REP_DELAY, delay)) {
    if (writeSynReport(uinput)) {
      return 1;
    }
  }

  return 0;
}

int
writeRepeatPeriod (UinputObject *uinput, int period) {
  if (writeInputEvent(uinput, EV_REP, REP_PERIOD, period)) {
    if (writeSynReport(uinput)) {
      return 1;
    }
  }

  return 0;
}

#ifdef HAVE_LINUX_INPUT_H
static int
enableAllKeys (UinputObject *uinput) {
  unsigned int key;

  if (!enableUinputEventType(uinput, EV_KEY)) return 0;

  for (key=0; key<=KEY_MAX; key+=1) {
    if (!enableUinputKey(uinput, key)) return 0;
  }

  return 1;
}
#endif /* HAVE_LINUX_INPUT_H */

UinputObject *
newUinputKeyboard (void) {
#ifdef HAVE_LINUX_INPUT_H
  UinputObject *uinput;

  if ((uinput = newUinputObject())) {
    if (enableAllKeys(uinput)) {
      if (enableUinputEventType(uinput, EV_REP)) {
        if (createUinputDevice(uinput)) {
          return uinput;
        }
      }
    }

    destroyUinputObject(uinput);
  }
#endif /* HAVE_LINUX_INPUT_H */

  return NULL;
}

void
initializeSystemObject (void) {
}
