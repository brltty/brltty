/*
 * BRLTTY - A background process providing access to the Linux console (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2001 by The BRLTTY Team. All rights reserved.
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

extern braille_driver *braille;
int refreshInterval = REFRESH_INTERVAL;
static brldim brl;

void
message (const unsigned char *string, short flags) {
  int length = strlen(string);
  int limit = brl.x * brl.y;

  memset(statcells, 0, sizeof(statcells));
  braille->writeStatus(statcells);

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
      brl.disp[index] = texttrans[brl.disp[index]];
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

  if (loadBrailleDriver(&driver)) {
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
      reverseTable(texttrans, untexttrans);
      braille->identify();		/* start-up messages */
      braille->initialize(parameterSettings, &brl, device);
      if (brl.x > 0) {
        printf("Braille display successfully initialized: %d %s of %d %s\n",
               brl.y, ((brl.y == 1)? "row": "rows"),
               brl.x, ((brl.x == 1)? "column": "columns"));
        message("This is BRLTTY!", 0);

        while (1) {
          static int timer = 0;
          int key = braille->read(CMDS_SCREEN);
          const char *description = NULL;
          int argument = -1;
          int adjustment = 1;
          #define SIMPLE(c, d) case c: description = d; break
          #define TOGGLE(c, d) \
            SIMPLE(c|VAL_SWITCHON, "set " d " on"); \
            SIMPLE(c|VAL_SWITCHOFF, "set " d " off"); \
            SIMPLE(c, "toggle " d)
          switch (key) {
            case EOF:
              if (timer > 10000) goto done;
              delay(refreshInterval);
              timer += refreshInterval;
              continue;
            SIMPLE(CMD_NOOP, "no operation");
            SIMPLE(CMD_LNUP, "up one line");
            SIMPLE(CMD_LNDN, "down one line");
            SIMPLE(CMD_WINUP, "up a few lines");
            SIMPLE(CMD_WINDN, "down a few lines");
            SIMPLE(CMD_PRDIFLN, "up to different line");
            SIMPLE(CMD_NXDIFLN, "down to different line");
            SIMPLE(CMD_ATTRUP, "up to different attributes");
            SIMPLE(CMD_ATTRDN, "down to different attributes");
            SIMPLE(CMD_PRBLNKLN, "up to previous paragraph");
            SIMPLE(CMD_NXBLNKLN, "down to next paragraph");
            SIMPLE(CMD_PRSEARCH, "search up");
            SIMPLE(CMD_NXSEARCH, "search down");
            SIMPLE(CMD_TOP, "up to top");
            SIMPLE(CMD_BOT, "down to bottom");
            SIMPLE(CMD_TOP_LEFT, "up to top-left");
            SIMPLE(CMD_BOT_LEFT, "down to bottom-left");
            SIMPLE(CMD_CHRLT, "left one character");
            SIMPLE(CMD_CHRRT, "right one character");
            SIMPLE(CMD_HWINLT, "left half window");
            SIMPLE(CMD_HWINRT, "right half window");
            SIMPLE(CMD_FWINLT, "left one window");
            SIMPLE(CMD_FWINRT, "right one window");
            SIMPLE(CMD_FWINLTSKIP, "left to non-blank window");
            SIMPLE(CMD_FWINRTSKIP, "right to non-blank window");
            SIMPLE(CMD_LNBEG, "left to start");
            SIMPLE(CMD_LNEND, "right to end");
            SIMPLE(CMD_CSRJMP, "cursor to window");
            SIMPLE(CMD_CUT_BEG, "start cut block");
            SIMPLE(CMD_CUT_END, "end cut block");
            SIMPLE(CMD_HOME, "go to cursor");
            SIMPLE(CMD_BACK, "go back");
            SIMPLE(CMD_CSRJMP_VERT, "cursor to line");
            SIMPLE(CMD_PASTE, "paste");
            TOGGLE(CMD_FREEZE, "screen freeze");
            TOGGLE(CMD_DISPMD, "display mode");
            TOGGLE(CMD_SIXDOTS, "six-dot braille");
            TOGGLE(CMD_SLIDEWIN, "sliding window");
            TOGGLE(CMD_SKPIDLNS, "skip identical lines");
            TOGGLE(CMD_SKPBLNKWINS, "skip blank windows");
            TOGGLE(CMD_CSRVIS, "show cursor");
            TOGGLE(CMD_CSRHIDE, "hide cursor");
            TOGGLE(CMD_CSRTRK, "cursor tracking");
            TOGGLE(CMD_CSRSIZE, "cursor shape");
            TOGGLE(CMD_CSRBLINK, "blinking cursor");
            TOGGLE(CMD_ATTRVIS, "show attributes");
            TOGGLE(CMD_ATTRBLINK, "blinking attributes");
            TOGGLE(CMD_CAPBLINK, "blinking capitals");
            TOGGLE(CMD_SND, "sound");
            SIMPLE(CMD_HELP, "show help page");
            SIMPLE(CMD_INFO, "show current settings");
            SIMPLE(CMD_PREFMENU, "show preferences menu");
            SIMPLE(CMD_PREFSAVE, "save preferences");
            SIMPLE(CMD_PREFLOAD, "load preferences");
            SIMPLE(CMD_SAY, "say current line");
            SIMPLE(CMD_SAYALL, "say to bottom");
            SIMPLE(CMD_MUTE, "mute speech");
            SIMPLE(CMD_SPKHOME, "go to speech cursor");
            SIMPLE(CMD_SWITCHVT_PREV, "select previous virtual terminal");
            SIMPLE(CMD_SWITCHVT_NEXT, "select next virtual terminal");
            SIMPLE(CMD_RESTARTBRL, "restart braille driver");
            SIMPLE(CMD_RESTARTSPEECH, "restart speech driver");
            SIMPLE(CMD_MENU_FIRST_ITEM, "first menu item");
            SIMPLE(CMD_MENU_LAST_ITEM, "last menu item");
            SIMPLE(CMD_MENU_PREV_ITEM, "previous menu item");
            SIMPLE(CMD_MENU_NEXT_ITEM, "next menu item");
            SIMPLE(CMD_MENU_PREV_SETTING, "previous menu setting");
            SIMPLE(CMD_MENU_NEXT_SETTING, "next menu setting");
            TOGGLE(CMD_INPUTMODE, "input mode");
            default:
              argument = key & VAL_ARG_MASK;
              switch (key & VAL_BLK_MASK) {
                SIMPLE(CR_ROUTE, "cursor to column");
                SIMPLE(CR_CUTBEGIN, "start new cut buffer from column");
                SIMPLE(CR_CUTAPPEND, "append to cut buffer from column");
                SIMPLE(CR_CUTRECT, "rectangular cut to column");
                SIMPLE(CR_CUTLINE, "linear cut to column");
                SIMPLE(CR_SWITCHVT, "select virtual terminal");
                SIMPLE(CR_NXINDENT, "down to line indented no more than column");
                SIMPLE(CR_PRINDENT, "up to line indented no more than column");
                SIMPLE(CR_DESCCHAR, "describe character at column");
                SIMPLE(CR_SETLEFT, "set left of window to column");
                SIMPLE(CR_SETMARK, "associate current window position with routing key");
                SIMPLE(CR_GOTOMARK, "return to window position associated with routing key");
                case VAL_PASSKEY:
                  if (argument < VPK_FUNCTION) {
                    switch (argument) {
                      SIMPLE(VPK_RETURN, "return key");
                      SIMPLE(VPK_TAB, "tab key");
                      SIMPLE(VPK_BACKSPACE, "backspace key");
                      SIMPLE(VPK_ESCAPE, "escape key");
                      SIMPLE(VPK_CURSOR_LEFT, "cursor left key");
                      SIMPLE(VPK_CURSOR_RIGHT, "cursor right key");
                      SIMPLE(VPK_CURSOR_UP, "cursor up key");
                      SIMPLE(VPK_CURSOR_DOWN, "cursor down key");
                      SIMPLE(VPK_PAGE_UP, "page up key");
                      SIMPLE(VPK_PAGE_DOWN, "page down key");
                      SIMPLE(VPK_HOME, "home key");
                      SIMPLE(VPK_END, "end key");
                      SIMPLE(VPK_INSERT, "insert key");
                      SIMPLE(VPK_DELETE, "delete key");
                      default:
                        break;
                    }
                    argument = -1;
                  } else {
                    description = "function key";
                    argument -= VPK_FUNCTION;
                  }
                default:
                  break;
              }
              break;
          }
          if (!description) {
            static char buffer[0X80];
            snprintf(buffer, sizeof(buffer), "unknown: %06X", key);
            description = buffer;
          } else if (argument >= 0) {
            static char buffer[0X80];
            snprintf(buffer, sizeof(buffer), "%s %d", description, argument+adjustment);
            description = buffer;
          }
          message(description, 0);
          timer = 0;
        }
      done:
        message("done", 0);

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
