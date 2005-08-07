/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2005 by The BRLTTY Team. All rights reserved.
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

#if _WIN32_WINNT >= WindowsNT4

#ifdef HAVE_LIBNTDLL

#include <ntdef.h>

typedef enum _PROCESSINFOCLASS {
    ProcessUserModeIOPL = 16,
} PROCESSINFOCLASS, PROCESS_INFORMATION_CLASS;

NTSTATUS WINAPI NtSetInformationProcess(HANDLE,PROCESS_INFORMATION_CLASS,PVOID,ULONG);

#include "rangelist.h"

static rangeList *enabledPorts;

int
enablePorts (int errorLevel, unsigned short int base, unsigned short int count) {
  if (!enabledPorts) {
    ULONG Iopl=3;
    if (NtSetInformationProcess(GetCurrentProcess(), ProcessUserModeIOPL,
	  &Iopl, sizeof(Iopl)) != STATUS_SUCCESS) return 0;
  }
  return !addRange(base, base+count, &enabledPorts);
}

int
disablePorts (unsigned short int base, unsigned short int count) {
  if (!enabledPorts)
    return 1;
  if (removeRange(base, base+count, &enabledPorts))
    return 0;
  if (!enabledPorts) {
    ULONG Iopl=0;
    if (NtSetInformationProcess(GetCurrentProcess(), ProcessUserModeIOPL,
	  &Iopl, sizeof(Iopl)) != STATUS_SUCCESS) return 0;
  }
  return 1;
}

#include "sys_ports_x86.h"

#else /* HAVE_LIBNTDLL */

#include "sys_ports_none.h"

#endif /* HAVE_LIBNTDLL */

#else /* _WIN32_WINNT */

int
enablePorts (int errorLevel, unsigned short int base, unsigned short int count) {
  return 1;
}

int
disablePorts (unsigned short int base, unsigned short int count) {
  return 1;
}

#include "sys_ports_x86.h"

#endif /* _WIN32_WINNT */
