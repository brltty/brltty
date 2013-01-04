/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2013 by The BRLTTY Developers.
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
#include "program.h"
#include "file.h"
#include "parse.h"
#include "prefs.h"
#include "charset.h"
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
sayStringSetting (SpeechSynthesizer *spk, const char *name, const char *string) {
  char statement[0X40];
  snprintf(statement, sizeof(statement), "%s %s", name, string);
  sayString(spk, statement, 1);
}

static void
sayIntegerSetting (SpeechSynthesizer *spk, const char *name, int integer) {
  char string[0X10];
  snprintf(string, sizeof(string), "%d", integer);
  sayStringSetting(spk, name, string);
}

static unsigned int
getIntegerSetting (unsigned char setting, unsigned char internal, unsigned int external) {
  return rescaleInteger(setting, internal, external);
}

int
setSpeechVolume (SpeechSynthesizer *spk, int setting, int say) {
  if (!speech->setVolume) return 0;
  logMessage(LOG_DEBUG, "setting speech volume: %d", setting);
  speech->setVolume(spk, setting);
  if (say) sayIntegerSetting(spk, gettext("volume"), setting);
  return 1;
}

unsigned int
getIntegerSpeechVolume (unsigned char setting, unsigned int normal) {
  return getIntegerSetting(setting, SPK_VOLUME_DEFAULT, normal);
}

#ifndef NO_FLOAT
float
getFloatSpeechVolume (unsigned char setting) {
  return (float)setting / (float)SPK_VOLUME_DEFAULT;
}
#endif /* NO_FLOAT */

int
setSpeechRate (SpeechSynthesizer *spk, int setting, int say) {
  if (!speech->setRate) return 0;
  logMessage(LOG_DEBUG, "setting speech rate: %d", setting);
  speech->setRate(spk, setting);
  if (say) sayIntegerSetting(spk, gettext("rate"), setting);
  return 1;
}

unsigned int
getIntegerSpeechRate (unsigned char setting, unsigned int normal) {
  return getIntegerSetting(setting, SPK_RATE_DEFAULT, normal);
}

#ifndef NO_FLOAT
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
#endif /* NO_FLOAT */

int
setSpeechPitch (SpeechSynthesizer *spk, int setting, int say) {
  if (!speech->setPitch) return 0;
  logMessage(LOG_DEBUG, "setting speech pitch: %d", setting);
  speech->setPitch(spk, setting);
  if (say) sayIntegerSetting(spk, gettext("pitch"), setting);
  return 1;
}

unsigned int
getIntegerSpeechPitch (unsigned char setting, unsigned int normal) {
  return getIntegerSetting(setting, SPK_PITCH_DEFAULT, normal);
}

#ifndef NO_FLOAT
float
getFloatSpeechPitch (unsigned char setting) {
  return (float)setting / (float)SPK_PITCH_DEFAULT;
}
#endif /* NO_FLOAT */

int
setSpeechPunctuation (SpeechSynthesizer *spk, SpeechPunctuation setting, int say) {
  if (!speech->setPunctuation) return 0;
  logMessage(LOG_DEBUG, "setting speech punctuation: %d", setting);
  speech->setPunctuation(spk, setting);
  return 1;
}

static char *speechInputPath = NULL;
#if defined(__MINGW32__)
static HANDLE speechInputHandle = INVALID_HANDLE_VALUE;
static int speechInputConnected;
#elif defined(S_ISFIFO)
static int speechInputDescriptor = -1;
#endif /* speech input definitions */

static void
exitSpeechInput (void) {
  if (speechInputPath) {
#if defined(__MINGW32__)
#elif defined(S_ISFIFO)
    unlink(speechInputPath);
#endif /* cleanup speech input */

    free(speechInputPath);
    speechInputPath = NULL;
  }

#if defined(__MINGW32__)
  if (speechInputHandle != INVALID_HANDLE_VALUE) {
    if (speechInputConnected) {
      DisconnectNamedPipe(speechInputHandle);
      speechInputConnected = 0;
    }

    CloseHandle(speechInputHandle);
    speechInputHandle = INVALID_HANDLE_VALUE;
  }
#elif defined(S_ISFIFO)
  if (speechInputDescriptor != -1) {
    close(speechInputDescriptor);
    speechInputDescriptor = -1;
  }
#endif /* disable speech input */
}

int
enableSpeechInput (const char *name) {
  const char *directory;
  onProgramExit(exitSpeechInput);

#if defined(__MINGW32__)
  directory = "//./pipe";
#else /* set speech input directory */
  directory = ".";
#endif /* set speech input directory */

  if ((speechInputPath = makePath(directory, name))) {
#if defined(__MINGW32__)
    if ((speechInputHandle = CreateNamedPipe(speechInputPath, PIPE_ACCESS_INBOUND,
                                             PIPE_TYPE_MESSAGE|PIPE_READMODE_MESSAGE|PIPE_NOWAIT,
                                             1, 0, 0, 0, NULL)) != INVALID_HANDLE_VALUE) {
      logMessage(LOG_DEBUG, "speech input pipe created: %s: handle=%u",
                 speechInputPath, (unsigned)speechInputHandle);
      return 1;
    } else {
      logWindowsSystemError("CreateNamedPipe");
    }
#elif defined(S_ISFIFO)
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
#endif /* enable speech input */

    free(speechInputPath);
    speechInputPath = NULL;
  }

  return 0;
}

void
processSpeechInput (SpeechSynthesizer *spk) {
#if defined(__MINGW32__)
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
#elif defined(S_ISFIFO)
  if (speechInputDescriptor != -1) {
    char buffer[0X1000];
    int count = read(speechInputDescriptor, buffer, sizeof(buffer));
    if (count > 0) sayCharacters(spk, buffer, count, 0);
  }
#endif /* process speech input */
}
