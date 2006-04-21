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


/* winmm.dll */
int have_winmm;

/* winmm.dll: pcm */
WIN_PROC_STUB(waveOutGetErrorTextA);
WIN_PROC_STUB(waveOutGetNumDevs);
WIN_PROC_STUB(waveOutGetDevCapsA);
WIN_PROC_STUB(waveOutOpen);
WIN_PROC_STUB(waveOutClose);
WIN_PROC_STUB(waveOutWrite);
WIN_PROC_STUB(waveOutReset);
WIN_PROC_STUB(waveOutPrepareHeader);
WIN_PROC_STUB(waveOutUnprepareHeader);

/* winmm.dll: midi */
WIN_PROC_STUB(midiOutGetErrorTextA);
WIN_PROC_STUB(midiOutGetNumDevs);
WIN_PROC_STUB(midiOutGetDevCapsA);
WIN_PROC_STUB(midiOutOpen);
WIN_PROC_STUB(midiOutClose);
WIN_PROC_STUB(midiOutLongMsg);
WIN_PROC_STUB(midiOutPrepareHeader);
WIN_PROC_STUB(midiOutUnprepareHeader);


/* ntdll.dll */
WIN_PROC_STUB(NtSetInformationProcess);


/* kernel32.dll: console */
WIN_PROC_STUB(AttachConsole);

/* kernel32.dll: named pipe */
WIN_PROC_STUB(CreateNamedPipeA);
WIN_PROC_STUB(ConnectNamedPipe);
WIN_PROC_STUB(PeekNamedPipe);


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

  if (LOAD_LIBRARY("winmm.dll")) {
    have_winmm = 1;

    GET_PROC(waveOutGetErrorTextA);
    GET_PROC(waveOutGetNumDevs);
    GET_PROC(waveOutGetDevCapsA);
    GET_PROC(waveOutOpen);
    GET_PROC(waveOutClose);
    GET_PROC(waveOutWrite);
    GET_PROC(waveOutReset);
    GET_PROC(waveOutPrepareHeader);
    GET_PROC(waveOutUnprepareHeader);
  
    GET_PROC(midiOutGetErrorTextA);
    GET_PROC(midiOutGetNumDevs);
    GET_PROC(midiOutGetDevCapsA);
    GET_PROC(midiOutOpen);
    GET_PROC(midiOutClose);
    GET_PROC(midiOutLongMsg);
    GET_PROC(midiOutPrepareHeader);
    GET_PROC(midiOutUnprepareHeader);
  }

  if (LOAD_LIBRARY("ntdll.dll")) {
    GET_PROC(NtSetInformationProcess);
  }

  if (LOAD_LIBRARY("kernel32.dll")) {
    GET_PROC(AttachConsole);

    GET_PROC(CreateNamedPipeA);
    GET_PROC(ConnectNamedPipe);
    GET_PROC(PeekNamedPipe);
  }

  if (LOAD_LIBRARY("user32.dll")) {
    GET_PROC(GetAltTabInfoA);
  }

  if (LOAD_LIBRARY("ws2_32.dll")) {
    GET_PROC(getaddrinfo);
    GET_PROC(freeaddrinfo);
  }
}
