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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "misc.h"
#include "sysmisc.h"
#include "spk.h"
#include "spk.auto.h"

#define SPKSYMBOL noSpeech
#define SPKNAME NoSpeech
#define SPKCODE no
#include "spk_driver.h"
static void spk_identify (void) {
  LogPrint(LOG_NOTICE, "No speech support.");
}
static int spk_open (char **parameters) { return 0; }
static void spk_say (const unsigned char *buffer, int len) { }
static void spk_mute (void) { }
static void spk_close (void) { }

const SpeechDriver *speech = &noSpeech;

double spkDurationStretchTable[] = {
  3.0000,
  2.6879,
  2.4082,
  2.1577,
  1.9332,
  1.7320,
  1.5518,
  1.3904,
  1.2457,
  1.1161,
  1.0000,
  0.8960,
  0.8027,
  0.7192,
  0.6444,
  0.5774,
  0.5173,
  0.4635,
  0.4152,
  0.3720,
  0.3333
};

const SpeechDriver *
loadSpeechDriver (const char *driver, const char *driverDirectory) {
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

void
sayString (const char *string) {
  speech->say(string, strlen(string));
}

void
saySpeechSetting (int setting, const char *name) {
  char phrase[0X40];
  snprintf(phrase, sizeof(phrase), "%s %d", name, setting);
  speech->mute();
  sayString(phrase);
}

void
setSpeechRate (int setting) {
  speech->rate(setting);
  saySpeechSetting(setting, "rate");
}

void
setSpeechVolume (int setting) {
  speech->volume(setting);
  saySpeechSetting(setting, "volume");
}

static char *speechFifoPath = NULL;
static int speechFifoDescriptor = -1;

static void
exitSpeechFifo (void) {
  if (speechFifoPath) {
    unlink(speechFifoPath);
    free(speechFifoPath);
    speechFifoPath = NULL;
  }

  if (speechFifoDescriptor != -1) {
    close(speechFifoDescriptor);
    speechFifoDescriptor = -1;
  }
}

int
openSpeechFifo (const char *directory, const char *path) {
  atexit(exitSpeechFifo);
  if ((speechFifoPath = makePath(directory, path))) {
    int ret = mkfifo(speechFifoPath, 0);
    if ((ret == -1) && (errno == EEXIST)) {
      struct stat fifo;
      if ((lstat(speechFifoPath, &fifo) != -1) && S_ISFIFO(fifo.st_mode)) ret = 0;
    }
    if (ret != -1) {
      chmod(speechFifoPath, S_IRUSR|S_IWUSR|S_IWGRP|S_IWOTH);
      if ((speechFifoDescriptor = open(speechFifoPath, O_RDONLY|O_NDELAY)) != -1) {
        LogPrint(LOG_DEBUG, "Speech FIFO created: %s: fd=%d",
                 speechFifoPath, speechFifoDescriptor);
        return 1;
      } else {
        LogPrint(LOG_ERR, "Cannot open speech FIFO: %s: %s",
                 speechFifoPath, strerror(errno));
      }

      unlink(speechFifoPath);
    } else {
      LogPrint(LOG_ERR, "Cannot create speech FIFO: %s: %s",
               speechFifoPath, strerror(errno));
    }

    free(speechFifoPath);
    speechFifoPath = NULL;
  }
  return 0;
}

void
processSpeechFifo (void) {
  if (speechFifoDescriptor != -1) {
    unsigned char buffer[0X1000];
    int count = read(speechFifoDescriptor, buffer, sizeof(buffer));
    if (count > 0) speech->say(buffer, count);
  }
}
