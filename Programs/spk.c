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
#include "system.h"
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

static const char *symbolName = "spk_driver";
const SpeechDriver *speech = &noSpeech;

/* load driver from library */
/* return true (nonzero) on success */
const SpeechDriver *
loadSpeechDriver (const char **driver) {
  const void *symbolAddress = NULL;

  if (*driver != NULL)
    if (strcmp(*driver, noSpeech.identifier) == 0) {
      *driver = NULL;
      return &noSpeech;
    }

  #ifdef SPEECH_BUILTIN
    {
      extern SpeechDriver spk_driver;
      if (*driver != NULL)
        if (strcmp(*driver, spk_driver.identifier) == 0)
          *driver = NULL;
      if (*driver == NULL) {
        return &spk_driver;
      }
    }
  #else /* SPEECH_BUILTIN */
    if (*driver == NULL) {
      return &noSpeech;
    }
  #endif /* SPEECH_BUILTIN */

  {
    const char *libraryName = *driver;

    /* allow shortcuts */
    if (strlen(libraryName) == 2) {
      char buffer[0X80];
      snprintf(buffer, sizeof(buffer), "%ss%s.%s",
               LIBRARY_NAME, libraryName, LIBRARY_EXTENSION);
      libraryName = strdup(buffer);
    }

    if (libraryName) {
      void *libraryHandle = loadSharedObject(libraryName);
      if (libraryHandle) {
        if (!findSharedSymbol(libraryHandle, symbolName, &symbolAddress)) {
          LogPrint(LOG_ERR, "Speech driver symbol not found: %s", symbolName);
          unloadSharedObject(libraryHandle);
          symbolAddress = NULL;
        }
      } else {
        LogPrint(LOG_ERR, "Cannot open speech driver library: %s", libraryName);
      }
      if (libraryName != *driver) free((void *)libraryName);
    } else {
    }
  }

  return symbolAddress;
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
