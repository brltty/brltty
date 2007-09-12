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

#ifdef __CYGWIN32__

#include "sys_exec_unix.h"

#define SHARED_OBJECT_LOAD_FLAGS (RTLD_NOW | RTLD_GLOBAL)
#include "sys_shlib_dlfcn.h"

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

#include "sys_shlib_windows.h"

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

int
installService (const char *name, const char *description) {
  int installed = 0;
  SC_HANDLE scm = OpenSCManager(NULL, NULL, SC_MANAGER_CREATE_SERVICE);

  if (scm) {
    SC_HANDLE service = CreateService(scm, name, description, SERVICE_ALL_ACCESS,
                                      SERVICE_WIN32_OWN_PROCESS | SERVICE_INTERACTIVE_PROCESS,
                                      SERVICE_AUTO_START, SERVICE_ERROR_NORMAL,
                                      getProgramPath(),
                                      NULL, NULL, NULL, NULL, NULL);

    if (service) {
      installed = 1;
      CloseServiceHandle(service);
    } else {
      LogWindowsError("CreateService");
    }

    CloseServiceHandle(scm);
  } else {
    LogWindowsError("OpenSCManager");
  }

  return installed;
}

int
removeService (const char *name) {
  int removed = 0;
  SC_HANDLE scm = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);

  if (scm) {
    SC_HANDLE service = OpenService(scm, name, SERVICE_ALL_ACCESS);

    if (service) {
      if (DeleteService(service)) {
        removed = 1;
      } else {
        LogWindowsError("DeleteService");
      }

      CloseServiceHandle(service);
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
