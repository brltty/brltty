/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2023 by The BRLTTY Developers.
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
#include "utf8.h"
#include "brl_cmds.h"
#include "embed.h"

typedef enum {
  PARM_FILE,
} ScreenParameters;

#define SCRPARMS "file"
#include "scr_driver.h"

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

static void
destruct_FileViewerScreen (void) {
  brlttyDisableInterrupt();

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
processParameters_FileViewerScreen (char **parameters) {
  filePath = parameters[PARM_FILE];
  if (filePath && !*filePath) filePath = NULL;

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
construct_FileViewerScreen (void) {
  fileCharacters = NULL;

  lineDescriptors = NULL;
  lineSize = 0;
  lineCount = 0;

  screenWidth = 0;
  cursorOffset = 0;

  loadFile();
  brlttyEnableInterrupt();
  return 1;
}

static int
poll_FileViewerScreen (void) {
  return 0;
}

static int
refresh_FileViewerScreen (void) {
  return 1;
}

static int
toScreenRow (int offset) {
  return offset / screenWidth;
}

static int
toScreenColumn (int offset) {
  return offset % screenWidth;
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
        target->attributes = SCR_COLOUR_DEFAULT;
        target += 1;
      }
    }

    return 1;
  }

  return 0;
}

static int
toScreenOffset (int row, int column) {
  return (row * screenWidth) + column;
}

static int
routeCursor_FileViewerScreen (int column, int row, int screen) {
  cursorOffset = toScreenOffset(row, column);
  return 1;
}

static void
moveCursor (int amount) {
  int newOffset = cursorOffset + amount;

  if ((newOffset >= 0) && (newOffset < (lineCount * screenWidth))) {
    cursorOffset = newOffset;
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
        cursorOffset = toScreenOffset(row+1, 0);
        return;
      }
    }
  }

  if (wasBlank) {
    alert(ALERT_COMMAND_REJECTED);
  } else {
    cursorOffset = toScreenOffset(row, 0);
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
        cursorOffset = toScreenOffset(row, 0);
        return;
      }
    }

    row += 1;
  }

  alert(ALERT_COMMAND_REJECTED);
}

static int
handleCommand_FileViewerScreen (int command) {
  switch (command) {
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

    case BRL_CMD_KEY(HOME):
      cursorOffset = 0;
      return 1;

    case BRL_CMD_KEY(END):
      cursorOffset = toScreenOffset(lineCount-1, 0);
      return 1;
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
