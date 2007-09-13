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

int
executeHostCommand (const char *const *arguments) {
  int result = 0XFF;
  char *line;

  if ((line = makeWindowsCommandLine(arguments))) {
    STARTUPINFO startup;
    PROCESS_INFORMATION process;

    memset(&startup, 0, sizeof(startup));
    startup.cb = sizeof(startup);

    memset(&process, 0, sizeof(process));

    LogPrint(LOG_DEBUG, "host command: %s", line);
    if (CreateProcess(NULL, line, NULL, NULL, TRUE,
                      CREATE_NEW_PROCESS_GROUP,
                      NULL, NULL, &startup, &process)) {
      DWORD status;
      while ((status = WaitForSingleObject(process.hProcess, INFINITE)) == WAIT_TIMEOUT);
      if (status == WAIT_OBJECT_0) {
        DWORD code;
        if (GetExitCodeProcess(process.hProcess, &code)) {
          result = code;
        } else {
          LogWindowsError("GetExitCodeProcess");
        }
      } else {
        LogWindowsError("WaitForSingleObject");
      }

      CloseHandle(process.hProcess);
      CloseHandle(process.hThread);
    } else {
      LogWindowsError("CreateProcess");
    }

    free(line);
  }

  return result;
}
