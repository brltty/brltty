/*
 * BRLTTY - A background process providing access to the Linux console (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2004 by The BRLTTY Team. All rights reserved.
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

/* apitest provides a small test utility for BRLTTY's API */
 
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "options.h"
#include "api.h"
#include "brldefs.h"
#include "cmd.h"

static brlapi_settings_t settings;

BEGIN_OPTION_TABLE
  {'i', "identifier", NULL, NULL, 0,
   "Show the driver's identifier."},
  {'k', "brlapi-key", "file", NULL, 0,
   "Path to file containing BrlAPI's authentication key."},
  {'l', "learn", NULL, NULL, 0,
   "Enter interactive command learn mode."},
  {'n', "name", NULL, NULL, 0,
   "Show the driver's name."},
  {'s', "brlapi-server", "[host][:port]", NULL, 0,
   "Host name (or address) and port of the BrlAPI server."},
  {'w', "window", NULL, NULL, 0,
   "Show the braille window's size."},
END_OPTION_TABLE

static int opt_learnMode = 0;
static int opt_showIdentifier = 0;
static int opt_showName = 0;
static int opt_showSize = 0;

static int
handleOption (const int option) {
  switch (option) {
    default:
      return 0;
    case 'i':
      opt_showIdentifier = 1;
      break;
    case 'k':
      settings.authKey = optarg;
      break;
    case 'l':
      opt_learnMode = 1;
      break;
    case 'n':
      opt_showName = 1;
      break;
    case 's':
      settings.hostName = optarg;
      break;
    case 'w':
      opt_showSize = 1;
      break;
  }
  return 1;
}

void showDisplaySize(void)
{
  unsigned int x, y;
  fprintf(stderr,"Getting display size: ");
  if (brlapi_getDisplaySize(&x, &y)<0) {
    brlapi_perror("failed");
    exit(1);
  }
  fprintf(stderr, "%dX%d\n", x, y);
}

void showDriverIdentifier(void)
{
  char id[3];
  fprintf(stderr, "Getting driver id: ");
  if (brlapi_getDriverId(id, sizeof(id))<0) {
    brlapi_perror("failed");
    exit(1);
  }
  fprintf(stderr, "%s\n", id);
}

void showDriverName(void)
{
  char name[30];
  fprintf(stderr, "Getting driver name: ");
  if (brlapi_getDriverName(name, sizeof(name))<0) {
    brlapi_perror("failed");
    exit(1);
  }
  fprintf(stderr, "%s\n", name);
}

void enterLearnMode(void)
{
  int res;
  brl_keycode_t cmd;
  char buf[0X100];

  if (brlapi_getTty(0, BRLCOMMANDS)<0) {
    brlapi_perror("getTty");
    exit(1);
  }

  if (brlapi_writeBrl(0, "command learn mode")<0) {
    brlapi_perror("writeBrl");
    exit(1);
  }

  while ((res = brlapi_readKey(1, &cmd)) != -1) {
    describeCommand(cmd, buf, sizeof(buf));
    brlapi_writeBrl(0, buf);
    if (cmd==CMD_LEARN) break;
  }
}

int main(int argc, char *argv[])
{
  int status = 0;
  int fd;
  settings.hostName = NULL; settings.authKey = NULL;

  processOptions(optionTable, optionCount, handleOption,
                 &argc, &argv, "");

  fprintf(stderr, "Connecting to BrlAPI... ");
  if ((fd=brlapi_initializeConnection(&settings, &settings)) >= 0) {
    fprintf(stderr, "done\n");
    fprintf(stderr,"Connected to %s using key file %s\n", settings.hostName, settings.authKey);

    if (opt_showIdentifier) {
      showDriverIdentifier();
    }

    if (opt_showName) {
      showDriverName();
    }

    if (opt_showSize) {
      showDisplaySize();
    }

    if (opt_learnMode) {
      enterLearnMode();
    }

    brlapi_closeConnection();
    fprintf(stderr, "Disconnected\n"); 
  } else {
    fprintf(stderr, "failed to connect to %s using key %s",settings.hostName, settings.authKey);
    brlapi_perror("");
    status = 1;
  }
  return status;
}
