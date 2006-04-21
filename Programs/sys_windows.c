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
#include "sys_windows.h"

#include "misc.h"
#include "system.h"

#include "sys_prog_windows.h"

#include "sys_boot_none.h"

#include "sys_exec_windows.h"

#include "sys_shlib_windows.h"

#include "sys_beep_windows.h"

#ifdef ENABLE_PCM_SUPPORT
#include "sys_pcm_windows.h"
#endif /* ENABLE_PCM_SUPPORT */

#ifdef ENABLE_MIDI_SUPPORT
#include "sys_midi_windows.h"
#endif /* ENABLE_MIDI_SUPPORT */

#include "sys_ports_windows.h"


/* ntdll.dll */
WIN_PROC_STUB(NtSetInformationProcess);


/* kernel32.dll: console */
WIN_PROC_STUB(AttachConsole);


/* user32.dll */
WIN_PROC_STUB(GetAltTabInfoA);


/* ws2_32.dll */
WIN_PROC_STUB(getaddrinfo);
WIN_PROC_STUB(freeaddrinfo);


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
  }

  if (LOAD_LIBRARY("ws2_32.dll")) {
    GET_PROC(getaddrinfo);
    GET_PROC(freeaddrinfo);
  }
}
