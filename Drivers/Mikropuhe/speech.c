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

/* Mikropuhe/speech.c - Speech library
 * For the Mikropuhe text to speech package
 * Maintained by Dave Mielke <dave@mielke.cc>
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

#include "Programs/misc.h"
#include "Programs/system.h"

typedef enum {
  PARM_NAME,
  PARM_PITCH
} DriverParameter;
#define SPKPARMS "name", "pitch"

#define SPK_HAVE_RATE
#define SPK_HAVE_VOLUME
#include "Programs/spk_driver.h"
#include <mpwrfile.h>

static void *speechLibrary = NULL;
static MPINT_ChannelInitExType mpChannelInitEx = NULL;
static MPINT_ChannelExitType mpChannelExit = NULL;
static MPINT_ChannelSpeakFileType mpChannelSpeakFile = NULL;

typedef struct {
  const char *name;
  const void **address;
} SymbolEntry;
#define SYMBOL_ENTRY(name) {"MPINT_" #name, (const void **)&mp##name}
static const SymbolEntry symbolTable[] = {
  SYMBOL_ENTRY(ChannelInitEx),
  SYMBOL_ENTRY(ChannelExit),
  SYMBOL_ENTRY(ChannelSpeakFile),
  {NULL, NULL}
};

static void *speechChannel = NULL;
static MPINT_SpeakFileParams speechParameters;
static volatile int speechDevice = -1;

static void
speechError (int code, const char *action) {
  const char *explanation;
  switch (code) {
    default:
      explanation = "unknown";
      break;
    case MPINT_ERR_GENERAL:
      explanation = "general";
      break;
    case MPINT_ERR_SYNTH:
      explanation = "text synthesis";
      break;
    case MPINT_ERR_MEM:
      explanation = "insufficient memory";
      break;
    case MPINT_ERR_DESTFILEOPEN:
      explanation = "file open";
      break;
    case MPINT_ERR_DESTFILEWRITE:
      explanation = "file write";
      break;
    case MPINT_ERR_EINVAL:
      explanation = "parameter";
      break;
    case MPINT_ERR_INITBADKEY:
      explanation = "invalid key";
      break;
    case MPINT_ERR_INITNOVOICES:
      explanation = "no voices";
      break;
    case MPINT_ERR_INITVOICEFAIL:
      explanation = "voice load";
      break;
    case MPINT_ERR_INITTEXTPARSE:
      explanation = "text parser load";
      break;
    case MPINT_ERR_SOUNDCARD:
      explanation = "sound device";
      break;
  }
  LogPrint(LOG_ERR, "Mikropuhe %s error: %s", action, explanation);
}

static int
speechWriter (const void *bytes, unsigned int count, void *data, void *reserved) {
  int written = safe_write(speechDevice, bytes, count);
  if (written == count) return 0;

  if (written == -1) {
    LogError("Mikropuhe write");
  } else {
    LogPrint(LOG_ERR, "Mikropuhe incomplete write: %d < %d", written, count);
  }
  return MPINT_ERR_GENERAL;
}

static int
speechWrite (const char *text, int tags) {
  if (speechDevice == -1) {
    if ((speechDevice = getPcmDevice(LOG_WARNING)) == -1) return 0;

    speechParameters.nChannels = setPcmChannelCount(speechDevice, 1);
    speechParameters.nSampleFreq = setPcmSampleRate(speechDevice, 22050);
    {
      static const PcmAmplitudeFormat formats[] = {PCM_FMT_S16L, PCM_FMT_U8, PCM_FMT_UNKNOWN};
      const PcmAmplitudeFormat *format = formats;
      while (*format != PCM_FMT_UNKNOWN) {
        if (setPcmAmplitudeFormat(speechDevice, *format) == *format) break;
        ++format;
      }

      switch (setPcmAmplitudeFormat(speechDevice, *format)) {
        case PCM_FMT_U8:
          speechParameters.nBits = 8;
          break;
        case PCM_FMT_S16L:
          speechParameters.nBits = 16;
          break;
        default:
          speechParameters.nBits = 0;
          break;
      }
    }
    LogPrint(LOG_INFO, "Mikropuhe audio configuration: channels=%d rate=%d bits=%d",
             speechParameters.nChannels, speechParameters.nSampleFreq, speechParameters.nBits);

    if (!speechParameters.nBits) {
      close(speechDevice);
      speechDevice = -1;
      return 0;
    }
  }

  speechParameters.nTags = tags;
  if (mpChannelSpeakFile) {
    int code = mpChannelSpeakFile(speechChannel, text, NULL, &speechParameters);
    if (!code) return 1;
    speechError(code, "channel speak");
  }
  return 0;
}

static int
writeText (const char *text) {
  return speechWrite(text, 0);
}

static int
writeTag (const char *tag) {
  return speechWrite(tag, MPINT_TAGS_OWN|MPINT_TAGS_SAPI5);
}

static void
loadLibrary (void) {
  if (!speechLibrary) {
    static const char *name = "libmplinux." LIBRARY_EXTENSION;
    char *path = makePath(MIKROPUHE_ROOT, name);
    if (path) {
      if ((speechLibrary = loadSharedObject(path))) {
        const SymbolEntry *symbol = symbolTable;
        while (symbol->name) {
          if (findSharedSymbol(speechLibrary, symbol->name, symbol->address)) {
            LogPrint(LOG_DEBUG, "Mikropuhe symbol: %s -> %p",
                     symbol->name, *symbol->address);
          } else {
            LogPrint(LOG_ERR, "Mikropuhe symbol not found: %s", symbol->name);
          }
          ++symbol;
        }
      } else {
        LogPrint(LOG_ERR, "Mikropuhe library not loaded: %s", path);
      }
      free(path);
    }
  }
}

static void
spk_identify (void) {
  LogPrint(LOG_NOTICE, "Mikropuhe text to speech engine.");
}

static int
spk_open (char **parameters) {
  int code;
  loadLibrary();
  if (mpChannelInitEx) {
    if (!(code = mpChannelInitEx(&speechChannel, NULL, NULL, NULL))) {
      memset(&speechParameters, 0, sizeof(speechParameters));
      speechParameters.nWriteWavHeader = 0;
      speechParameters.pfnWrite = speechWriter;
      speechParameters.pWriteData = NULL;

      {
        const char *name = parameters[PARM_NAME];
        if (name && *name) {
          char tag[0X100];
          snprintf(tag, sizeof(tag), "<voice name=\"%s\"/>", name);
          writeTag(tag);
        }
      }

      {
        const char *pitch = parameters[PARM_PITCH];
        if (pitch && *pitch) {
          int setting = 0;
          static const int minimum = -10;
          static const int maximum = 10;
          if (validateInteger(&setting, "pitch", pitch, &minimum, &maximum)) {
            char tag[0X100];
            snprintf(tag, sizeof(tag), "<pitch absmiddle=\"%d\"/>", setting);
            writeTag(tag);
          }
        }
      }

      return 1;
    } else {
      speechError(code, "channel initialization");
    }
  }

  spk_close();
  return 0;
}

static void
spk_close (void) {
  if (speechChannel) {
    if (mpChannelExit) mpChannelExit(speechChannel, NULL, 0);
    speechChannel = NULL;
  }

  if (speechDevice != -1) {
    close(speechDevice);
    speechDevice = -1;
  }

  if (speechLibrary) {
    const SymbolEntry *symbol = symbolTable;
    while (symbol->name)
      *(symbol++)->address = NULL;

    unloadSharedObject(speechLibrary);
    speechLibrary = NULL;
  }
}

static void
spk_say (const unsigned char *buffer, int length) {
  unsigned char text[length + 1];
  memcpy(text, buffer, length);
  text[length] = 0;
  if (writeText(text))
    writeTag("<break time=\"none\"/>");
}

static void
spk_mute (void) {
}

static void
spk_rate (int setting) {
  char tag[0X40];
  snprintf(tag, sizeof(tag), "<rate absspeed=\"%d\"/>",
           setting * 10 / SPK_DEFAULT_RATE - 10);
  writeTag(tag);
}

static void
spk_volume (int setting) {
  char tag[0X40];
  int percentage = setting * 50 / SPK_DEFAULT_VOLUME;
  snprintf(tag, sizeof(tag), "<volume level=\"%d\"/>",
           MIN(100, MAX(1, percentage)));
  writeTag(tag);
}
