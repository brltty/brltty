/*
 * BRLTTY - A background process providing access to the Linux console (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2002 by The BRLTTY Team. All rights reserved.
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

/* scr_base.cc - screen drivers library
 *
 * Note: Although C++, this code requires no standard C++ library.
 * This is important as BRLTTY *must not* rely on too many
 * run-time shared libraries, nor be a huge executable.
 */

#define SCR_C 1

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>

#include "misc.h"
#include "route.h"
#include "scr.h"
#include "scr_base.h"

#ifdef HAVE_LIBGPM
#include <gpm.h>
extern int gpm_tried;

static int
gpmOpenConnection (void) {
  if (!gpm_flag) {
    Gpm_Connect connection;
    memset(&connection, 0, sizeof(connection));
    connection.eventMask = GPM_MOVE;
    connection.defaultMask = ~0;
    connection.minMod = 0;
    connection.maxMod = ~0;

    int wasTried = gpm_tried;
    gpm_tried = 0;
    gpm_zerobased = 1;

    if (Gpm_Open(&connection, -1) == -1) {
      if (!wasTried) LogPrint(LOG_WARNING, "GPM open error: %s", strerror(errno));
      return 0;
    }
    LogPrint(LOG_DEBUG, "GPM opened: fd=%d", gpm_fd);
  }
  return 1;
}

static void
gpmCloseConnection (void) {
  Gpm_Close();
  LogPrint(LOG_DEBUG, "GPM closed.");
}
#endif /* HAVE_LIBGPM */

int
Screen::insert (unsigned short key) {
  return 0;
}


int
Screen::route (int column, int row, int screen) {
  return 0;
}


int
Screen::point (int column, int row) {
  return 0;
}

int
Screen::pointer (int &column, int &row) {
  return 0;
}


int
Screen::selectvt (int vt) {
  return 0;
}


int
Screen::switchvt (int vt) {
  return 0;
}


int
Screen::execute (int cmd) {
  return 0;
}


int
RealScreen::route (int column, int row, int screen) {
  return csrjmp(column, row, screen);
}


int
RealScreen::point (int column, int row) {
#ifdef HAVE_LIBGPM
  if (gpmOpenConnection()) {
    Gpm_Event event;
    event.x = column;
    event.y = row;
    if (GPM_DRAWPOINTER(&event) != -1) return 1;
    gpmCloseConnection();
  }
#endif /* HAVE_LIBGPM */
  return 0;
}

int
RealScreen::pointer (int &column, int &row) {
#ifdef HAVE_LIBGPM
  if (gpmOpenConnection()) {
    if (gpm_fd >= 0) {
      int ok = 0;
      int error = 0;
      while (1) {
        fd_set mask;
        FD_ZERO(&mask);
        FD_SET(gpm_fd, &mask);

        struct timeval timeout;
        memset(&timeout, 0, sizeof(timeout));

        int count = select(gpm_fd+1, &mask, NULL, NULL, &timeout);
        if (count == 0) break;
        error = 1;
        if (count < 0) break;
        if (!FD_ISSET(gpm_fd, &mask)) break;

        Gpm_Event event;
        if (Gpm_GetEvent(&event) != 1) break;
        error = 0;

        column = event.x;
        row = event.y;
        ok = 1;
      }

      if (error) gpmCloseConnection();
      return ok;
    }
  }
#endif /* HAVE_LIBGPM */
  return 0;
}


FrozenScreen::FrozenScreen ()
{
  text = NULL;
  attributes = NULL;
}


int
FrozenScreen::open (Screen *source)
{
  source->describe(description);
  if ((text = new unsigned char[description.rows * description.cols])) {
    if ((attributes = new unsigned char[description.rows * description.cols])) {
      if (source->read((ScreenBox){0, 0, description.cols, description.rows}, text, SCR_TEXT)) {
        if (source->read((ScreenBox){0, 0, description.cols, description.rows}, attributes, SCR_ATTRIB)) {
          return 1;
        }
      }
      delete attributes;
      attributes = NULL;
    } else {
      LogError("Attributes buffer allocation");
    }
    delete text;
    text = NULL;
  } else {
    LogError("Text buffer allocation");
  }
  return 0;
}


void
FrozenScreen::close (void)
{
  if (text) {
    delete text;
    text = NULL;
  }
  if (attributes) {
    delete attributes;
    attributes = NULL;
  }
}


void
FrozenScreen::describe (ScreenDescription &desc)
{
  desc = description;
}


unsigned char *
FrozenScreen::read (ScreenBox box, unsigned char *buffer, ScreenMode mode)
{
  if ((box.left >= 0) && (box.width > 0) && ((box.left + box.width) <= description.cols) &&
      (box.top >= 0) && (box.height > 0) && ((box.top + box.height) <= description.rows)) {
    unsigned char *screen = (mode == SCR_TEXT)? text: attributes;
    for (int row=0; row<box.height; row++)
      memcpy(buffer + (row * box.width),
             screen + ((box.top + row) * description.cols) + box.left,
             box.width);
    return buffer;
  }
  return NULL;
}


HelpScreen::HelpScreen ()
{
  fileDescriptor = -1;
  pageDescriptions = NULL;
  pages = NULL;
  characters = NULL;
  pageNumber = 0;
  cursorRow = 0;
  cursorColumn = 0;
}


int
HelpScreen::loadPages (const char *file)
{
  int bytesRead;
  unsigned long characterCount = 0; /* total length of formatted help screens */

  if ((fileDescriptor = ::open(file, O_RDONLY)) == -1) {
    LogError("Help file open");
    goto failure;
  }

  if ((bytesRead = ::read(fileDescriptor, &fileHeader, sizeof(fileHeader))) == -1) {
    LogError("Help file read");
    goto failure;
  }
  if (bytesRead != sizeof(fileHeader)) {
    LogPrint(LOG_ERR, "Help file corrupt");
    goto failure;
  }
  if (fileHeader.pages < 1) {
    LogPrint(LOG_ERR, "Help file corrupt");
    goto failure;
  }

  if (!(pageDescriptions = new HelpPageEntry[fileHeader.pages])) {
    LogError("Help page descriptions allocation");
    goto failure;
  }
  for (int page=0; page<fileHeader.pages; page++) {
    HelpPageEntry *description = &pageDescriptions[page];
    if ((bytesRead = ::read(fileDescriptor, description, sizeof(*description))) == -1) {
      LogError("Help file read");
      goto failure;
    }
    if (bytesRead != sizeof(*description)) {
      LogPrint(LOG_ERR, "Help file corrupt");
      goto failure;
    }
    characterCount += description->rows * description->columns;
  }

  if (!(pages = new unsigned char *[fileHeader.pages])) {
    LogError("Help page addresses allocation");
    goto failure;
  }
  if (!(characters = new unsigned char[characterCount])) {
    LogError("Help page buffer allocation");
    goto failure;
  }
  pages[0] = characters;
  for (int page=0; page<(fileHeader.pages-1); page++) {
    const HelpPageEntry *description = &pageDescriptions[page];
    pages[page+1] = pages[page] + (description->rows * description->columns);
  }

  for (int page=0; page<fileHeader.pages; page++) {
    const HelpPageEntry *description = &pageDescriptions[page];
    unsigned char *buffer = pages[page];
    for (int row=0; row<description->rows; row++) {
      unsigned char lineLength;
      if ((bytesRead = ::read(fileDescriptor, &lineLength, sizeof(lineLength))) == -1) {
        LogError("Help line length read");
        goto failure;
      }
      if (bytesRead != sizeof(lineLength)) {
        LogPrint(LOG_ERR, "Help file corrupt.");
        goto failure;
      }
      unsigned char *line = &buffer[row * description->columns];
      if (lineLength) {
        if ((bytesRead = ::read(fileDescriptor, line, lineLength)) == -1) {
          LogError("Help line read");
          goto failure;
        }
        if (bytesRead != lineLength) {
          LogPrint(LOG_ERR, "Help file corrupt");
          goto failure;
        }
      }
      memset(&line[lineLength], ' ', description->columns-lineLength);
    }
  }

  ::close(fileDescriptor);
  fileDescriptor = -1;
  return 1;

failure:
  close();
  return 0;
}


int
HelpScreen::open (const char *file)
{
  if (!pages)
    if (!loadPages(file))
      return 0;
  if ((pageNumber < 0) || (pageNumber >= fileHeader.pages)) pageNumber = 0;
  return 1;
}


void
HelpScreen::close (void)
{
  if (characters) {
    delete characters;
    characters = NULL;
  }
  if (pages) {
    delete pages;
    pages = NULL;
  }
  if (pageDescriptions) {
    delete pageDescriptions;
    pageDescriptions = NULL;
  }
  if (fileDescriptor != -1) {
    ::close(fileDescriptor);
    fileDescriptor = -1;
  }
}


void
HelpScreen::setPageNumber (short page)
{
  pageNumber = page;
}


short
HelpScreen::getPageNumber (void)
{
  return pageNumber;
}


short
HelpScreen::getPageCount (void)
{
  return (fileDescriptor != -1)? fileHeader.pages: 0;
}


void
HelpScreen::describe (ScreenDescription &desc)
{
  const HelpPageEntry *description = &pageDescriptions[pageNumber];
  desc.posx = cursorColumn;
  desc.posy = cursorRow;
  desc.cols = description->columns;
  desc.rows = description->rows;
  desc.no = 0;	/* 0 is reserved for help screen */
}


unsigned char *
HelpScreen::read (ScreenBox box, unsigned char *buffer, ScreenMode mode)
{
  const HelpPageEntry *description = &pageDescriptions[pageNumber];
  if ((box.left >= 0) && (box.width > 0) && ((box.left + box.width) <= description->columns) &&
      (box.top >= 0) && (box.height > 0) && ((box.top + box.height) <= description->rows)) {
    if (mode == SCR_TEXT) {
      for (int row=0; row<box.height; row++) {
        memcpy(buffer + (row * box.width),
               pages[pageNumber] + ((box.top + row) * description->columns) + box.left,
               box.width);
      }
    } else {
      memset(buffer, 0X07, box.width*box.height);
    }
    return buffer;
  }
  return NULL;
}


int
HelpScreen::insert (unsigned short key)
{
  switch (key) {
    case KEY_PAGE_UP:
      if (pageNumber > 0) {
        --pageNumber;
        cursorRow = cursorColumn = 0;
        return 1;
      }
      break;
    case KEY_PAGE_DOWN:
      if (pageNumber < (fileHeader.pages - 1)) {
        ++pageNumber;
        cursorRow = cursorColumn = 0;
        return 1;
      }
      break;
    case KEY_CURSOR_UP:
      if (cursorRow > 0) {
        --cursorRow;
        return 1;
      }
      break;
    case KEY_CURSOR_DOWN:
      if (cursorRow < (pageDescriptions[pageNumber].rows - 1)) {
        ++cursorRow;
        return 1;
      }
      break;
    case KEY_CURSOR_LEFT:
      if (cursorColumn > 0) {
        --cursorColumn;
        return 1;
      }
      break;
    case KEY_CURSOR_RIGHT:
      if (cursorColumn < (pageDescriptions[pageNumber].columns - 1)) {
        ++cursorColumn;
        return 1;
      }
      break;
    default:
      break;
  }
  return 0;
}


int
HelpScreen::route (int column, int row, int screen) {
  const HelpPageEntry *description = &pageDescriptions[pageNumber];
  if (row != -1) {
    if ((row < 0) || (row >= description->rows)) return 0;
    cursorRow = row;
  }
  if (column != -1) {
    if ((column < 0) || (column >= description->columns)) return 0;
    cursorColumn = column;
  }
  return 1;
}
