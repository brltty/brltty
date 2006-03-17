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

#if !defined(WINDOWS_NT)
#define USE_PORTS_X86

int
enablePorts (int errorLevel, unsigned short int base, unsigned short int count) {
  return 1;
}

int
disablePorts (unsigned short int base, unsigned short int count) {
  return 1;
}
#elif defined(HAVE_LIBNTDLL)
#define USE_PORTS_X86

#include <ntdef.h>

typedef enum _PROCESSINFOCLASS {
  ProcessUserModeIOPL = 16,
} PROCESSINFOCLASS, PROCESS_INFORMATION_CLASS;

NTSTATUS WINAPI NtSetInformationProcess (HANDLE, PROCESS_INFORMATION_CLASS, PVOID, ULONG);

static int portsEnabled = 0;

int
enablePorts (int errorLevel, unsigned short int base, unsigned short int count) {
  if (!portsEnabled) {
    ULONG Iopl=3;
    if (NtSetInformationProcess(GetCurrentProcess(), ProcessUserModeIOPL,
                                &Iopl, sizeof(Iopl)) != STATUS_SUCCESS) {
      return 0;
    }
    portsEnabled = 1;
  }
  return 1;
}

int
disablePorts (unsigned short int base, unsigned short int count) {
  return 1;
}
#endif

#if defined(USE_PORTS_X86)
#include "sys_ports_x86.h"
#else /* USE_PORTS_ */
#include "sys_ports_none.h"
#endif /* USE_PORTS_ */
