/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2014 by The BRLTTY Developers.
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
#include "parameters.h"
#include "program.h"
#include "file.h"
#include "parse.h"
#include "prefs.h"
#include "charset.h"
#include "spk.h"
#include "spk_thread.h"
#include "brltty.h"

void
constructSpeechSynthesizer (volatile SpeechSynthesizer *spk) {
  spk->canAutospeak = 1;

  spk->track.isActive = 0;
  spk->track.screenNumber = SPK_SCR_NONE;
  spk->track.firstLine = 0;
  spk->track.speechLocation = SPK_LOC_NONE;

  spk->setVolume = NULL;
  spk->setRate = NULL;
  spk->setPitch = NULL;
  spk->setPunctuation = NULL;

  spk->driver.thread = NULL;
  spk->driver.data = NULL;
}

void
destructSpeechSynthesizer (volatile SpeechSynthesizer *spk) {
}

int
startSpeechDriverThread (char **parameters) {
  if (!spk.driver.thread) {
    if (!(spk.driver.thread = newSpeechDriverThread(&spk, parameters))) {
      return 0;
    }
  }

  return 1;
}

void
stopSpeechDriverThread (void) {
  if (spk.driver.thread) {
    destroySpeechDriverThread(spk.driver.thread);
    spk.driver.thread = NULL;
  }
}

void
setSpeechFinished (void) {
  spk.track.isActive = 0;
  spk.track.speechLocation = SPK_LOC_NONE;

  endAutospeakDelay();
}

void
setSpeechLocation (int location) {
  if (spk.track.isActive) {
    if (scr.number == spk.track.screenNumber) {
      if (location != spk.track.speechLocation) {
        spk.track.speechLocation = location;
        if (ses->trackCursor) trackSpeech();
      }

      return;
    }

    setSpeechFinished();
  }
}

int
muteSpeech (const char *reason) {
  int result;

  logMessage(LOG_CATEGORY(SPEECH_EVENTS), "mute: %s", reason);
  result = speechRequest_muteSpeech(spk.driver.thread);

  setSpeechFinished();
  return result;
}

int
sayUtf8Characters (
  const char *text, const unsigned char *attributes,
  size_t length, size_t count,
  int immediate
) {
  if (count) {
    if (immediate) {
      if (!muteSpeech(__func__)) {
        return 0;
      }
    }

    logMessage(LOG_CATEGORY(SPEECH_EVENTS), "say: %s", text);
    if (!speechRequest_sayText(spk.driver.thread, text, length, count, attributes)) return 0;
  }

  return 1;
}

void
sayString (const char *string, int immediate) {
  sayUtf8Characters(string, NULL, strlen(string), getTextLength(string), immediate);
}

static void
sayStringSetting (const char *name, const char *string) {
  char statement[0X40];
  snprintf(statement, sizeof(statement), "%s %s", name, string);
  sayString(statement, 1);
}

static void
sayIntegerSetting (const char *name, int integer) {
  char string[0X10];
  snprintf(string, sizeof(string), "%d", integer);
  sayStringSetting(name, string);
}

int
canSetSpeechVolume (void) {
  return spk.setVolume != NULL;
}

int
setSpeechVolume (int setting, int say) {
  if (!canSetSpeechVolume()) return 0;
  logMessage(LOG_CATEGORY(SPEECH_EVENTS), "set volume: %d", setting);
  speechRequest_setVolume(spk.driver.thread, setting);
  if (say) sayIntegerSetting(gettext("volume"), setting);
  return 1;
}

int
canSetSpeechRate (void) {
  return spk.setRate != NULL;
}

int
setSpeechRate (int setting, int say) {
  if (!canSetSpeechRate()) return 0;
  logMessage(LOG_CATEGORY(SPEECH_EVENTS), "set rate: %d", setting);
  speechRequest_setRate(spk.driver.thread, setting);
  if (say) sayIntegerSetting(gettext("rate"), setting);
  return 1;
}

int
canSetSpeechPitch (void) {
  return spk.setPitch != NULL;
}

int
setSpeechPitch (int setting, int say) {
  if (!canSetSpeechPitch()) return 0;
  logMessage(LOG_CATEGORY(SPEECH_EVENTS), "set pitch: %d", setting);
  speechRequest_setPitch(spk.driver.thread, setting);
  if (say) sayIntegerSetting(gettext("pitch"), setting);
  return 1;
}

int
canSetSpeechPunctuation (void) {
  return spk.setPunctuation != NULL;
}

int
setSpeechPunctuation (SpeechPunctuation setting, int say) {
  if (!canSetSpeechPunctuation()) return 0;
  logMessage(LOG_CATEGORY(SPEECH_EVENTS), "set punctuation: %d", setting);
  speechRequest_setPunctuation(spk.driver.thread, setting);
  return 1;
}
