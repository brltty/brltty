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

/* apitest provides a small test utility for brltty's API */
 
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "api.h"
#include "brldefs.h"
#include "cmd.h"

void getDisplaySize(void)
{
  unsigned int x, y;
  fprintf(stderr,"Getting display size: ");
  if (brlapi_getDisplaySize(&x, &y)<0) {
    brlapi_perror("failed");
    exit(1);
  }
  fprintf(stderr, "%dX%d\n", x, y);
}

void getDriverId(void)
{
  char id[3];
  fprintf(stderr, "Getting driver id: ");
  if (brlapi_getDriverId(id, sizeof(id))<0) {
    brlapi_perror("failed");
    exit(1);
  }
  fprintf(stderr, "%s\n", id);
}

void getDriverName(void)
{
  char name[30];
  fprintf(stderr, "Getting driver name: ");
  if (brlapi_getDriverName(name, sizeof(name))<0) {
    brlapi_perror("failed");
    exit(1);
  }
  fprintf(stderr, "%s\n", name);
}

void testLearnMode(void)
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
  int i, fd;
  fprintf(stderr, "Connecting to BrlAPI... ");
  if ((fd=brlapi_initializeConnection(NULL, NULL))<0) {
    brlapi_perror("failed");
    exit(1);
  }
  fprintf(stderr, "done\n");
  for (i=1; i<argc; i++) {
    if (!strcmp(argv[i],"-ds")) {
      getDisplaySize();
      continue;
    } else if (!strcmp(argv[i], "-di")) {
      getDriverId();
      continue;
    } else if (!strcmp(argv[i],"-dn")) {
      getDriverName();
      continue;
    } else if (!strcmp(argv[i], "-lm")) {
      testLearnMode();
      continue;
    } else fprintf(stderr, "Don't know what to do with argument %d (%s), skipping\n",i,argv[i]);
  }
  brlapi_closeConnection();
  fprintf(stderr, "Disconnected\n"); 
  return 0;
}
