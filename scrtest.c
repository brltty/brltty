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

/* scrtest.c - Test program for the screen reading library
 * $Id: scrtest.c,v 1.3 1996/09/24 01:04:27 nn201 Exp $
 */

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <getopt.h>
#include <string.h>
#include <ctype.h>

#include "scr.h"

static short
integerOption (const char *option, short minimum, short maximum, const char *name) {
  char *end;
  long int value = strtol(option, &end, 0);
  if ((end == option) || *end) {
    fprintf(stderr, "scrtest: Invalid %s: %s\n", name, option);
  } else if (value < minimum) {
    fprintf(stderr, "scrtest: %s is less than %d: %ld\n", name, minimum, value);
  } else if (value > maximum) {
    fprintf(stderr, "scrtest: %s is greater than %d: %ld\n", name, maximum, value);
  } else {
    return value;
  }
  exit(2);
}

static void
setRegion (short *offsetField, const char *offsetOption, short offsetDefault, const char *offsetName,
           short *sizeField, const char *sizeOption, short sizeLimit, const char *sizeName) {
  if (offsetOption) {
    *offsetField = integerOption(offsetOption, 0, sizeLimit-1, offsetName);
    if (sizeOption) {
      *sizeField = integerOption(sizeOption, 1, sizeLimit-*offsetField, sizeName);
      return;
    }
  } else if (sizeOption) {
    *sizeField = integerOption(sizeOption, 1, sizeLimit, sizeName);
    *offsetField = (sizeLimit - *sizeField) / 2;
    return;
  } else {
    short offsetLimit = (sizeLimit - 1) / 2;
    if ((*offsetField = offsetDefault) > offsetLimit) *offsetField = offsetLimit;
  }
  if ((*sizeField = sizeLimit - (*offsetField * 2)) < 1) *sizeField = 1;
}

int
main (int argc, char *argv[]) {
  int status;

  const char *const shortOptions = ":c:h:p:r:w:";
  const struct option longOptions[] = {
    {"column"   , required_argument, NULL, 'c'},
    {"height"   , required_argument, NULL, 'h'},
    {"parameter", required_argument, NULL, 'p'},
    {"row"      , required_argument, NULL, 'r'},
    {"width"    , required_argument, NULL, 'w'},
    {NULL       , 0                , NULL,  0 }
  };
  const char *boxTop = NULL;
  const char *boxHeight = NULL;
  const char *boxLeft = NULL;
  const char *boxWidth = NULL;

  const char *const *parameterNames = getScreenParameters();
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
      fprintf(stderr, "scrtest: Insufficient memory.\n");
      exit(9);
    }
    setting = parameterSettings;
    while (count--) *setting++ = "";
    *setting = NULL;
  }

  opterr = 0;
  while (1) {
    int option = getopt_long(argc, argv, shortOptions, longOptions, NULL);
    if (option == -1) break;
    switch (option) {
      default:
        fprintf(stderr, "scrtest: Unimplemented option: -%c\n", option);
        exit(2);
      case '?':
        fprintf(stderr, "scrtest: Invalid option: -%c\n", optopt);
        exit(2);
      case ':':
        fprintf(stderr, "scrtest: Missing operand: -%c\n", optopt);
        exit(2);
      case 'c':
        boxLeft = optarg;
        break;
      case 'h':
        boxHeight = optarg;
        break;
      case 'p': {
        char *assignment = optarg;
        int ok = 0;
        char *delimiter = strchr(assignment, '=');
        if (!delimiter) {
          fprintf(stderr, "scrtest: Missing screen parameter value: %s\n", assignment);
        } else if (delimiter == assignment) {
          fprintf(stderr, "scrtest: Missing screen parameter name: %s\n", assignment);
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
          if (!ok) fprintf(stderr, "scrtest: Invalid screen parameter: %s\n", assignment);
        }
        if (!ok) exit(2);
        break;
      }
      case 'r':
        boxTop = optarg;
        break;
      case 'w':
        boxWidth = optarg;
        break;
    }
  }
  argv += optind; argc -= optind;
  if (argc) {
    fprintf(stderr, "scrtest: Too many parameters.\n");
    exit(2);
  }

  if (initializeLiveScreen(parameterSettings)) {
    ScreenDescription description;
    ScreenBox box;
    unsigned char buffer[0X800];

    describeScreen(&description);
    printf("Screen: %dx%d\n", description.cols, description.rows);
    printf("Cursor: [%d,%d]\n", description.posx, description.posy);

    setRegion(&box.left, boxLeft, 5, "starting column",
              &box.width, boxWidth, description.cols, "region width");
    setRegion(&box.top, boxTop, 5, "starting row",
              &box.height, boxHeight, description.rows, "region height");
    printf("Region: %dx%d@[%d,%d]\n", box.width, box.height, box.left, box.top);
    if (readScreen(box, buffer, SCR_TEXT)) {
      int line;
      for (line=0; line<box.height; line++) {
        int column;
        for (column=0; column<box.width; column++) {
          unsigned char character = buffer[line * box.width + column];
          if (isprint(character))
            putchar(character);
          else
            putchar(' ');
        }
        putchar('\n');
      }
      status = 0;
    } else {
      fprintf(stderr, "scrtest: can't read screen.\n");
      status = 4;
    }
    closeAllScreens();
  } else {
    fprintf(stderr, "scrtest: Can't initialize screen driver.\n");
    status = 3;
  }
  return status;
}
