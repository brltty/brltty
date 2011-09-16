/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2011 by The BRLTTY Developers.
 *
 * BRLTTY comes with ABSOLUTELY NO WARRANTY.
 *
 * This is free software, placed under the terms of the
 * GNU General Public License, as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any
 * later version. Please see the file LICENSE-GPL for details.
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

#include "log.h"
#include "file.h"
#include "parse.h"
#include "charset.h"
#include "prefs.h"
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

static int
spk_construct (SpeechSynthesizer *spk, char **parameters) {
  return 1;
}

static void
spk_destruct (SpeechSynthesizer *spk) {
}

static void
spk_say (SpeechSynthesizer *spk, const unsigned char *text, size_t length, size_t count, const unsigned char *attributes) {
}

static void
spk_mute (SpeechSynthesizer *spk) {
}

const SpeechDriver *speech = &noSpeech;

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
initializeSpeechSynthesizer (SpeechSynthesizer *spk) {
  spk->data = NULL;
}

void
sayCharacters (SpeechSynthesizer *spk, const char *characters, size_t count, int mute) {
  if (count) {
    unsigned char bytes[(count * UTF8_LEN_MAX) + 1];
    unsigned char *b = bytes;

    {
      int i;
      for (i=0; i<count; i+=1) {
        Utf8Buffer utf8;
        size_t utfs = convertCharToUtf8(characters[i], utf8);

        if (utfs) {
          memcpy(b, utf8, utfs);
          b += utfs;
        } else {
          *b++ = ' ';
        }
      }

      *b = 0;
    }

    if (mute) speech->mute(spk);
    speech->say(spk, bytes, b-bytes, count, NULL);
  }
}

void
sayString (SpeechSynthesizer *spk, const char *string, int mute) {
  sayCharacters(spk, string, strlen(string), mute);
}

static void
saySpeechSetting (SpeechSynthesizer *spk, int setting, const char *name) {
  if (!prefs.autospeak) {
    char phrase[0X40];
    snprintf(phrase, sizeof(phrase), "%s %d", name, setting);
    sayString(spk, phrase, 1);
  }
}

static unsigned int
getIntegerSetting (unsigned char setting, unsigned char internal, unsigned int external) {
  return rescaleInteger(setting, internal, external);
}

void
setSpeechRate (SpeechSynthesizer *spk, int setting, int say) {
  logMessage(LOG_DEBUG, "setting speech rate: %d", setting);
  speech->rate(spk, setting);
  if (say) saySpeechSetting(spk, setting, "rate");
}

unsigned int
getIntegerSpeechRate (unsigned char setting, unsigned int normal) {
  return getIntegerSetting(setting, SPK_RATE_DEFAULT, normal);
}

float
getFloatSpeechRate (unsigned char setting) {
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

  return spkRateTable[setting];
}

void
setSpeechVolume (SpeechSynthesizer *spk, int setting, int say) {
  logMessage(LOG_DEBUG, "setting speech volume: %d", setting);
  speech->volume(spk, setting);
  if (say) saySpeechSetting(spk, setting, "volume");
}

unsigned int
getIntegerSpeechVolume (unsigned char setting, unsigned int normal) {
  return getIntegerSetting(setting, SPK_VOLUME_DEFAULT, normal);
}

float
getFloatSpeechVolume (unsigned char setting) {
  return (float)setting / (float)SPK_VOLUME_DEFAULT;
}

void
setSpeechPitch (SpeechSynthesizer *spk, int setting, int say) {
  logMessage(LOG_DEBUG, "setting speech pitch: %d", setting);
  speech->pitch(spk, setting);
  if (say) saySpeechSetting(spk, setting, "pitch");
}

unsigned int
getIntegerSpeechPitch (unsigned char setting, unsigned int normal) {
  return getIntegerSetting(setting, SPK_PITCH_DEFAULT, normal);
}

float
getFloatSpeechPitch (unsigned char setting) {
  return (float)setting / (float)SPK_PITCH_DEFAULT;
}

void
setSpeechPunctuation (SpeechSynthesizer *spk, SpeechPunctuation setting, int say) {
  logMessage(LOG_DEBUG, "setting speech punctuation: %d", setting);
  speech->punctuation(spk, setting);
}

static char *speechInputPath = NULL;
#ifdef __MINGW32__
static HANDLE speechInputHandle = INVALID_HANDLE_VALUE;
static int speechInputConnected;
#else /* __MINGW32__ */
static int speechInputDescriptor = -1;
#endif /* __MINGW32__ */

static void
exitSpeechInput (void) {
  if (speechInputPath) {
#ifndef __MINGW32__
    unlink(speechInputPath);
#endif /* __MINGW32__ */

    free(speechInputPath);
    speechInputPath = NULL;
  }

#ifdef __MINGW32__
  if (speechInputHandle != INVALID_HANDLE_VALUE) {
    if (speechInputConnected) {
      DisconnectNamedPipe(speechInputHandle);
      speechInputConnected = 0;
    }

    CloseHandle(speechInputHandle);
    speechInputHandle = INVALID_HANDLE_VALUE;
  }
#else /* __MINGW32__ */
  if (speechInputDescriptor != -1) {
    close(speechInputDescriptor);
    speechInputDescriptor = -1;
  }
#endif /* __MINGW32__ */
}

int
enableSpeechInput (const char *name) {
  const char *directory;
  atexit(exitSpeechInput);

#ifdef __MINGW32__
  directory = "//./pipe";
#else /* __MINGW32__ */
  directory = ".";
#endif /* __MINGW32__ */

  if ((speechInputPath = makePath(directory, name))) {
#ifdef __MINGW32__
    if ((speechInputHandle = CreateNamedPipe(speechInputPath, PIPE_ACCESS_INBOUND,
                                             PIPE_TYPE_MESSAGE|PIPE_READMODE_MESSAGE|PIPE_NOWAIT,
                                             1, 0, 0, 0, NULL)) != INVALID_HANDLE_VALUE) {
      logMessage(LOG_DEBUG, "speech input pipe created: %s: handle=%u",
                 speechInputPath, (unsigned)speechInputHandle);
      return 1;
    } else {
      logWindowsSystemError("CreateNamedPipe");
    }
#else /* __MINGW32__ */
    int result = mkfifo(speechInputPath, 0);

    if ((result == -1) && (errno == EEXIST)) {
      struct stat fifo;

      if ((lstat(speechInputPath, &fifo) != -1) && S_ISFIFO(fifo.st_mode)) result = 0;
    }

    if (result != -1) {
      chmod(speechInputPath, S_IRUSR|S_IWUSR|S_IWGRP|S_IWOTH);

      if ((speechInputDescriptor = open(speechInputPath, O_RDONLY|O_NONBLOCK)) != -1) {
        logMessage(LOG_DEBUG, "speech input FIFO created: %s: fd=%d",
                   speechInputPath, speechInputDescriptor);
        return 1;
      } else {
        logMessage(LOG_ERR, "cannot open speech input FIFO: %s: %s",
                   speechInputPath, strerror(errno));
      }

      unlink(speechInputPath);
    } else {
      logMessage(LOG_ERR, "cannot create speech input FIFO: %s: %s",
                 speechInputPath, strerror(errno));
    }
#endif /* __MINGW32__ */

    free(speechInputPath);
    speechInputPath = NULL;
  }

  return 0;
}

void
processSpeechInput (SpeechSynthesizer *spk) {
#ifdef __MINGW32__
  if (speechInputHandle != INVALID_HANDLE_VALUE) {
    if (speechInputConnected ||
        (speechInputConnected = ConnectNamedPipe(speechInputHandle, NULL)) ||
        (speechInputConnected = (GetLastError() == ERROR_PIPE_CONNECTED))) {
      char buffer[0X1000];
      DWORD count;

      if (ReadFile(speechInputHandle, buffer, sizeof(buffer), &count, NULL)) {
        if (count) sayCharacters(spk, buffer, count, 0);
      } else {
        DWORD error = GetLastError();

        if (error != ERROR_NO_DATA) {
          speechInputConnected = 0;
          DisconnectNamedPipe(speechInputHandle);

          if (error != ERROR_BROKEN_PIPE)
            logWindowsSystemError("speech input FIFO read");
        }
      }
    }
  }
#else /* __MINGW32__ */
  if (speechInputDescriptor != -1) {
    char buffer[0X1000];
    int count = read(speechInputDescriptor, buffer, sizeof(buffer));
    if (count > 0) sayCharacters(spk, buffer, count, 0);
  }
#endif /* __MINGW32__ */
}
