/*
 * BRLTTY - A background process providing access to the Linux console (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2003 by The BRLTTY Team. All rights reserved.
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "options.h"
#include "scr.h"

BEGIN_OPTION_TABLE
  {'l', "left", "column", NULL, 0,
   "Left edge of region (from 0)."},
  {'c', "columns", "count", NULL, 0,
   "Width of region."},
  {'t', "top", "row", NULL, 0,
   "Top edge of region (from 0)."},
  {'r', "rows", "count", NULL, 0,
   "Height of region."},
END_OPTION_TABLE

const char *opt_boxLeft = NULL;
const char *opt_boxWidth = NULL;
const char *opt_boxTop = NULL;
const char *opt_boxHeight = NULL;

static int
handleOption (const int option) {
  switch (option) {
    default:
      return 0;
    case 'c':
      opt_boxWidth = optarg;
      break;
    case 'l':
      opt_boxLeft = optarg;
      break;
    case 'r':
      opt_boxHeight = optarg;
      break;
    case 't':
      opt_boxTop = optarg;
      break;
  }
  return 1;
}

static void
setRegion (short *offsetValue, const char *offsetOption, short offsetDefault, const char *offsetName,
           short *sizeValue, const char *sizeOption, short sizeLimit, const char *sizeName) {
  if (offsetOption) {
    *offsetValue = integerArgument(offsetOption, 0, sizeLimit-1, offsetName);
    if (sizeOption) {
      *sizeValue = integerArgument(sizeOption, 1, sizeLimit-*offsetValue, sizeName);
      return;
    }
  } else if (sizeOption) {
    *sizeValue = integerArgument(sizeOption, 1, sizeLimit, sizeName);
    *offsetValue = (sizeLimit - *sizeValue) / 2;
    return;
  } else {
    short offsetLimit = (sizeLimit - 1) / 2;
    if ((*offsetValue = offsetDefault) > offsetLimit) *offsetValue = offsetLimit;
  }
  if ((*sizeValue = sizeLimit - (*offsetValue * 2)) < 1) *sizeValue = 1;
}

int
main (int argc, char *argv[]) {
  int status;

  const char *const *parameterNames;
  char **parameterSettings;

  processOptions(optionTable, optionCount, handleOption,
                 &argc, &argv, "[parameter=value ...]");

  initializeAllScreens();
  if (!(parameterNames = getScreenParameters())) {
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
      fprintf(stderr, "%s: Insufficient memory.\n", programName);
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
      fprintf(stderr, "%s: Missing screen parameter value: %s\n", programName, assignment);
    } else if (delimiter == assignment) {
      fprintf(stderr, "%s: Missing screen parameter name: %s\n", programName, assignment);
    } else {
      size_t nameLength = delimiter - assignment;
      const char *const *name = parameterNames;
      while (*name) {
        if (strlen(*name) >= nameLength) {
          if (strncasecmp(*name, assignment, nameLength) == 0) {
            parameterSettings[name - parameterNames] = delimiter + 1;
            ok = 1;
            break;
          }
        }
        ++name;
      }
      if (!ok) fprintf(stderr, "%s: Invalid screen parameter: %s\n", programName, assignment);
    }
    if (!ok) exit(2);
    --argc;
  }

  if (openLiveScreen(parameterSettings)) {
    ScreenDescription description;
    short left, top, width, height;
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
      fprintf(stderr, "%s: can't read screen.\n", programName);
      status = 4;
    }
    closeAllScreens();
  } else {
    fprintf(stderr, "%s: Can't initialize screen driver.\n", programName);
    status = 3;
  }
  return status;
}
