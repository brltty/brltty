/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2005 by The BRLTTY Team. All rights reserved.
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

/* Swift/speech.c - Speech library
 * For the Swift text to speech package
 * Maintained by Dave Mielke <dave@mielke.cc>
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>

#include "Programs/misc.h"

typedef enum {
  PARM_NAME
} DriverParameter;
#define SPKPARMS "name"

#define SPK_HAVE_RATE
#define SPK_HAVE_VOLUME
#include "Programs/spk_driver.h"
#include <swift.h>

static swift_engine *swiftEngine = NULL;
static swift_port *swiftPort = NULL;

static void
speechError (swift_result_t result, const char *action) {
  LogPrint(LOG_ERR, "Swift %s error: %s", action, swift_strerror(result));
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
  if (setPortParameter(name, swift_val_string(value))) return 1;
  LogPrint(LOG_WARNING, "Couldn't set %s=%s", name, value);
  return 0;
}

static int
setIntegerParameter (const char *name, int value) {
  if (setPortParameter(name, swift_val_int(value))) return 1;
  LogPrint(LOG_WARNING, "Couldn't set %s=%d", name, value);
  return 0;
}

static int
setRate (int rate) {
  return setIntegerParameter("speech/rate", rate);
}

static int
setVolume (int volume) {
  return setIntegerParameter("audio/volume", volume);
}

static void
spk_identify (void) {
  LogPrint(LOG_NOTICE, "Swift Speech Driver [using %s (for %s), version %s, %s].",
           swift_engine_name, swift_platform, swift_version, swift_date);
}

static int
spk_open (char **parameters) {
  swift_result_t result;

  if (setenv("SWIFT_HOME", SWIFT_ROOT, 1) != -1) {
    swift_params *engineParameters;

    if ((engineParameters = swift_params_new(NULL))) {
      swift_params_set_string(engineParameters,
                              "config/voice-path",
                              SWIFT_ROOT "/voices");

      if ((swiftEngine = swift_engine_open(engineParameters))) {
        if ((swiftPort = swift_port_open(swiftEngine, NULL))) {
          {
            const char *name = parameters[PARM_NAME];
            if (name && *name) {
              swift_voice *voice;
              if (*name == '/') {
                voice = swift_port_set_voice_from_dir(swiftPort, name);
              } else {
                voice = swift_port_set_voice_by_name(swiftPort, name);
              }

              if (!voice) {
                 LogPrint(LOG_WARNING, "Swift voice set error: %s", name);
              }
            }
          }

          setStringParameter("tts/content-type", "text/plain");
          return 1;
        } else {
          LogPrint(LOG_ERR, "Swift port open error.");
        }

        if ((result = swift_engine_close(swiftEngine)) != SWIFT_SUCCESS) {
          speechError(result, "engine close");
        }
        swiftEngine = NULL;
      } else {
        LogPrint(LOG_ERR, "Swift engine open error.");
      }
    } else {
      LogPrint(LOG_ERR, "Swift engine parameters allocation error.");
    }
  } else {
    LogPrint(LOG_ERR, "Unable to set Swift's home directory.");
  }

  return 0;
}

static void
spk_close (void) {
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

static void
spk_say (const unsigned char *buffer, int length) {
  swift_result_t result;
  swift_background_t job;

  if ((result = swift_port_speak_text(swiftPort, buffer, length, NULL, &job, NULL)) != SWIFT_SUCCESS) {
    speechError(result, "port speak");
  }
}

static void
spk_mute (void) {
  swift_result_t result;

  if ((result = swift_port_stop(swiftPort, SWIFT_ASYNC_CURRENT, SWIFT_EVENT_NOW)) != SWIFT_SUCCESS) {
    speechError(result, "port stop");
  }
}

static int
convertSetting (float setting, int reference, int scale) {
  return (int)(setting * (float)scale / (float)reference);
}

static void
spk_rate (int setting) {
  setRate(convertSetting(1.0/spkDurationStretchTable[setting], 1, 170));
}

static void
spk_volume (int setting) {
  setVolume(convertSetting(setting, SPK_DEFAULT_VOLUME, 100));
}
