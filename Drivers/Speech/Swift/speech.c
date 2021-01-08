/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2021 by The BRLTTY Developers.
 *
 * BRLTTY comes with ABSOLUTELY NO WARRANTY.
 *
 * This is free software, placed under the terms of the
 * GNU Lesser General Public License, as published by the Free Software
 * Foundation; either version 2.1 of the License, or (at your option) any
 * later version. Please see the file LICENSE-LGPL for details.
 *
 * Web Page: http://brltty.app/
 *
 * This software is maintained by Dave Mielke <dave@mielke.cc>.
 */

/* Swift/speech.c - Speech library
 * For the Swift text to speech package
 * Maintained by Dave Mielke <dave@mielke.cc>
 */

#include "prologue.h"

#include <stdio.h>

#include "log.h"

typedef enum {
  PARM_NAME
} DriverParameter;
#define SPKPARMS "name"

#include "spk_driver.h"
#include <swift.h>

static swift_engine *swiftEngine = NULL;
static swift_port *swiftPort = NULL;

static void
speechError (swift_result_t result, const char *action) {
  logMessage(LOG_ERR, "Swift %s error: %s", action, swift_strerror(result));
}

static int
setPortParameter (const char *name, swift_val *value) {
  swift_result_t result = swift_port_set_param(swiftPort, name, value, SWIFT_ASYNC_NONE);
  if (result == SWIFT_SUCCESS) return 1;
  speechError(result, "parameter set");
  return 0;
}

static int
setStringParameter (const char *name, const char *value) {
  logMessage(LOG_DEBUG, "setting swift string parameter: %s=%s", name, value);
  if (setPortParameter(name, swift_val_string(value))) return 1;
  logMessage(LOG_WARNING, "Couldn't set %s=%s", name, value);
  return 0;
}

static int
setIntegerParameter (const char *name, int value) {
  logMessage(LOG_DEBUG, "setting swift integer parameter: %s=%d", name, value);
  if (setPortParameter(name, swift_val_int(value))) return 1;
  logMessage(LOG_WARNING, "Couldn't set %s=%d", name, value);
  return 0;
}

static int
setVolume (int percentage) {
  return setIntegerParameter("audio/volume", percentage);
}

static int
setRate (int wordsPerMinute) {
  return setIntegerParameter("speech/rate", wordsPerMinute);
}

static int
setEnvironmentVariable (const char *name, const char *value) {
  logMessage(LOG_DEBUG, "setting swift environment variable: %s=%s", name, value);
  if (setenv(name, value, 1) != -1) return 1;
  logSystemError("environment variable set");
  return 0;
}

static void
spk_say (volatile SpeechSynthesizer *spk, const unsigned char *buffer, size_t length, size_t count, const unsigned char *attributes) {
  swift_result_t result;
  swift_background_t job;

  if ((result = swift_port_speak_text(swiftPort, buffer, length, NULL, &job, NULL)) != SWIFT_SUCCESS) {
    speechError(result, "port speak");
  }
}

static void
spk_mute (volatile SpeechSynthesizer *spk) {
  swift_result_t result;

  if ((result = swift_port_stop(swiftPort, SWIFT_ASYNC_CURRENT, SWIFT_EVENT_NOW)) != SWIFT_SUCCESS) {
  //speechError(result, "port stop");
  }
}

static void
spk_setVolume (volatile SpeechSynthesizer *spk, unsigned char setting) {
  setVolume(getIntegerSpeechVolume(setting, 100));
}

static void
spk_setRate (volatile SpeechSynthesizer *spk, unsigned char setting) {
  setRate((int)(getFloatSpeechRate(setting) * 170.0));
}

static int
spk_construct (volatile SpeechSynthesizer *spk, char **parameters) {
  swift_result_t result;

  spk->setVolume = spk_setVolume;
  spk->setRate = spk_setRate;

  if (setEnvironmentVariable("SWIFT_HOME", SWIFT_ROOT)) {
    swift_params *engineParameters;

    if ((engineParameters = swift_params_new(NULL))) {
      if ((swiftEngine = swift_engine_open(engineParameters))) {
        if ((swiftPort = swift_port_open(swiftEngine, NULL))) {
          {
            const char *name = parameters[PARM_NAME];
            if (name && *name) {
              swift_voice *voice;
              if (*name == '/') {
                logMessage(LOG_DEBUG, "setting swift voice directory: %s", name);
                voice = swift_port_set_voice_from_dir(swiftPort, name);
              } else {
                logMessage(LOG_DEBUG, "setting swift voice name: %s", name);
                voice = swift_port_set_voice_by_name(swiftPort, name);
              }

              if (!voice) {
                 logMessage(LOG_WARNING, "Swift voice set error: %s", name);
              }
            }
          }

          setStringParameter("tts/content-type", "text/plain");

          logMessage(LOG_INFO, "Swift Engine: %s for %s, version %s, %s",
                     swift_engine_name, swift_platform, swift_version, swift_date);
          return 1;
        } else {
          logMessage(LOG_ERR, "Swift port open error.");
        }

        if ((result = swift_engine_close(swiftEngine)) != SWIFT_SUCCESS) {
          speechError(result, "engine close");
        }
        swiftEngine = NULL;
      } else {
        logMessage(LOG_ERR, "Swift engine open error.");
      }
    } else {
      logMessage(LOG_ERR, "Swift engine parameters allocation error.");
    }
  }

  return 0;
}

static void
spk_destruct (volatile SpeechSynthesizer *spk) {
  swift_result_t result;

  if (swiftPort) {
    if ((result = swift_port_close(swiftPort)) != SWIFT_SUCCESS) {
      speechError(result, "port close");
    }
    swiftPort = NULL;
  }

  if (swiftEngine) {
    if ((result = swift_engine_close(swiftEngine)) != SWIFT_SUCCESS) {
      speechError(result, "engine close");
    }
    swiftEngine = NULL;
  }
}
