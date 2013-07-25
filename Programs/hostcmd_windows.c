/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2013 by The BRLTTY Developers.
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

#include "log.h"
#include "sys_windows.h"
#include "hostcmd_windows.h"
#include "hostcmd_internal.h"

int
isHostCommand (const char *path) {
  return 0;
}

int
constructHostCommandPackageData (HostCommandPackageData *pkg) {
  pkg->inputHandle = INVALID_HANDLE_VALUE;
  pkg->outputHandle = INVALID_HANDLE_VALUE;
  return 1;
}

static void
closeHandle (HANDLE *handle) {
  if (*handle != INVALID_HANDLE_VALUE) {
    if (*handle) {
      CloseHandle(*handle);
    }

    *handle = INVALID_HANDLE_VALUE;
  }
}

void
destructHostCommandPackageData (HostCommandPackageData *pkg) {
  closeHandle(&pkg->inputHandle);
  closeHandle(&pkg->outputHandle);
}

int
prepareHostCommandStream (HostCommandStream *hcs, void *data) {
  SECURITY_ATTRIBUTES attributes;

  ZeroMemory(&attributes, sizeof(attributes));
  attributes.nLength = sizeof(attributes);
  attributes.bInheritHandle = TRUE;
  attributes.lpSecurityDescriptor = NULL;

  if (CreatePipe(&hcs->package.inputHandle, &hcs->package.outputHandle, &attributes, 0)) {
    HANDLE parentHandle = hcs->isInput? hcs->package.outputHandle: hcs->package.inputHandle;

    if (SetHandleInformation(parentHandle, HANDLE_FLAG_INHERIT, 0)) {
      return 1;
    } else {
      logWindowsSystemError("SetHandleInformation");
    }
  } else {
    logWindowsSystemError("CreatePipe");
  }

  return 0;
}

int
runCommand (
  int *result,
  const char *const *command,
  HostCommandStream *streams,
  int asynchronous
) {
  int ok = 0;
  char *line = makeWindowsCommandLine(command);

  if (line) {
    STARTUPINFO startup;
    PROCESS_INFORMATION process;

    ZeroMemory(&startup, sizeof(startup));
    startup.cb = sizeof(startup);
    startup.dwFlags = STARTF_USESTDHANDLES;

    ZeroMemory(&process, sizeof(process));

    logMessage(LOG_DEBUG, "host command: %s", line);
    if (CreateProcess(NULL, line, NULL, NULL, TRUE,
                      CREATE_NEW_PROCESS_GROUP,
                      NULL, NULL, &startup, &process)) {
      DWORD status;

      ok = 1;

      if (asynchronous) {
        *result = 0;
      } else {
        *result = 0XFF;

        while ((status = WaitForSingleObject(process.hProcess, INFINITE)) == WAIT_TIMEOUT);

        if (status == WAIT_OBJECT_0) {
          DWORD code;

          if (GetExitCodeProcess(process.hProcess, &code)) {
            *result = code;
          } else {
            logWindowsSystemError("GetExitCodeProcess");
          }
        } else {
          logWindowsSystemError("WaitForSingleObject");
        }
      }

      CloseHandle(process.hProcess);
      CloseHandle(process.hThread);
    } else {
      logWindowsSystemError("CreateProcess");
    }

    free(line);
  } else {
    logMallocError();
  }

  return ok;
}
