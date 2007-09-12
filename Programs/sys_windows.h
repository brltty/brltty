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

#ifndef BRLTTY_INCLUDED_SYS_WINDOWS
#define BRLTTY_INCLUDED_SYS_WINDOWS

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "prologue.h"

#define WIN_PROC_STUB(name) typeof(name) (*name##Proc)


/* ntdll.dll */
#include <ntdef.h>

typedef enum _PROCESSINFOCLASS {
  ProcessUserModeIOPL = 16,
} PROCESSINFOCLASS, PROCESS_INFORMATION_CLASS;

extern NTSTATUS WINAPI NtSetInformationProcess (HANDLE, PROCESS_INFORMATION_CLASS, PVOID, ULONG);
extern WIN_PROC_STUB(NtSetInformationProcess);


/* kernel32.dll: console */
extern WIN_PROC_STUB(AttachConsole);


/* user32.dll */
extern WIN_PROC_STUB(GetAltTabInfoA);
extern WIN_PROC_STUB(SendInput);


/* ws2_32.dll */
#ifdef __MINGW32__
#include <ws2tcpip.h>

extern WIN_PROC_STUB(getaddrinfo);
extern WIN_PROC_STUB(freeaddrinfo);

#define getaddrinfo(host,port,hints,res) getaddrinfoProc(host,port,hints,res)
#define freeaddrinfo(res) freeaddrinfoProc(res)
#endif /* __MINGW32__ */


extern int installService (const char *name, const char *description);
extern int uninstallWindowsService (const char *name);

extern void sysInit (void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BRLTTY_INCLUDED_SYS_WINDOWS */
