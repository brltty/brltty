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

#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>

#ifndef O_BINARY
#define O_BINARY 0
#endif /* O_BINARY */

#include "misc.h"
#include "scr.h"
#include "scr_help.h"

static int fileDescriptor;
static HelpFileHeader fileHeader;
static short pageNumber;
static unsigned char cursorRow, cursorColumn;
static HelpPageEntry *pageDescriptions;
static unsigned char **pages;
static unsigned char *characters;

static void
close_HelpScreen (void) {
  if (characters) {
    free(characters);
    characters = NULL;
  }

  if (pages) {
    free(pages);
    pages = NULL;
  }

  if (pageDescriptions) {
    free(pageDescriptions);
    pageDescriptions = NULL;
  }

  if (fileDescriptor != -1) {
    close(fileDescriptor);
    fileDescriptor = -1;
  }
}

static int
loadPages (const char *file) {
  int empty = 0;
  unsigned long characterCount = 0;
  int bytesRead;

  if ((fileDescriptor = open(file, O_RDONLY|O_BINARY)) == -1) {
    LogError("Help file open");
    goto failure;
  }

  if ((bytesRead = read(fileDescriptor, &fileHeader, sizeof(fileHeader))) == -1) {
    LogError("Help file read");
    goto failure;
  }

  if (bytesRead == 0) {
    LogPrint(LOG_WARNING, "Help file is empty.");
    empty = 1;
    fileHeader.pages = 1;
  } else if (bytesRead != sizeof(fileHeader)) {
    LogPrint(LOG_ERR, "Help file corrupt");
    goto failure;
  }

  if (fileHeader.pages < 1) {
    LogPrint(LOG_ERR, "Help file corrupt");
    goto failure;
  }

  {
    int size = sizeof(*pageDescriptions) * fileHeader.pages;

    if (!(pageDescriptions = malloc(size))) {
      LogError("Help page descriptions allocation");
      goto failure;
    }

    if (empty) {
      putBigEndian(&pageDescriptions->height, 1);
      putBigEndian(&pageDescriptions->width, 1);
    } else {
      if ((bytesRead = read(fileDescriptor, pageDescriptions, size)) == -1) {
        LogError("Help file read");
        goto failure;
      }

      if (bytesRead != size) {
        LogPrint(LOG_ERR, "Help file corrupt");
        goto failure;
      }
    }
  }

  {
    int page;
    for (page=0; page<fileHeader.pages; page++) {
      HelpPageEntry *description = &pageDescriptions[page];
      characterCount += getBigEndian(description->height) * getBigEndian(description->width);
    }
  }

  if (!(pages = calloc(fileHeader.pages, sizeof(*pages)))) {
    LogError("Help page addresses allocation");
    goto failure;
  }
  if (!(characters = calloc(characterCount, sizeof(*characters)))) {
    LogError("Help page buffer allocation");
    goto failure;
  }

  pages[0] = characters;
  {
    int page;
    for (page=0; page<(fileHeader.pages-1); page++) {
      const HelpPageEntry *description = &pageDescriptions[page];
      pages[page+1] = pages[page] + (getBigEndian(description->height) * getBigEndian(description->width));
    }
  }

  {
    int page;
    for (page=0; page<fileHeader.pages; page++) {
      const HelpPageEntry *description = &pageDescriptions[page];
      unsigned char *buffer = pages[page];
      {
        int row;
        for (row=0; row<getBigEndian(description->height); row++) {
          unsigned char lineLength;
          unsigned char *line = &buffer[row * getBigEndian(description->width)];

          if (empty) {
            lineLength = 1;
          } else {
            if ((bytesRead = read(fileDescriptor, &lineLength, sizeof(lineLength))) == -1) {
              LogError("Help line length read");
              goto failure;
            }

            if (bytesRead != sizeof(lineLength)) {
              LogPrint(LOG_ERR, "Help file corrupt.");
              goto failure;
            }
          }

          if (lineLength) {
            if (empty) {
              memset(line, ' ', lineLength);
            } else {
              if ((bytesRead = read(fileDescriptor, line, lineLength)) == -1) {
                LogError("Help line read");
                goto failure;
              }

              if (bytesRead != lineLength) {
                LogPrint(LOG_ERR, "Help file corrupt");
                goto failure;
              }
            }
          }
          memset(&line[lineLength], ' ', getBigEndian(description->width)-lineLength);
        }
      }
    }
  }

  close(fileDescriptor);
  fileDescriptor = -1;
  return 1;

failure:
  close_HelpScreen();
  return 0;
}

static int
open_HelpScreen (const char *file) {
  if (!pages)
    if (!loadPages(file))
      return 0;
  if ((pageNumber < 0) || (pageNumber >= fileHeader.pages)) pageNumber = 0;
  return 1;
}

static void
setPageNumber_HelpScreen (short page) {
  pageNumber = page;
}

static short
getPageNumber_HelpScreen (void) {
  return pageNumber;
}

static short
getPageCount_HelpScreen (void) {
  return (fileDescriptor != -1)? fileHeader.pages: 0;
}

static int
currentvt_HelpScreen (void) {
  return userVirtualTerminal(1);
}

static void
describe_HelpScreen (ScreenDescription *description) {
  const HelpPageEntry *page = &pageDescriptions[pageNumber];
  description->posx = cursorColumn;
  description->posy = cursorRow;
  description->cols = getBigEndian(page->width);
  description->rows = getBigEndian(page->height);
  description->number = currentvt_HelpScreen();
}

static int
read_HelpScreen (ScreenBox box, unsigned char *buffer, ScreenMode mode) {
  const HelpPageEntry *description = &pageDescriptions[pageNumber];
  if (validateScreenBox(&box, getBigEndian(description->width), getBigEndian(description->height))) {
    if (mode == SCR_TEXT) {
       int row;
      for (row=0; row<box.height; row++) {
        memcpy(buffer + (row * box.width),
               pages[pageNumber] + ((box.top + row) * getBigEndian(description->width)) + box.left,
               box.width);
      }
    } else {
      memset(buffer, 0X07, box.width*box.height);
    }
    return 1;
  }
  return 0;
}

static int
insert_HelpScreen (ScreenKey key) {
  switch (key) {
    case SCR_KEY_PAGE_UP:
      if (pageNumber > 0) {
        --pageNumber;
        cursorRow = cursorColumn = 0;
        return 1;
      }
      break;
    case SCR_KEY_PAGE_DOWN:
      if (pageNumber < (fileHeader.pages - 1)) {
        ++pageNumber;
        cursorRow = cursorColumn = 0;
        return 1;
      }
      break;
    case SCR_KEY_CURSOR_UP:
      if (cursorRow > 0) {
        --cursorRow;
        return 1;
      }
      break;
    case SCR_KEY_CURSOR_DOWN:
      if (cursorRow < (getBigEndian(pageDescriptions[pageNumber].height) - 1)) {
        ++cursorRow;
        return 1;
      }
      break;
    case SCR_KEY_CURSOR_LEFT:
      if (cursorColumn > 0) {
        --cursorColumn;
        return 1;
      }
      break;
    case SCR_KEY_CURSOR_RIGHT:
      if (cursorColumn < (getBigEndian(pageDescriptions[pageNumber].width) - 1)) {
        ++cursorColumn;
        return 1;
      }
      break;
    default:
      break;
  }
  return 0;
}

static int
route_HelpScreen (int column, int row, int screen) {
  const HelpPageEntry *description = &pageDescriptions[pageNumber];
  if (row != -1) {
    if ((row < 0) || (row >= getBigEndian(description->height))) return 0;
    cursorRow = row;
  }
  if (column != -1) {
    if ((column < 0) || (column >= getBigEndian(description->width))) return 0;
    cursorColumn = column;
  }
  return 1;
}

void
initializeHelpScreen (HelpScreen *help) {
  initializeBaseScreen(&help->base);
  help->base.currentvt = currentvt_HelpScreen;
  help->base.describe = describe_HelpScreen;
  help->base.read = read_HelpScreen;
  help->base.insert = insert_HelpScreen;
  help->base.route = route_HelpScreen;
  help->open = open_HelpScreen;
  help->close = close_HelpScreen;
  help->setPageNumber = setPageNumber_HelpScreen;
  help->getPageNumber = getPageNumber_HelpScreen;
  help->getPageCount = getPageCount_HelpScreen;
  fileDescriptor = -1;
  pageDescriptions = NULL;
  pages = NULL;
  characters = NULL;
  pageNumber = 0;
  cursorRow = 0;
  cursorColumn = 0;
}
