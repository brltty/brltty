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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>

#include "misc.h"
#include "sysmisc.h"
#include "spk.h"
#include "spk.auto.h"

#define SPKSYMBOL noSpeech
#define SPKNAME "NoSpeech"
#define SPKDRIVER no
#include "spk_driver.h"
static void spk_identify (void) {
  LogPrint(LOG_NOTICE, "No speech support.");
}
static void spk_open (char **parameters) { }
static void spk_say (const unsigned char *buffer, int len) { }
static void spk_mute (void) { }
static void spk_close (void) { }

const SpeechDriver *speech = &noSpeech;

const SpeechDriver *
loadSpeechDriver (const char **driver, const char *driverDirectory) {
  return loadDriver(driver,
                    driverDirectory, "spk_driver",
                    "speech", 's',
                    driverTable,
                    &noSpeech, noSpeech.identifier);
}

int
listSpeechDrivers (const char *directory) {
  int ok = 0;
  char *path = makePath(directory, "brltty-spk.lst");
  if (path) {
    int fd = open(path, O_RDONLY);
    if (fd != -1) {
      char buffer[0X40];
      int count;
      fprintf(stderr, "Available Speech Drivers:\n\n");
      fprintf(stderr, "XX  Description\n");
      fprintf(stderr, "--  -----------\n");
      while ((count = read(fd, buffer, sizeof(buffer))))
        fwrite(buffer, count, 1, stderr);
      ok = 1;
      close(fd);
    } else {
      LogPrint(LOG_ERR, "Cannot open speech driver list: %s: %s",
               path, strerror(errno));
    }
    free(path);
  }
  return ok;
}
