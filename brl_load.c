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

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <dlfcn.h>

#include "misc.h"
#include "message.h"
#include "brl.h"

#define BRLSYMBOL noBraille
#define BRLNAME "NoBraille"
#define BRLDRIVER "no"
#define BRLHELP "/dev/null"
#define PREFSTYLE ST_None
#include "brl_driver.h"
static void brl_identify (void) {
  LogPrint(LOG_NOTICE, "No braille support.");
}
static void brl_initialize (char **parameters, brldim *brl, const char *device) { }
static void brl_close (brldim *brl) { }
static void brl_writeWindow (brldim *brl) { }
static void brl_writeStatus (const unsigned char *status) { }
static int brl_read (DriverCommandContext cmds) { return EOF; }

static void *libraryHandle = NULL;	/* handle to driver */
static const char *symbolName = "brl_driver";
const BrailleDriver *braille = &noBraille;

/*
 * Output braille translation tables.
 * The files *.auto.h (the default tables) are generated at compile-time.
 */
unsigned char textTable[0X100] = {
  #include "text.auto.h"
};
unsigned char untextTable[0X100];

unsigned char attributesTable[0X100] = {
  #include "attrib.auto.h"
};

void *contractionTable = NULL;

const CommandEntry commandTable[] = {
  #include "cmds.auto.h"
  {EOF, NULL, NULL}
};

/* load driver from library */
/* return true (nonzero) on success */
const BrailleDriver *
loadBrailleDriver (const char **driver) {
  if (*driver != NULL)
    if (strcmp(*driver, noBraille.identifier) == 0) {
      *driver = NULL;
      return &noBraille;
    }

  #ifdef BRL_BUILTIN
    {
      extern BrailleDriver brl_driver;
      if (*driver != NULL)
        if (strcmp(*driver, brl_driver.identifier) == 0)
          *driver = NULL;
      if (*driver == NULL) {
        return &brl_driver;
      }
    }
  #else
    if (*driver == NULL) {
      return &noBraille;
    }
  #endif

  {
    const char *libraryName = *driver;

    /* allow shortcuts */
    if (strlen(libraryName) == 2) {
      static char buffer[] = "libbrlttyb??.so.1"; /* two ? for shortcut */
      memcpy(strchr(buffer, '?'), libraryName, 2);
      libraryName = buffer;
    }

    if ((libraryHandle = dlopen(libraryName, RTLD_NOW|RTLD_GLOBAL))) {
      const char *error;
      const void *address = dlsym(libraryHandle, symbolName);
      if (!(error = dlerror())) {
        return address;
      } else {
        LogPrint(LOG_ERR, "%s", error);
        LogPrint(LOG_ERR, "Braille driver symbol not found: %s", symbolName);
      }
      dlclose(libraryHandle);
      libraryHandle = NULL;
    } else {
      LogPrint(LOG_ERR, "%s", dlerror());
      LogPrint(LOG_ERR, "Cannot open braille driver library: %s", libraryName);
    }
  }

  return NULL;
}


int
listBrailleDrivers (void) {
  char buf[64];
  static const char *list_file = LIB_PATH "/brltty-brl.lst";
  int cnt, fd = open(list_file, O_RDONLY);
  if (fd < 0) {
    fprintf(stderr, "Error: can't access braille driver list file\n");
    perror(list_file);
    return 0;
  }
  fprintf(stderr, "Available Braille Drivers:\n\n");
  fprintf(stderr, "XX\tDescription\n");
  fprintf(stderr, "--\t-----------\n");
  while ((cnt = read(fd, buf, sizeof(buf))))
    fwrite(buf, cnt, 1, stderr);
  close(fd);
  return 1;
}

void
learnCommands (int poll, int timeout) {
  int timer = 0;
  message("command learn mode", MSG_NODELAY);
  while (1) {
    int key = braille->read(CMDS_SCREEN);
    if (key == EOF) {
      if (timer > timeout) break;
      delay(poll);
      timer += poll;
    } else {
      int blk = key & VAL_BLK_MASK;
      int arg = key & VAL_ARG_MASK;
      int cmd = blk | arg;
      unsigned char buffer[0X100];
      const CommandEntry *candidate = NULL;
      const CommandEntry *last = NULL;
      const CommandEntry *command = commandTable;
      if (cmd == CMD_LEARN) return;
      while (command->name) {
        if ((command->code & VAL_BLK_MASK) == blk) {
          if (!last || (last->code < command->code)) last = command;
          if (!candidate || (candidate->code < cmd)) candidate = command;
        }
        ++command;
      }
      if (candidate)
        if (candidate->code != cmd)
          if ((blk == 0) || (candidate->code < last->code))
            candidate = NULL;
      if (!candidate) {
        snprintf(buffer, sizeof(buffer), "unknown: %06X", key);
      } else if ((candidate == last) && (blk != 0)) {
        unsigned int number;
        switch (blk) {
          default:
            number = cmd - candidate->code + 1;
            break;
          case VAL_PASSCHAR:
            number = cmd - candidate->code;
            break;
          case VAL_PASSDOTS: {
            unsigned char dots[] = {B1, B2, B3, B4, B5, B6, B7, B8};
            int dot;
            number = 0;
            for (dot=0; dot<sizeof(dots); ++dot) {
              if (arg & dots[dot]) {
                number = (number * 10) + (dot + 1);
              }
            }
            break;
          }
        }
        snprintf(buffer, sizeof(buffer), "%s[%d]: %s",
                 candidate->name, number, candidate->description);
      } else {
        int offset;
        snprintf(buffer, sizeof(buffer), "%s: %n%s",
                 candidate->name, &offset, candidate->description);
        if ((blk == 0) && (key & VAL_SWITCHMASK)) {
          char *description = buffer + offset;
          const char *oldVerb = "toggle";
          int oldLength = strlen(oldVerb);
          if (strncmp(description, oldVerb, oldLength) == 0) {
            const char *newVerb = "set";
            int newLength = strlen(newVerb);
            memmove(description+newLength, description+oldLength, strlen(description+oldLength)+1);
            memcpy(description, newVerb, newLength);
            if (key & VAL_SWITCHON) {
              char *end = strrchr(description, '/');
              if (end) *end = 0;
            } else {
              char *target = strrchr(description, ' ');
              if (target) {
                const char *source = strchr(++target, '/');
                if (source) {
                  memmove(target, source+1, strlen(source));
                }
              }
            }
          }
        }
      }
      message(buffer, MSG_NODELAY);
      timer = 0;
    }
  }
  message("done", 0);
}
