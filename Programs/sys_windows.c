/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2010 by The BRLTTY Developers.
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

#include <fcntl.h>
#include <errno.h>

#include "misc.h"
#include "system.h"
#include "sys_windows.h"

#include "sys_prog_windows.h"

#include "sys_boot_none.h"

#include "sys_mount_none.h"

#include "sys_beep_windows.h"

#include "sys_ports_windows.h"

#include "sys_kbd_none.h"

#ifdef __CYGWIN32__

#include "sys_exec_unix.h"

#ifdef ENABLE_SHARED_OBJECTS
#define SHARED_OBJECT_LOAD_FLAGS (RTLD_NOW | RTLD_GLOBAL)
#include "sys_shlib_dlfcn.h"
#endif /* ENABLE_SHARED_OBJECTS */

#ifdef ENABLE_PCM_SUPPORT
#define PCM_OSS_DEVICE_PATH "/dev/dsp"
#include "sys_pcm_oss.h"
#endif /* ENABLE_PCM_SUPPORT */

#ifdef ENABLE_MIDI_SUPPORT
#define MIDI_OSS_DEVICE_PATH "/dev/sequencer"
#include "sys_midi_oss.h"
#endif /* ENABLE_MIDI_SUPPORT */

#else /* __CYGWIN32__ */

#include "sys_exec_windows.h"

#ifdef ENABLE_SHARED_OBJECTS
#include "sys_shlib_windows.h"
#endif /* ENABLE_SHARED_OBJECTS */

#ifdef ENABLE_PCM_SUPPORT
#include "sys_pcm_windows.h"
#endif /* ENABLE_PCM_SUPPORT */

#ifdef ENABLE_MIDI_SUPPORT
#include "sys_midi_windows.h"
#endif /* ENABLE_MIDI_SUPPORT */

#endif /* __CYGWIN32__ */


/* ntdll.dll */
WIN_PROC_STUB(NtSetInformationProcess);


/* kernel32.dll: console */
WIN_PROC_STUB(AttachConsole);


/* user32.dll */
WIN_PROC_STUB(GetAltTabInfoA);
WIN_PROC_STUB(SendInput);


#ifdef __MINGW32__
/* ws2_32.dll */
WIN_PROC_STUB(getaddrinfo);
WIN_PROC_STUB(freeaddrinfo);
#endif /* __MINGW32__ */


static void *
loadLibrary (const char *name) {
  HMODULE module = LoadLibrary(name);
  if (!module) LogPrint(LOG_DEBUG, "%s: %s", gettext("cannot load library"), name);
  return module;
}

static void *
getProcedure (HMODULE module, const char *name) {
  void *address = module? GetProcAddress(module, name): NULL;
  if (!address) LogPrint(LOG_DEBUG, "%s: %s", gettext("cannot find procedure"), name);
  return address;
}

static int
addWindowsCommandLineCharacter (char **buffer, int *size, int *length, char character) {
  if (*length == *size) {
    char *newBuffer = realloc(*buffer, (*size = *size? *size<<1: 0X80));
    if (!newBuffer) {
      LogError("realloc");
      return 0;
    }
    *buffer = newBuffer;
  }

  (*buffer)[(*length)++] = character;
  return 1;
}

char *
makeWindowsCommandLine (const char *const *arguments) {
  const char backslash = '\\';
  const char quote = '"';
  char *buffer = NULL;
  int size = 0;
  int length = 0;

#define ADD(c) if (!addWindowsCommandLineCharacter(&buffer, &size, &length, (c))) goto error
  while (*arguments) {
    const char *character = *arguments;
    int backslashCount = 0;
    int needQuotes = 0;
    int start = length;

    while (*character) {
      if (*character == backslash) {
        ++backslashCount;
      } else {
        if (*character == quote) {
          needQuotes = 1;
          backslashCount = (backslashCount * 2) + 1;
        } else if ((*character == ' ') || (*character == '\t')) {
          needQuotes = 1;
        }

        while (backslashCount > 0) {
          ADD(backslash);
          --backslashCount;
        }

        ADD(*character);
      }

      ++character;
    }

    if (needQuotes) backslashCount *= 2;
    while (backslashCount > 0) {
      ADD(backslash);
      --backslashCount;
    }

    if (needQuotes) {
      ADD(quote);
      ADD(quote);
      memmove(&buffer[start+1], &buffer[start], length-start-1);
      buffer[start] = quote;
    }

    ADD(' ');
    ++arguments;
  }
#undef ADD

  buffer[length-1] = 0;
  {
    char *line = realloc(buffer, length);
    if (line) return line;
    LogError("realloc");
  }

error:
  if (buffer) free(buffer);
  return NULL;
}

int
installService (const char *name, const char *description) {
  int installed = 0;
  const char *const arguments[] = {
    getProgramPath(),
    NULL
  };
  char *command = makeWindowsCommandLine(arguments);

  if (command) {
    SC_HANDLE scm = OpenSCManager(NULL, NULL, SC_MANAGER_CREATE_SERVICE);

    if (scm) {
      SC_HANDLE service = CreateService(scm, name, description, SERVICE_ALL_ACCESS,
                                        SERVICE_WIN32_OWN_PROCESS | SERVICE_INTERACTIVE_PROCESS,
                                        SERVICE_AUTO_START, SERVICE_ERROR_NORMAL,
                                        command, NULL, NULL, NULL, NULL, NULL);

      if (service) {
        LogPrint(LOG_NOTICE, "service installed: %s", name);
        installed = 1;

        CloseServiceHandle(service);
      } else if (GetLastError() == ERROR_SERVICE_EXISTS) {
        LogPrint(LOG_WARNING, "service already installed: %s", name);
        installed = 1;
      } else {
        LogWindowsError("CreateService");
      }

      CloseServiceHandle(scm);
    } else {
      LogWindowsError("OpenSCManager");
    }

    free(command);
  }

  return installed;
}

int
removeService (const char *name) {
  int removed = 0;
  SC_HANDLE scm = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);

  if (scm) {
    SC_HANDLE service = OpenService(scm, name, DELETE);

    if (service) {
      if (DeleteService(service)) {
        LogPrint(LOG_NOTICE, "service removed: %s", name);
        removed = 1;
      } else if (GetLastError() == ERROR_SERVICE_MARKED_FOR_DELETE) {
        LogPrint(LOG_WARNING, "service already being removed: %s", name);
        removed = 1;
      } else {
        LogWindowsError("DeleteService");
      }

      CloseServiceHandle(service);
    } else if (GetLastError() == ERROR_SERVICE_DOES_NOT_EXIST) {
      LogPrint(LOG_WARNING, "service not installed: %s", name);
      removed = 1;
    } else {
      LogWindowsError("OpenService");
    }

    CloseServiceHandle(scm);
  } else {
    LogWindowsError("OpenSCManager");
  }

  return removed;
}

void
sysInit (void) {
  HMODULE library;

#define LOAD_LIBRARY(name) (library = loadLibrary(name))
#define GET_PROC(name) (name##Proc = getProcedure(library, #name))

  if (LOAD_LIBRARY("ntdll.dll")) {
    GET_PROC(NtSetInformationProcess);
  }

  if (LOAD_LIBRARY("kernel32.dll")) {
    GET_PROC(AttachConsole);
  }

  if (LOAD_LIBRARY("user32.dll")) {
    GET_PROC(GetAltTabInfoA);
    GET_PROC(SendInput);
  }

#ifdef __MINGW32__
  if (LOAD_LIBRARY("ws2_32.dll")) {
    GET_PROC(getaddrinfo);
    GET_PROC(freeaddrinfo);
  }
#endif /* __MINGW32__ */
}
