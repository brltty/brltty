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

#ifndef BRLTTY_INCLUDED_SYS_WINDOWS
#define BRLTTY_INCLUDED_SYS_WINDOWS

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "prologue.h"

#define WIN_PROC_STUB(name) typeof(name) (*name##Proc)


/* winmm.dll */
extern int have_winmm;

/* winmm.dll: pcm */
extern WIN_PROC_STUB(waveOutGetErrorTextA);
extern WIN_PROC_STUB(waveOutGetNumDevs);
extern WIN_PROC_STUB(waveOutGetDevCapsA);
extern WIN_PROC_STUB(waveOutOpen);
extern WIN_PROC_STUB(waveOutClose);
extern WIN_PROC_STUB(waveOutWrite);
extern WIN_PROC_STUB(waveOutReset);
extern WIN_PROC_STUB(waveOutPrepareHeader);
extern WIN_PROC_STUB(waveOutUnprepareHeader);

/* winmm.dll: midi */
extern WIN_PROC_STUB(midiOutGetErrorTextA);
extern WIN_PROC_STUB(midiOutGetNumDevs);
extern WIN_PROC_STUB(midiOutGetDevCapsA);
extern WIN_PROC_STUB(midiOutOpen);
extern WIN_PROC_STUB(midiOutClose);
extern WIN_PROC_STUB(midiOutLongMsg);
extern WIN_PROC_STUB(midiOutPrepareHeader);
extern WIN_PROC_STUB(midiOutUnprepareHeader);


/* ntdll.dll */
#include <ntdef.h>

typedef enum _PROCESSINFOCLASS {
  ProcessUserModeIOPL = 16,
} PROCESSINFOCLASS, PROCESS_INFORMATION_CLASS;

extern NTSTATUS WINAPI NtSetInformationProcess (HANDLE, PROCESS_INFORMATION_CLASS, PVOID, ULONG);
extern WIN_PROC_STUB(NtSetInformationProcess);


/* kernel32.dll: console */
extern WIN_PROC_STUB(AttachConsole);

/* kernel32.dll: named pipe */
extern WIN_PROC_STUB(CreateNamedPipeA);
extern WIN_PROC_STUB(ConnectNamedPipe);
extern WIN_PROC_STUB(PeekNamedPipe);


/* user32.dll */
extern WIN_PROC_STUB(GetAltTabInfoA);


/* ws2_32.dll */
#include <ws2tcpip.h>

extern WIN_PROC_STUB(getaddrinfo);
extern WIN_PROC_STUB(freeaddrinfo);

#define getaddrinfo(host,port,hints,res) getaddrinfoProc(host,port,hints,res)
#define freeaddrinfo(res) freeaddrinfoProc(res)


extern void sysInit (void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BRLTTY_INCLUDED_SYS_WINDOWS */
