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
braille_driver *braille = NULL;	/* filled by dynamic libs */

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
int loadBrailleDriver (const char **driver) {
  if (*driver != NULL)
    if (strcmp(*driver, noBraille.identifier) == 0) {
      braille = &noBraille;
      *driver = NULL;
      return 1;
    }

  #ifdef BRL_BUILTIN
    {
      extern braille_driver brl_driver;
      if (*driver != NULL)
        if (strcmp(*driver, brl_driver.identifier) == 0)
          *driver = NULL;
      if (*driver == NULL) {
        braille = &brl_driver;
        return 1;
      }
    }
  #else
    if (*driver == NULL) {
      braille = &noBraille;
      return 1;
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
      braille = dlsym(libraryHandle, symbolName);
      if (!(error = dlerror())) {
        return 1;
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

  braille = &noBraille;
  return 0;
}


int listBrailleDrivers (void) {
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
      if (candidate && (candidate->code != cmd))
        if ((blk == 0) || (candidate->code < last->code))
          candidate = NULL;
      if (!candidate) {
        snprintf(buffer, sizeof(buffer), "unknown: %06X", key);
      } else if ((candidate == last) && (blk != 0)) {
        snprintf(buffer, sizeof(buffer), "%s[%d]: %s",
                 candidate->name, cmd-last->code+1, candidate->description);
      } else {
        snprintf(buffer, sizeof(buffer), "%s: %s",
                 candidate->name, candidate->description);
      }
      message(buffer, MSG_NODELAY);
      timer = 0;
    }
  }
  message("done", 0);
}
