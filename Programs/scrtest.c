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

/* scrtest.c - Test program for the screen reading library
 * $Id: scrtest.c,v 1.3 1996/09/24 01:04:27 nn201 Exp $
 */

#include "prologue.h"

#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <ctype.h>

#include "program.h"
#include "options.h"
#include "misc.h"
#include "scr.h"

static char *opt_boxLeft;
static char *opt_boxWidth;
static char *opt_boxTop;
static char *opt_boxHeight;
static char *opt_screenDriver;
static char *opt_libraryDirectory;

BEGIN_OPTION_TABLE
  {"left", "column", 'l', 0, 0,
   &opt_boxLeft, NULL,
   "Left edge of region (from 0)."},

  {"columns", "count", 'c', 0, 0,
   &opt_boxWidth, NULL,
   "Width of region."},

  {"top", "row", 't', 0, 0,
   &opt_boxTop, NULL,
   "Top edge of region (from 0)."},

  {"rows", "count", 'r', 0, 0,
   &opt_boxHeight, NULL,
   "Height of region."},

  {"screen-driver", "driver", 'x', 0, 0,
   &opt_screenDriver, SCREEN_DRIVER,
   "Screen driver: one of {" SCREEN_DRIVER_CODES "}"},

  {"library-directory", "directory", 'L', 0, 0,
   &opt_libraryDirectory, LIBRARY_DIRECTORY,
   "Path to directory for loading drivers."},
END_OPTION_TABLE

static void
setRegion (int *offsetValue, const char *offsetOption, int offsetDefault, const char *offsetName,
           int *sizeValue, const char *sizeOption, int sizeLimit, const char *sizeName) {
  if (offsetOption) {
    {
      const int minimum = 0;
      const int maximum = sizeLimit - 1;
      if (!validateInteger(offsetValue, offsetOption, &minimum, &maximum)) {
        LogPrint(LOG_ERR, "invalid %s: %s", offsetName, offsetOption);
        exit(2);
      }
    }

    if (sizeOption) {
      const int minimum = 1;
      const int maximum = sizeLimit - *offsetValue;
      if (!validateInteger(sizeValue, sizeOption, &minimum, &maximum)) {
        LogPrint(LOG_ERR, "invalid %s: %s", sizeName, sizeOption);
        exit(2);
      }
      return;
    }
  } else if (sizeOption) {
    const int minimum = 1;
    const int maximum = sizeLimit;
    if (!validateInteger(sizeValue, sizeOption, &minimum, &maximum)) {
      LogPrint(LOG_ERR, "invalid %s: %s", sizeName, sizeOption);
      exit(2);
    }
    *offsetValue = (sizeLimit - *sizeValue) / 2;
    return;
  } else {
    int offsetLimit = (sizeLimit - 1) / 2;
    if ((*offsetValue = offsetDefault) > offsetLimit) *offsetValue = offsetLimit;
  }
  if ((*sizeValue = sizeLimit - (*offsetValue * 2)) < 1) *sizeValue = 1;
}

int
main (int argc, char *argv[]) {
  int status;
  void *driverObject;

  processOptions(optionTable, optionCount,
                 "scrtest", &argc, &argv,
                 NULL, NULL, NULL,
                 "[parameter=value ...]");

  {
    char **const paths[] = {
      &opt_libraryDirectory,
      NULL
    };
    fixInstallPaths(paths);
  }

  if ((screen = loadScreenDriver(opt_screenDriver, &driverObject, opt_libraryDirectory))) {
    const char *const *parameterNames = getScreenParameters(screen);
    char **parameterSettings;

    if (!parameterNames) {
      static const char *const noNames[] = {NULL};
      parameterNames = noNames;
    }

    {
      const char *const *name = parameterNames;
      unsigned int count;
      char **setting;
      while (*name) ++name;
      count = name - parameterNames;
      if (!(parameterSettings = malloc((count + 1) * sizeof(*parameterSettings)))) {
        LogPrint(LOG_ERR, "insufficient memory.");
        exit(9);
      }
      setting = parameterSettings;
      while (count--) *setting++ = "";
      *setting = NULL;
    }

    while (argc) {
      char *assignment = *argv++;
      int ok = 0;
      char *delimiter = strchr(assignment, '=');
      if (!delimiter) {
        LogPrint(LOG_ERR, "missing screen parameter value: %s", assignment);
      } else if (delimiter == assignment) {
        LogPrint(LOG_ERR, "missing screen parameter name: %s", assignment);
      } else {
        size_t nameLength = delimiter - assignment;
        const char *const *name = parameterNames;
        while (*name) {
          if (strncasecmp(assignment, *name, nameLength) == 0) {
            parameterSettings[name - parameterNames] = delimiter + 1;
            ok = 1;
            break;
          }
          ++name;
        }
        if (!ok) LogPrint(LOG_ERR, "invalid screen parameter: %s", assignment);
      }
      if (!ok) exit(2);
      --argc;
    }

    if (openScreenDriver(parameterSettings)) {
      ScreenDescription description;
      int left, top, width, height;
      unsigned char buffer[0X800];

      describeScreen(&description);
      printf("Screen: %dx%d\n", description.cols, description.rows);
      printf("Cursor: [%d,%d]\n", description.posx, description.posy);

      setRegion(&left, opt_boxLeft, 5, "starting column",
                &width, opt_boxWidth, description.cols, "region width");
      setRegion(&top, opt_boxTop, 5, "starting row",
                &height, opt_boxHeight, description.rows, "region height");
      printf("Region: %dx%d@[%d,%d]\n", width, height, left, top);
      if (readScreen(left, top, width, height, buffer, SCR_TEXT)) {
        int line;
        for (line=0; line<height; line++) {
          int column;
          for (column=0; column<width; column++) {
            unsigned char character = buffer[line * width + column];
            if (isprint(character))
              putchar(character);
            else
              putchar(' ');
          }
          putchar('\n');
        }
        status = 0;
      } else {
        LogPrint(LOG_ERR, "Can't read screen.");
        status = 4;
      }
    } else {
      LogPrint(LOG_ERR, "can't open screen.");
      status = 3;
    }

    closeScreenDriver();
  } else {
    LogPrint(LOG_ERR, "can't load screen driver.");
    status = 3;
  }
  return status;
}
