/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2007 by The BRLTTY Developers.
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

#include "prologue.h"

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>

#include "misc.h"
#include "drivers.h"
#include "spk.h"
#include "spk.auto.h"

#define SPKSYMBOL noSpeech
#define DRIVER_NAME NoSpeech
#define DRIVER_CODE no
#define DRIVER_COMMENT "no speech support"
#define DRIVER_VERSION ""
#define DRIVER_DEVELOPERS ""
#include "spk_driver.h"
static int spk_open (char **parameters) { return 1; }
static void spk_close (void) { }
static void spk_say (const unsigned char *buffer, int length) { }
static void spk_mute (void) { }

const SpeechDriver *speech = &noSpeech;

static const float spkRateTable[] = {
  0.3333,
  0.3720,
  0.4152,
  0.4635,
  0.5173,
  0.5774,
  0.6444,
  0.7192,
  0.8027,
  0.8960,
  1.0000,
  1.1161,
  1.2457,
  1.3904,
  1.5518,
  1.7320,
  1.9332,
  2.1577,
  2.4082,
  2.6879,
  3.0000
};

int
haveSpeechDriver (const char *code) {
  return haveDriver(code, SPEECH_DRIVER_CODES, driverTable);
}

const char *
getDefaultSpeechDriver (void) {
  return getDefaultDriver(driverTable);
}

const SpeechDriver *
loadSpeechDriver (const char *code, void **driverObject, const char *driverDirectory) {
  return loadDriver(code, driverObject,
                    driverDirectory, driverTable,
                    "speech", 's', "spk",
                    &noSpeech, &noSpeech.definition);
}

void
identifySpeechDriver (const SpeechDriver *driver, int full) {
  identifyDriver("Speech", &driver->definition, full);
}

void
identifySpeechDrivers (int full) {
  const DriverEntry *entry = driverTable;
  while (entry->address) {
    const SpeechDriver *driver = entry++->address;
    identifySpeechDriver(driver, full);
  }
}

void
sayString (const char *string, int mute) {
  if (mute) speech->mute();
  speech->say((unsigned char *)string, strlen(string));
}

static void
saySpeechSetting (int setting, const char *name) {
  char phrase[0X40];
  snprintf(phrase, sizeof(phrase), "%s %d", name, setting);
  sayString(phrase, 1);
}

void
setSpeechRate (int setting, int say) {
  LogPrint(LOG_DEBUG, "setting speech rate: %d", setting);
  speech->rate(spkRateTable[setting]);
  if (say) saySpeechSetting(setting, "rate");
}

void
setSpeechVolume (int setting, int say) {
  LogPrint(LOG_DEBUG, "setting speech volume: %d", setting);
  speech->volume((float)setting / (float)SPK_DEFAULT_VOLUME);
  if (say) saySpeechSetting(setting, "volume");
}

static char *speechFifoPath = NULL;
#ifdef __MINGW32__
static HANDLE speechFifoHandle = INVALID_HANDLE_VALUE;
static int speechFifoConnected;
#else /* __MINGW32__ */
static int speechFifoDescriptor = -1;
#endif /* __MINGW32__ */

static void
exitSpeechFifo (void) {
  if (speechFifoPath) {
#ifndef __MINGW32__
    unlink(speechFifoPath);
#endif /* __MINGW32__ */
    free(speechFifoPath);
    speechFifoPath = NULL;
  }

#ifdef __MINGW32__
  if (speechFifoHandle != INVALID_HANDLE_VALUE) {
    if (speechFifoConnected) {
      DisconnectNamedPipe(speechFifoHandle);
      speechFifoConnected = 0;
    }
    CloseHandle(speechFifoHandle);
    speechFifoHandle = INVALID_HANDLE_VALUE;
  }
#else /* __MINGW32__ */
  if (speechFifoDescriptor != -1) {
    close(speechFifoDescriptor);
    speechFifoDescriptor = -1;
  }
#endif /* __MINGW32__ */
}

int
openSpeechFifo (const char *directory, const char *path) {
  atexit(exitSpeechFifo);

#ifdef __MINGW32__
  directory = "//./pipe";
#endif /* __MINGW32__ */

  if ((speechFifoPath = makePath(directory, path))) {
#ifdef __MINGW32__
    if ((speechFifoHandle = CreateNamedPipe(speechFifoPath, PIPE_ACCESS_INBOUND,
                                            PIPE_TYPE_MESSAGE|PIPE_READMODE_MESSAGE|PIPE_NOWAIT,
                                            1, 0, 0, 0, NULL)) != INVALID_HANDLE_VALUE) {
      LogPrint(LOG_DEBUG, "Speech FIFO created: %s: handle=%u",
               speechFifoPath, (unsigned)speechFifoHandle);
      return 1;
    } else {
      LogWindowsError("CreateNamedPipe");
    }
#else /* __MINGW32__ */
    int ret = mkfifo(speechFifoPath, 0);

    if ((ret == -1) && (errno == EEXIST)) {
      struct stat fifo;
      if ((lstat(speechFifoPath, &fifo) != -1) && S_ISFIFO(fifo.st_mode)) ret = 0;
    }

    if (ret != -1) {
      chmod(speechFifoPath, S_IRUSR|S_IWUSR|S_IWGRP|S_IWOTH);
      if ((speechFifoDescriptor = open(speechFifoPath, O_RDONLY|O_NONBLOCK)) != -1) {
        LogPrint(LOG_DEBUG, "Speech FIFO created: %s: fd=%d",
                 speechFifoPath, speechFifoDescriptor);
        return 1;
      } else {
        LogPrint(LOG_ERR, "cannot open speech FIFO: %s: %s",
                 speechFifoPath, strerror(errno));
      }

      unlink(speechFifoPath);
    } else {
      LogPrint(LOG_ERR, "cannot create speech FIFO: %s: %s",
               speechFifoPath, strerror(errno));
    }
#endif /* __MINGW32__ */

    free(speechFifoPath);
    speechFifoPath = NULL;
  }

  return 0;
}

void
processSpeechFifo (void) {
#ifdef __MINGW32__
  if (speechFifoHandle != INVALID_HANDLE_VALUE) {
    if (speechFifoConnected ||
        (speechFifoConnected = ConnectNamedPipe(speechFifoHandle, NULL)) ||
        (speechFifoConnected = (GetLastError() == ERROR_PIPE_CONNECTED))) {
      unsigned char buffer[0X1000];
      DWORD count;

      if (ReadFile(speechFifoHandle, buffer, sizeof(buffer), &count, NULL)) {
        if (count) speech->say(buffer, count);
      } else {
        DWORD error = GetLastError();

        if (error != ERROR_NO_DATA) {
          speechFifoConnected = 0;
          DisconnectNamedPipe(speechFifoHandle);

          if (error != ERROR_BROKEN_PIPE)
            LogWindowsError("Speech FIFO read");
        }
      }
    }
  }
#else /* __MINGW32__ */
  if (speechFifoDescriptor != -1) {
    unsigned char buffer[0X1000];
    int count = read(speechFifoDescriptor, buffer, sizeof(buffer));
    if (count > 0) speech->say(buffer, count);
  }
#endif /* __MINGW32__ */
}
