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
#include <fcntl.h>

#include "misc.h"
#include "sysmisc.h"
#include "spk.h"

#define SPKSYMBOL noSpeech
#define SPKNAME "NoSpeech"
#define SPKDRIVER "no"
#include "spk_driver.h"
static void spk_identify (void) {
  LogPrint(LOG_NOTICE, "No speech support.");
}
static void spk_open (char **parameters) { }
static void spk_say (const unsigned char *buffer, int len) { }
static void spk_mute (void) { }
static void spk_close (void) { }

const SpeechDriver *speech = &noSpeech;

/* load driver from library */
/* return true (nonzero) on success */
const SpeechDriver *
loadSpeechDriver (const char **driver, const char *driverDirectory) {
#ifdef SPEECH_BUILTIN
  extern SpeechDriver spk_driver;
  const void *builtinAddress = &spk_driver;
  const char *builtinIdentifier = spk_driver.identifier;
#else /* SPEECH_BUILTIN */
  const void *builtinAddress = NULL;
  const char *builtinIdentifier = NULL;
#endif /* SPEECH_BUILTIN */
  return loadDriver(driver,
                    driverDirectory, "spk_driver",
                    "speech", 's',
                    builtinAddress, builtinIdentifier,
                    &noSpeech, noSpeech.identifier);
}

int
listSpeechDrivers (void) {
  char buf[64];
  static const char *list_file = LIBRARY_DIRECTORY "/brltty-spk.lst";
  int cnt, fd = open(list_file, O_RDONLY);
  if (fd < 0) {
    fprintf(stderr, "Error: can't access speech driver list file\n");
    perror(list_file);
    return 0;
  }
  fprintf(stderr, "Available Speech Drivers:\n\n");
  fprintf(stderr, "XX\tDescription\n");
  fprintf(stderr, "--\t-----------\n");
  while ((cnt = read(fd, buf, sizeof(buf))))
    fwrite(buf, cnt, 1, stderr);
  close(fd);
  return 1;
}
