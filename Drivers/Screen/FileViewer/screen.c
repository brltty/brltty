/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2025 by The BRLTTY Developers.
 *
 * BRLTTY comes with ABSOLUTELY NO WARRANTY.
 *
 * This is free software, placed under the terms of the
 * GNU Lesser General Public License, as published by the Free Software
 * Foundation; either version 2.1 of the License, or (at your option) any
 * later version. Please see the file LICENSE-LGPL for details.
 *
 * Web Page: http://brltty.app/
 *
 * This software is maintained by Dave Mielke <dave@mielke.cc>.
 */

#include "prologue.h"

#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>

#include "log.h"
#include "alert.h"
#include "strfmt.h"
#include "parse.h"
#include "utf8.h"
#include "brl_cmds.h"
#include "embed.h"

typedef enum {
  PARM_FILE,
  PARM_SHOW,
} ScreenParameters;

#define SCRPARMS "file", "show"
#include "scr_driver.h"

#include <term.h>
#include <stdio.h>

#include "get_curses.h"

static unsigned int showOnTerminal = 0;

static const char *filePath;
static wchar_t *fileCharacters;

typedef struct {
  unsigned int offset;
  unsigned int length;
} LineDescriptor;

static LineDescriptor *lineDescriptors;
static unsigned int lineSize;
static unsigned int lineCount;

static int screenWidth;
static int cursorOffset;

static int
processParameters_FileViewerScreen (char **parameters) {
  filePath = parameters[PARM_FILE];
  if (filePath && !*filePath) filePath = NULL;

  showOnTerminal = 0;
  {
    const char *parameter = parameters[PARM_SHOW];

    if (parameter && *parameter) {
      if (!validateYesNo(&showOnTerminal, parameter)) {
        logMessage(LOG_WARNING, "%s: %s", "invalid show setting", parameter);
      }
    }
  }

  return 1;
}

static int
addLine (const wchar_t *from, const wchar_t *to) {
  size_t lineLength = to - from;
  if (lineLength > screenWidth) screenWidth = lineLength;

  if (lineCount == lineSize) {
    size_t newSize = lineSize? lineSize<<1: 0X80;
    LineDescriptor *newArray = realloc(lineDescriptors, ARRAY_SIZE(lineDescriptors, newSize));

    if (!newArray) {
      logMallocError();
      return 0;
    }

    lineDescriptors = newArray;
    lineSize = newSize;
  }

  LineDescriptor *line = &lineDescriptors[lineCount++];
  line->offset = from - fileCharacters;
  line->length = to - from;

  return 1;
}

static int
setScreenContent (const char *text) {
  unsigned int characterCount = countUtf8Characters(text);
  fileCharacters = malloc(characterCount * sizeof(*fileCharacters));

  if (fileCharacters) {
    makeWcharsFromUtf8(text, fileCharacters, characterCount);

    const wchar_t *current = fileCharacters;
    const wchar_t *end = current + characterCount;

    while (current < end) {
      wchar_t *next = wcschr(current, WC_C('\n'));

      if (!next) {
        if (!addLine(current, end)) return 0;
        break;
      }

      if (!addLine(current, next)) return 0;
      current = next + 1;
    }

    return 1;
  } else {
    logMallocError();
  }

  return 0;
}

static int
loadFile (void) {
  const char *problem = NULL;

  if (filePath) {
    struct stat status;

    if (stat(filePath, &status) != -1) {
      size_t fileSize = status.st_size;
      char *text = malloc(fileSize + 1);

      if (text) {
        int fileDescriptor = open(filePath, O_RDONLY);

        if (fileDescriptor != -1) {
          ssize_t result = read(fileDescriptor, text, fileSize);

          if (result != -1) {
            if (result < fileSize) fileSize = result;
            text[fileSize] = 0;
            setScreenContent(text);
          } else {
            problem = strerror(errno);
          }

          close(fileDescriptor);
        } else {
          problem = strerror(errno);
        }

        free(text);
      } else {
        problem = strerror(errno);
      }
    } else {
      problem = strerror(errno);
    }
  } else {
    problem = gettext("file not specified");
    filePath = NULL;
  }

  if (!problem) return 1;
  char log[0X100];
  STR_BEGIN(log, sizeof(log));

  if (filePath) STR_PRINTF("%s: ", filePath);
  STR_PRINTF("%s", problem);

  STR_END;
  logMessage(LOG_WARNING, "%s", log);

  setScreenContent(log);
  return 0;
}

static int
toScreenRow (int offset) {
  return offset / screenWidth;
}

static int
toScreenColumn (int offset) {
  return offset % screenWidth;
}

#ifdef GOT_CURSES
static int terminalInitialized = 0;

static void
writeTerminalString (const char *string) {
  if (string) {
    putp(string);
  }
}

static void
writeTerminalCapability0 (const char *name) {
  writeTerminalString(tigetstr(name));
}

static void
writeTerminalCapability2 (const char *name, int p1, int p2) {
  writeTerminalString(tiparm(tigetstr(name), p1, p2));
}

static void
clearTerminalScreen (void) {
  writeTerminalCapability0("clear");
}

static void
placeTerminalCursor (int row, int column) {
  writeTerminalCapability2("cup", row, column);
}

static void
refreshTerminalScreen (int oldOffset) {
  if (terminalInitialized) {
    int newRow = toScreenRow(cursorOffset);
    int newFrom = (newRow / lines) * lines;
    int refresh = oldOffset < 0;

    if (!refresh) {
      int oldRow = toScreenRow(oldOffset);
      int oldFrom = (oldRow / lines) * lines;
      if (newFrom != oldFrom) refresh = 1;
    }

    if (refresh) {
      clearTerminalScreen();

      int to = newFrom + lines;
      if (to > lineCount) to = lineCount;

      const LineDescriptor *line = &lineDescriptors[newFrom];
      const LineDescriptor *lineEnd = &lineDescriptors[to];
      int row = 0;

      while (line < lineEnd) {
        placeTerminalCursor(row, 0);

        unsigned int count = line->length;
        int tooLong = count > columns;
        if (tooLong) count = columns - 1;

        {
          const wchar_t *wc = &fileCharacters[line->offset];
          const wchar_t *wcEnd = wc + count;

          while (wc < wcEnd) {
            Utf8Buffer utf8;
            convertCodepointToUtf8(*wc++, utf8);
            writeTerminalString(utf8);
          }
        }

        if (tooLong) {
          static Utf8Buffer ellipsis = {0};
          if (!ellipsis[0]) convertCodepointToUtf8(0X2026, ellipsis);
          writeTerminalString(ellipsis);
        }

        row += 1;
        line += 1;
      }
    }

    int column = toScreenColumn(cursorOffset);
    if (column >= columns) column = columns - 1;
    placeTerminalCursor((newRow - newFrom), column);
    fflush(stdout);
  }
}

static int
beginTerminal (void) {
  const char *problem = "terminal initialization failure";

  if (isatty(STDOUT_FILENO)) {
    int error;
    int result = setupterm(NULL, STDOUT_FILENO, &error);
    int initialized = result == OK;

    if (initialized) {
      writeTerminalCapability2("csr", 0, lines-1);
      writeTerminalCapability0("smcup");

      logMessage(
        LOG_CATEGORY(SCREEN_DRIVER),
        "terminal successfully initialized"
      );

      return 1;
    }

    if (result == ERR) {
      if (error == 0) {
        problem = "terminal is generic";
      } else if (error == 1) {
        problem = "terminal is hardcopy";
      } else if (error == -1) {
        problem = "terminfo database not found";
      }
    }
  } else {
    problem = "standard output isn't a terminal";
  }

  logMessage(LOG_WARNING, "%s", problem);
  return 0;
}

static void
endTerminal (void) {
  clearTerminalScreen();
  writeTerminalCapability0("rmcup");
  reset_shell_mode();
}
#endif /* GOT_CURSES */

static int
construct_FileViewerScreen (void) {
  fileCharacters = NULL;

  lineDescriptors = NULL;
  lineSize = 0;
  lineCount = 0;

  screenWidth = 0;
  loadFile();
  cursorOffset = 0;

  if (showOnTerminal) {
    #ifdef GOT_CURSES
    terminalInitialized = beginTerminal();
    refreshTerminalScreen(-1);
    #endif /* GOT_CURSES */
  }

  brlttyEnableInterrupt();
  return 1;
}

static void
destruct_FileViewerScreen (void) {
  brlttyDisableInterrupt();

  #ifdef GOT_CURSES
  if (terminalInitialized) endTerminal();
  #endif /* GOT_CURSES */

  if (lineDescriptors) {
    free(lineDescriptors);
    lineDescriptors = NULL;
  }

  if (fileCharacters) {
    free(fileCharacters);
    fileCharacters = NULL;
  }
}

static int
poll_FileViewerScreen (void) {
  return 0;
}

static int
refresh_FileViewerScreen (void) {
  return 1;
}

static void
describe_FileViewerScreen (ScreenDescription *description) {
  description->rows = lineCount;
  description->cols = screenWidth;
  description->posy = toScreenRow(cursorOffset);
  description->posx = toScreenColumn(cursorOffset);
}

static int
readCharacters_FileViewerScreen (const ScreenBox *box, ScreenCharacter *buffer) {
  if (validateScreenBox(box, screenWidth, lineCount)) {
    ScreenCharacter *target = buffer;

    for (unsigned int row=0; row<box->height; row+=1) {
      const LineDescriptor *line = &lineDescriptors[box->top + row];

      unsigned int from = box->left;
      unsigned int to = from + box->width;

      for (unsigned int column=from; column<to; column+=1) {
        target->text = (column < line->length)? fileCharacters[line->offset + column]: WC_C(' ');
        target->color.vgaAttributes = VGA_COLOR_DEFAULT;
        target += 1;
      }
    }

    return 1;
  }

  return 0;
}

static void
placeCursor (int newOffset) {
  int oldOffset = cursorOffset;

  if (newOffset != oldOffset) {
    cursorOffset = newOffset;

    #ifdef GOT_CURSES
    refreshTerminalScreen(oldOffset);
    #endif /* GOT_CURSES */
  }
}

static int
toScreenOffset (int row, int column) {
  return (row * screenWidth) + column;
}

static int
routeCursor_FileViewerScreen (int column, int row, int screen) {
  placeCursor(toScreenOffset(row, column));
  return 1;
}

static void
moveCursor (int amount) {
  int newOffset = cursorOffset + amount;

  if ((newOffset >= 0) && (newOffset < (lineCount * screenWidth))) {
    placeCursor(newOffset);
  } else {
    alert(ALERT_COMMAND_REJECTED);
  }
}

static int
isBlankRow (int row) {
  const LineDescriptor *line = &lineDescriptors[row];
  const wchar_t *character = &fileCharacters[line->offset];
  const wchar_t *end = character + line->length;

  while (character < end) {
    if (!iswspace(*character)) return 0;
    character += 1;
  }

  return 1;
}

static void
findPreviousParagraph (void) {
  int wasBlank = 1;
  int row = toScreenRow(cursorOffset);

  while (row > 0) {
    int isBlank = isBlankRow(--row);

    if (isBlank != wasBlank) {
      if ((wasBlank = isBlank)) {
        placeCursor(toScreenOffset(row+1, 0));
        return;
      }
    }
  }

  if (wasBlank) {
    alert(ALERT_COMMAND_REJECTED);
  } else {
    placeCursor(toScreenOffset(row, 0));
  }
}

static void
findNextParagraph (void) {
  int wasBlank = 0;
  int row = toScreenRow(cursorOffset);

  while (row < lineCount) {
    int isBlank = isBlankRow(row);

    if (isBlank != wasBlank) {
      if (!(wasBlank = isBlank)) {
        placeCursor(toScreenOffset(row, 0));
        return;
      }
    }

    row += 1;
  }

  alert(ALERT_COMMAND_REJECTED);
}

static int
handleCommand_FileViewerScreen (int command) {
  int hasControl = command & BRL_FLG_INPUT_CONTROL;

  switch (command & BRL_MSK_CMD) {
    case BRL_CMD_KEY(ESCAPE):
      brlttyInterrupt(WAIT_STOP);
      return 1;

    case BRL_CMD_KEY(CURSOR_LEFT):
      moveCursor(-1);
      return 1;

    case BRL_CMD_KEY(CURSOR_RIGHT):
      moveCursor(1);
      return 1;

    case BRL_CMD_KEY(CURSOR_UP):
      moveCursor(-screenWidth);
      return 1;

    case BRL_CMD_KEY(CURSOR_DOWN):
      moveCursor(screenWidth);
      return 1;

    case BRL_CMD_KEY(PAGE_UP):
      findPreviousParagraph();
      return 1;

    case BRL_CMD_KEY(PAGE_DOWN):
      findNextParagraph();
      return 1;

    case BRL_CMD_KEY(HOME): {
      if (hasControl) {
        placeCursor(0);
      } else {
        moveCursor(-toScreenColumn(cursorOffset));
      }

      return 1;
    }

    case BRL_CMD_KEY(END): {
      if (hasControl) {
        placeCursor(toScreenOffset(lineCount-1, 0));
      } else {
        int row = toScreenRow(cursorOffset);
        const LineDescriptor *line = &lineDescriptors[row];
        placeCursor(toScreenOffset(row, (line->length - 1)));
      }

      return 1;
    }

    default: {
      switch (command & BRL_MSK_BLK) {
        case BRL_CMD_BLK(PASSDOTS):
        case BRL_CMD_BLK(PASSCHAR):
          alert(ALERT_COMMAND_REJECTED);
          return 1;
      }
    }
  }

  return 0;
}

static void
scr_initialize (MainScreen *main) {
  initializeRealScreen(main);

  main->base.poll = poll_FileViewerScreen;
  main->base.refresh = refresh_FileViewerScreen;

  main->base.describe = describe_FileViewerScreen;
  main->base.readCharacters = readCharacters_FileViewerScreen;

  main->base.routeCursor = routeCursor_FileViewerScreen;
  main->base.handleCommand = handleCommand_FileViewerScreen;

  main->processParameters = processParameters_FileViewerScreen;
  main->construct = construct_FileViewerScreen;
  main->destruct = destruct_FileViewerScreen;
}
