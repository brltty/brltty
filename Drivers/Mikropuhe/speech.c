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
#include <sys/types.h>
#include <signal.h>
#include <sys/wait.h>

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

static const char *devicePath = "/dev/dsp";
static int deviceDescriptor = -1;

static pid_t childProcess = -1;
static int pipeDescriptors[2];
static const int *pipeOutput = &pipeDescriptors[0];
static const int *pipeInput = &pipeDescriptors[1];

static int
speechWriter (const void *bytes, unsigned int count, void *data, void *reserved) {
  if (deviceDescriptor != -1) {
    int written = write(deviceDescriptor, bytes, count);
    if (written == count) return 0;
    if (written == -1) {
      LogError("Mikropuhe write");
    } else {
      LogPrint(LOG_ERR, "Mikropuhe incomplete write: %d < %d", written, count);
    }
  }
  return 1;
}

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
    if ((code = mpChannelInitEx(&speechChannel, NULL, NULL, NULL)) == 0) {
      memset(&speechParameters, 0, sizeof(speechParameters));
      speechParameters.nTags = MPINT_TAGS_SAPI5;
      speechParameters.nSampleFreq = 22050;
      speechParameters.nBits = 16;
      speechParameters.nChannels = 1;
      speechParameters.nWriteWavHeader = 0;
      speechParameters.pfnWrite = speechWriter;
      speechParameters.pWriteData = NULL;

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
  spk_mute();

  if (speechChannel) {
    if (mpChannelExit) mpChannelExit(speechChannel, NULL, 0);
    speechChannel = NULL;
  }

  if (speechLibrary) {
    const SymbolEntry *symbol = symbolTable;
    while (symbol->name)
      *(symbol++)->address = NULL;

    unloadSharedObject(speechLibrary);
    speechLibrary = NULL;
  }
}

static int
doChildProcess (void) {
  FILE *stream = fdopen(*pipeOutput, "r");
  char buffer[0X400];
  char *line;
  while ((line = fgets(buffer, sizeof(buffer), stream))) {
    if (mpChannelSpeakFile) {
      int code;
      if ((code = mpChannelSpeakFile(speechChannel, line, NULL, &speechParameters)) != 0)
        speechError(code, "channel speak");
    }
  }
  return 0;
}

static void
spk_say (const unsigned char *buffer, int length) {
  if (speechChannel) {
    if (childProcess != -1) goto ready;

    if (pipe(pipeDescriptors) != -1) {
      if ((childProcess = fork()) == -1) {
        LogError("fork");
      } else if (childProcess == 0) {
        _exit(doChildProcess());
      } else
      ready: {
        unsigned char text[length + 1];
        memcpy(text, buffer, length);
        text[length] = '\n';
        write(*pipeInput, text, sizeof(text));
        return;
      }

      close(*pipeInput);
      close(*pipeOutput);
    } else {
      LogError("pipe");
    }
  }
}

static void
spk_mute (void) {
  if (childProcess != -1) {
    close(*pipeInput);
    close(*pipeOutput);

    kill(childProcess, SIGKILL);
    waitpid(childProcess, NULL, 0);
    childProcess = -1;
  }
}

static void
spk_rate (int setting) {
}

static void
spk_volume (int setting) {
}
