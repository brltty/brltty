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

/*
 * papenmeier/simulate.c - Braille display test program
 * the file brl.c is included - HACK, but this allows easier testing
 * 
 *  This program simulates input to papenmeier screen 2d terminal
 *  by calling handle_keys - just for debug and test usage
 */

#include <stdio.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <signal.h>

/* HACK - include brl.c - with little adjustments */

#define  BRLDRIVER   NULL
#define  BRLHELP "nohelp" 

#define _SERIAL_C_
#define _SCR_H
#define READ_CONFIG

#include "brl.c"
#include "../misc.c"

#define BRLCOLS   80

braille_driver dummybraille;
braille_driver *braille = &dummybraille;

char* parameters[] = { "file", "y", "y", "y" };

brldim dummy_brldim;		// unused

static void finish(int sig);
static void error(char* txt);

static char **brailleParameters = NULL;

char* search_code(int code, int cmd)
{
  int i;
  for(i=0;symbols[i].sname; i++)
    if (symbols[i].token == code &&
	symbols[i].val == cmd)
	return symbols[i].sname;
  return "???";
}

void simulate(int code, int ispressed, int offsroute)
{
  int cmd, cmd2;
  char* txt;
  char* onoff = "";

  fprintf(stderr, "simulate %d %d %d\n", code, ispressed, offsroute);
  cmd = handle_key( code, ispressed, offsroute);
  cmd2 = cmd;
  if (cmd >= VAL_SWITCHON) {
    cmd2 = cmd % VAL_SWITCHON;
    if (cmd & VAL_SWITCHON)
      onoff = " 0N ";
    if (cmd & VAL_SWITCHOFF)
      onoff = " 0FF ";
  }
  txt =  search_code(KEYCODE, cmd2);
  if (txt[0] == '?')
    txt =  search_code(VPK, cmd2);
  fprintf(stderr, "return %d/0x%04x - %s%s\n", cmd, cmd, txt, onoff);
}


int main(int argc, char* argv[])
{
  int i;

  signal(SIGINT, finish);

  // open serial
  // initbrl(parameters, &dummy_brldim, argv[1]);

  LogOpen();
  SetLogLevel(999);
  SetStderrLevel(999);
  /* HACK - used with serial.c - 2d screen */
  the_terminal = &pm_terminals[3];
  addr_status = 0x0000;
  addr_display = addr_status + the_terminal->statcells;

  debug_keys = debug_reads = debug_writes = 1;

  if (! brl_fd)
    error("OOPS - cant open ");

  fprintf(stderr, "--- front keys ---\n");
  for(i=1; i <= 13; i++) {
    simulate(OFFS_FRONT + i, 1, 0);
    simulate(OFFS_FRONT + i, 0, 0);
  }

  fprintf(stderr, "--- status keys ---\n");
  for(i=1; i <= 21; i++) {
    simulate(OFFS_STAT + i, 1, 0);
    simulate(OFFS_STAT + i, 0, 0);
  }

  fprintf(stderr, "--- input mode ---\n");
  simulate(OFFS_STAT + 22, 1, 0);
  simulate(OFFS_STAT + 22, 0, 0);
  for(i=1; i <= 4; i++) {
    simulate(OFFS_FRONT + i, 1, 0);
    simulate(OFFS_FRONT + i, 0, 0);
  }
  for(i=10; i <= 13; i++) {
    simulate(OFFS_FRONT + i, 1, 0);
    simulate(OFFS_FRONT + i, 0, 0);
  }

  simulate(OFFS_STAT + 22, 1, 0);
  simulate(OFFS_STAT + 22, 0, 0);

  fprintf(stderr, "--- input spec key ---\n");
  simulate(OFFS_FRONT + 2, 1, 0);
  simulate(OFFS_STAT + 21, 1, 0);
  simulate(OFFS_STAT + 21, 0, 0);
  simulate(OFFS_FRONT + 2, 0, 0);

  finish(0);               /* we're done */
  return 0;
}

static void finish(int sig)
{
  closebrl (&dummy_brldim);
  exit(sig);
}

static void error(char* txt)
{
  puts(txt);
  finish(1);
}
