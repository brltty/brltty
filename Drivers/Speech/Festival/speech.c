/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2018 by The BRLTTY Developers.
 *
 * BRLTTY comes with ABSOLUTELY NO WARRANTY.
 *
 * This is free software, placed under the terms of the
 * GNU Lesser General Public License, as published by the Free Software
 * Foundation; either version 2.1 of the License, or (at your option) any
 * later version. Please see the file LICENSE-LGPL for details.
 *
 * Web Page: http://brltty.com/
 *
 * This software is maintained by Dave Mielke <dave@mielke.cc>.
 */

/* BRLTTY speech driver for the Festival text to speech engine.
 * Written by: Nikhil Nair <nn201@cus.cam.ac.uk>
 * Maintained by: Dave Mielke <dave@mielke.cc>
 */

#include "prologue.h"

#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <ctype.h>

#include "log.h"

typedef enum {
  PARM_COMMAND,
  PARM_NAME
} DriverParameter;
#define SPKPARMS "command", "name"

#include "spk_driver.h"
#include "speech.h"		/* for speech definitions */

static char **festivalParameters = NULL;
static FILE *festivalStream = NULL;
static float festivalRate;

static int writeCommand (const char *command, int reopen);

static int
setRate (float setting, int reopen) {
  char command[0X40];
  snprintf(command, sizeof(command), "(Parameter.set 'Duration_Stretch %f)", 1.0/setting);
  return writeCommand(command, reopen);
}

static void
closeStream (void) {
  logMessage(LOG_DEBUG, "stopping festival");
  pclose(festivalStream);
  festivalStream = NULL;
}

static int
openStream (void) {
  const char *command = festivalParameters[PARM_COMMAND];
  if (!command || !*command) command = "festival";

  logMessage(LOG_DEBUG, "starting festival: command=%s", command);
  if ((festivalStream = popen(command, "w"))) {
    setvbuf(festivalStream, NULL, _IOLBF, 0X1000);

    if (!writeCommand("(gc-status nil)", 0)) return 0;
    if (!writeCommand("(audio_mode 'async)", 0)) return 0;
    if (!writeCommand("(Parameter.set 'Audio_Method 'netaudio)", 0)) return 0;

    {
      const char *name = festivalParameters[PARM_NAME];
      if (name && *name) {
        if (strcasecmp(name, "Kevin") == 0) {
          if (!writeCommand("(voice_ked_diphone)", 0)) return 0;
        } else if (strcasecmp(name, "Kal") == 0) {
          if (!writeCommand("(voice_kal_diphone)", 0)) return 0;
        } else {
          logMessage(LOG_WARNING, "unknown Festival voice name: %s", name);
        }
      }
    }

    if (festivalRate != 0.0)
      if (!setRate(festivalRate, 0))
        return 0;

    return 1;
  }

  return 0;
}

static int
writeString (const char *string, int reopen) {
  if (!festivalStream) {
    if (!reopen) return 0;
    if (!openStream()) return 0;
  }

  fputs(string, festivalStream);
  if (!ferror(festivalStream)) return 1;

  logSystemError("fputs");
  closeStream();
  return 0;
}

static int
writeCommand (const char *command, int reopen) {
  return writeString(command, reopen) && writeString("\n", 0);
}

static void
spk_setRate (volatile SpeechSynthesizer *spk, unsigned char setting) {
  setRate(festivalRate=getFloatSpeechRate(setting), 1);
}

static int
spk_construct (volatile SpeechSynthesizer *spk, char **parameters) {
  spk->setRate = spk_setRate;

  festivalParameters = parameters;
  festivalRate = 0.0;
  return openStream();
}

static void
spk_destruct (volatile SpeechSynthesizer *spk) {
  if (writeCommand("(quit)", 0)) closeStream();
  festivalParameters = NULL;
}

static void
spk_say (volatile SpeechSynthesizer *spk, const unsigned char *buffer, size_t length, size_t count, const unsigned char *attributes) {
  int started = 0;
  unsigned int index;

  for (index=0; index<length; index+=1) {
    unsigned char byte = buffer[index];
    if (iscntrl(byte)) byte = ' ';

    if (!isspace(byte)) {
      if (!started) {
        if (!writeString("(SayText \"", 1)) return;
        started = 1;
      }
    }

    if (started) {
      const char bytes[] = {'\\', byte, 0};
      const char *string = bytes;

      if (!strchr("\\\"", byte)) string += 1;
      if (!writeString(string, 0)) return;
    }
  }

  if (started) {
    if (!writeString("\")\n", 0)) return;
  }
}

static void
spk_mute (volatile SpeechSynthesizer *spk) {
  writeCommand("(audio_mode 'shutup)", 0);
}
