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

static int
addWindowsCommandLineCharacter (char **buffer, int *size, int *length, char character) {
  if (*length == *size) {
    char *newBuffer = realloc(*buffer, (*size = *size? *size<<1: 0X1));
    if (!newBuffer) {
      LogError("realloc");
      return 0;
    }
    *buffer = newBuffer;
  }

  (*buffer)[(*length)++] = character;
  return 1;
}

static char *
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
