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

/* brltest.c - Test progrm for the Braille display library
 * $Id: brltest.c,v 1.3 1996/09/24 01:04:24 nn201 Exp $
 */

#define BRLTTY_C 1

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <getopt.h>
#include <string.h>

#include "brl.h"
#include "misc.h"
#include "message.h"
#include "scr.h"
#include "config.h"

int refreshInterval = REFRESH_INTERVAL;
static brldim brl;
static unsigned char statusCells[StatusCellCount];        /* status cell buffer */

void
message (const unsigned char *string, short flags) {
  int length = strlen(string);
  int limit = brl.x * brl.y;

  memset(statusCells, 0, sizeof(statusCells));
  braille->writeStatus(statusCells);

  memset(brl.disp, ' ', brl.x*brl.y);
  while (length) {
    int count = (length <= limit)? length: (limit - 1);
    int index;
    for (index=0; index<count; brl.disp[index++]=*string++);
    if (length -= count)
      brl.disp[(brl.x * brl.y) - 1] = '-';
    else
      while (index < limit) brl.disp[index++] = ' ';

    /* Do Braille translation using text table: */
    for (index=0; index<limit; index++)
      brl.disp[index] = textTable[brl.disp[index]];
    braille->writeWindow(&brl);
    if (length) {
      int timer = 0;
      while (braille->read(CMDS_MESSAGE) == EOF) {
        if (timer > 4000) break;
        delay(refreshInterval);
        timer += refreshInterval;
      }
    }
  }
}

int
main (int argc, char *argv[]) {
  int status;
  const char *driver = NULL;

  const char *const shortOptions = ":d:";
  const struct option longOptions[] = {
    {"device"   , required_argument, NULL, 'd'},
    {NULL       , 0                , NULL,  0 }
  };
  const char *device = NULL;

  opterr = 0;
  while (1) {
    int option = getopt_long(argc, argv, shortOptions, longOptions, NULL);
    if (option == -1) break;
    switch (option) {
      default:
        fprintf(stderr, "brltest: Unimplemented option: -%c\n", option);
        exit(2);
      case '?':
        fprintf(stderr, "brltest: Invalid option: -%c\n", optopt);
        exit(2);
      case ':':
        fprintf(stderr, "brltest: Missing operand: -%c\n", optopt);
        exit(2);
      case 'd':
        device = optarg;
        break;
    }
  }
  argv += optind; argc -= optind;
  if (argc) {
    driver = *argv++;
    --argc;
  }
  if (!device) device = BRLDEV;

  if ((braille = loadBrailleDriver(&driver))) {
    const char *const *parameterNames = braille->parameters;
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
        fprintf(stderr, "brltest: Insufficient memory.\n");
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
        LogPrint(LOG_ERR, "Missing braille driver parameter value: %s", assignment);
      } else if (delimiter == assignment) {
        LogPrint(LOG_ERR, "Missing braille driver parameter name: %s", assignment);
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
        if (!ok) LogPrint(LOG_ERR, "Invalid braille driver parameter: %s", assignment);
      }
      if (!ok) exit(2);
      --argc;
    }

    if (chdir(HOME_DIR) != -1) {
      reverseTable(textTable, untextTable);
      braille->identify();		/* start-up messages */
      braille->initialize(parameterSettings, &brl, device);
      if (brl.x > 0) {
        printf("Braille display successfully initialized: %d %s of %d %s\n",
               brl.y, ((brl.y == 1)? "row": "rows"),
               brl.x, ((brl.x == 1)? "column": "columns"));
        learnCommands(refreshInterval, 10000);
        braille->close(&brl);		/* finish with the display */
        status = 0;
      } else {
        LogPrint(LOG_ERR, "Can't initialize braille driver.");
        status = 5;
      }
    } else {
      LogPrint(LOG_ERR, "Can't change directory to '%s': %s", HOME_DIR, strerror(errno));
      status = 4;
    }
  } else {
    LogPrint(LOG_ERR, "Can't load braille driver.");
    status = 3;
  }
  return status;
}

/* dummy function to allow brl.o to link... */
void
setHelpPageNumber (short page) {
}
int
insertString (const unsigned char *string) {
  return 0;
}
