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

/* ViaVoice/speech.c */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <eci.h>

#include "Programs/misc.h"

typedef enum {
   PARM_IniFile,
   PARM_SampleRate,
   PARM_AbbreviationMode,
   PARM_NumberMode,
   PARM_SynthMode,
   PARM_TextMode,
   PARM_Language,
   PARM_Voice,
   PARM_VocalTract,
   PARM_Breathiness,
   PARM_HeadSize,
   PARM_PitchBaseline,
   PARM_PitchFluctuation,
   PARM_Roughness
} DriverParameter;
#define SPKPARMS "inifile", "samplerate", "abbreviationmode", "numbermode", "synthmode", "textmode", "language", "voice", "vocaltract", "breathiness", "headsize", "pitchbaseline", "pitchfluctuation", "roughness"

#define SPK_HAVE_TRACK
#define SPK_HAVE_RATE
#define SPK_HAVE_VOLUME
#include "Programs/spk_driver.h"
#include "speech.h"

static ECIHand eci = NULL_ECI_HAND;

static char *sayBuffer = NULL;
static int saySize = 0;

static const int languageMap[] = {
   eciGeneralAmericanEnglish,
   eciBritishEnglish,
   eciCastilianSpanish,
   eciMexicanSpanish,
   eciStandardFrench,
   eciCanadianFrench,
   eciStandardGerman,
   eciStandardItalian,
   eciSimplifiedChinese,
   eciBrazilianPortuguese
};
static const char *const languageNames[] = {
   "AmericanEnglish",
   "BritishEnglish",
   "CastilianSpanish",
   "MexicanSpanish",
   "StandardFrench",
   "CanadianFrench",
   "StandardGerman",
   "StandardItalian",
   "SimplifiedChinese",
   "BrazilianPortuguese",
   NULL
};

static void
reportError (ECIHand eci, const char *routine) {
   int status = eciProgStatus(eci);
   char message[100];
   eciErrorMessage(eci, message);
   LogPrint(LOG_ERR, "%s error %4.4X: %s", routine, status, message);
}

static void
reportParameter (const char *description, int setting, const char *const *choices, const int *map) {
   char buffer[0X10];
   const char *value = buffer;
   if (setting == -1) {
      value = "unknown";
   } else if (choices) {
      int choice = 0;
      while (choices[choice]) {
	 if (setting == (map? map[choice]: choice)) {
	    value = choices[choice];
	    break;
	 }
         ++choice;
      }
   }
   if (value == buffer) snprintf(buffer, sizeof(buffer), "%d", setting);
   LogPrint(LOG_DEBUG, "ViaVoice Parameter: %s = %s", description, value);
}

static void
reportEnvironmentParameter (ECIHand eci, const char *description, ECIParam parameter, int setting, const char *const *choices, const int *map) {
   if (parameter != eciNumParams) setting = eciGetParam(eci, parameter);
   reportParameter(description, setting, choices, map);
}

static int
setEnvironmentParameter (ECIHand eci, const char *description, ECIParam parameter, int setting) {
   if (parameter == eciNumParams) {
      int ok = eciCopyVoice(eci, setting, 0);
      if (!ok) reportError(eci, "eciCopyVoice");
      return ok;
   }
   return eciSetParam(eci, parameter, setting) >= 0;
}

static int
choiceEnvironmentParameter (ECIHand eci, const char *description, const char *value, ECIParam parameter, const char *const *choices, const int *map) {
   int ok = 0;
   int assume = 1;
   if (*value) {
      unsigned int setting;
      if (validateChoice(&setting, description, value, choices)) {
	 if (map) setting = map[setting];
         if (setEnvironmentParameter(eci, description, parameter, setting)) {
	    ok = 1;
	    assume = setting;
	 }
      }
   }
   reportEnvironmentParameter(eci, description, parameter, assume, choices, map);
   return ok;
}

static int rangeEnvironmentParameter (ECIHand eci, const char *description, const char *value, ECIParam parameter, int minimum, int maximum) __attribute__((unused));
static int
rangeEnvironmentParameter (ECIHand eci, const char *description, const char *value, ECIParam parameter, int minimum, int maximum) {
   int ok = 0;
   int assume = 1;
   if (*value) {
      int setting;
      if (validateInteger(&setting, description, value, &minimum, &maximum)) {
         if (setEnvironmentParameter(eci, description, parameter, setting)) {
	    ok = 1;
	    assume = setting;
	 }
      }
   }
   reportEnvironmentParameter(eci, description, parameter, assume, NULL, NULL);
   return ok;
}

static void
reportVoiceParameter (ECIHand eci, const char *description, ECIVoiceParam parameter, const char *const *choices, const int *map) {
   reportParameter(description, eciGetVoiceParam(eci, 0, parameter), choices, map);
}

static int
setVoiceParameter (ECIHand eci, const char *description, ECIVoiceParam parameter, int setting) {
   return eciSetVoiceParam(eci, 0, parameter, setting) >= 0;
}

static int
choiceVoiceParameter (ECIHand eci, const char *description, const char *value, ECIVoiceParam parameter, const char *const *choices, const int *map) {
   int ok = 0;
   if (*value) {
      unsigned int setting;
      if (validateChoice(&setting, description, value, choices)) {
	 if (map) setting = map[setting];
         if (setVoiceParameter(eci, description, parameter, setting)) {
	    ok = 1;
	 }
      }
   }
   reportVoiceParameter(eci, description, parameter, choices, map);
   return ok;
}

static int
rangeVoiceParameter (ECIHand eci, const char *description, const char *value, ECIVoiceParam parameter, int minimum, int maximum) {
   int ok = 0;
   if (*value) {
      int setting;
      if (validateInteger(&setting, description, value, &minimum, &maximum)) {
         if (setVoiceParameter(eci, description, parameter, setting)) {
	    ok = 1;
	 }
      }
   }
   reportVoiceParameter(eci, description, parameter, NULL, NULL);
   return ok;
}

static void
spk_identify (void) {
   {
      char version[0X80];
      eciVersion(version);
      LogPrint(LOG_INFO, "ViaVoice [%s] text to speech engine.", version);
   }
}

static int
setIni (const char *path) {
   const char *variable = INI_VARIABLE;
   LogPrint(LOG_DEBUG, "ViaVoice Ini Variable: %s", variable);

   if (!*path) {
      const char *value = getenv(variable);
      if (value) {
         path = value;
	 goto isSet;
      }
      value = INI_DEFAULT;
   }
   if (setenv(variable, path, 1) == -1) {
      LogError("environment variable assignment");
      return 0;
   }
isSet:

   LogPrint(LOG_INFO, "ViaVoice Ini File: %s", path);
   return 1;
}

static int
spk_open (char **parameters) {
   if (!eci) {
      if (setIni(parameters[PARM_IniFile])) {
	 if ((eci = eciNew()) != NULL_ECI_HAND) {
	    if (eciSetOutputDevice(eci, 0)) {
	       const char *sampleRates[] = {"8000", "11025", "22050", NULL};
	       const char *abbreviationModes[] = {"on", "off", NULL};
	       const char *numberModes[] = {"word", "year", NULL};
	       const char *synthModes[] = {"sentence", "none", NULL};
	       const char *textModes[] = {"talk", "spell", "literal", "phonetic", NULL};
	       const char *voices[] = {"", "AdultMale", "AdultFemale", "Child", "", "", "", "ElderlyFemale", "ElderlyMale", NULL};
	       const char *vocalTracts[] = {"male", "female", NULL};
	       choiceEnvironmentParameter(eci, "sample rate", parameters[PARM_SampleRate], eciSampleRate, sampleRates, NULL);
	       choiceEnvironmentParameter(eci, "abbreviation mode", parameters[PARM_AbbreviationMode], eciDictionary, abbreviationModes, NULL);
	       choiceEnvironmentParameter(eci, "number mode", parameters[PARM_NumberMode], eciNumberMode, numberModes, NULL);
	       choiceEnvironmentParameter(eci, "synth mode", parameters[PARM_SynthMode], eciSynthMode, synthModes, NULL);
	       choiceEnvironmentParameter(eci, "text mode", parameters[PARM_TextMode], eciTextMode, textModes, NULL);
	       choiceEnvironmentParameter(eci, "language", parameters[PARM_Language], eciLanguageDialect, languageNames, languageMap);
	       choiceEnvironmentParameter(eci, "voice", parameters[PARM_Voice], eciNumParams, voices, NULL);
	       choiceVoiceParameter(eci, "vocal tract", parameters[PARM_VocalTract], eciGender, vocalTracts, NULL);
	       rangeVoiceParameter(eci, "breathiness", parameters[PARM_Breathiness], eciBreathiness, 0, 100);
	       rangeVoiceParameter(eci, "head size", parameters[PARM_HeadSize], eciHeadSize, 0, 100);
	       rangeVoiceParameter(eci, "pitch baseline", parameters[PARM_PitchBaseline], eciPitchBaseline, 0, 100);
	       rangeVoiceParameter(eci, "pitch fluctuation", parameters[PARM_PitchFluctuation], eciPitchFluctuation, 0, 100);
	       rangeVoiceParameter(eci, "roughness", parameters[PARM_Roughness], eciRoughness, 0, 100);
               return 1;
	    } else {
	       reportError(eci, "eciSetOutputDevice");
	    }
            eciDelete(eci);
            eci = NULL_ECI_HAND;
	 } else {
	    LogPrint(LOG_ERR, "ViaVoice initialization error.");
	 }
      }
   }
   return 0;
}

static void
spk_close (void) {
   if (eci) {
      eciDelete(eci);
      eci = NULL_ECI_HAND;
   }
}

static int
ensureSayBuffer (int size) {
   if (size > saySize) {
      char *newBuffer = (char *)malloc(size |= 0XFF);
      if (!newBuffer) {
         LogError("speech buffer allocation");
	 return 0;
      }
      free(sayBuffer);
      sayBuffer = newBuffer;
      saySize = size;
   }
   return 1;
}

static int
saySegment (ECIHand eci, const unsigned char *buffer, int from, int to) {
   int length = to - from;
   if (!length) return 1;
   if (ensureSayBuffer(length+1)) {
      memcpy(sayBuffer, &buffer[from], length);
      sayBuffer[length] = 0;
      if (eciAddText(eci, sayBuffer)) {
	 if (eciInsertIndex(eci, to)) {
	    return 1;
	 } else {
	    reportError(eci, "eciInsertIndex");
	 }
      } else {
         reportError(eci, "eciAddText");
      }
   }
   return 0;
}

static void
spk_say (const unsigned char *buffer, int length) {
   if (eci) {
      int onSpace = -1;
      int sayFrom = 0;
      int index;
      for (index=0; index<length; ++index) {
	 int isSpace = isspace(buffer[index])? 1: 0;
	 if (isSpace != onSpace) {
	    onSpace = isSpace;
	    if (index > sayFrom) {
	       if (!saySegment(eci, buffer, sayFrom, index)) {
	          break;
	       }
	       sayFrom = index;
	    }
	 }
      }
      if (index == length) {
	 if (saySegment(eci, buffer, sayFrom, index)) {
	    if (eciSynthesize(eci)) {
	       return;
	    } else {
	       reportError(eci, "eciSynthesize");
	    }
	 }
      }
      eciStop(eci);
   }
}

static void
spk_mute (void) {
   if (eci) {
      if (eciStop(eci)) {
      } else {
         reportError(eci, "eciStop");
      }
   }
}

static void
spk_doTrack (void) {
   if (eci) {
      eciSpeaking(eci);
   }
}

static int
spk_getTrack (void) {
   if (eci) {
      int index = eciGetIndex(eci);
      LogPrint(LOG_DEBUG, "speech tracked at %d", index);
      return index;
   }
   return 0;
}

static int
spk_isSpeaking (void) {
   if (eci) {
      if (eciSpeaking(eci)) {
         return 1;
      }
   }
   return 0;
}

static void
spk_rate (int setting) {
   eciSetVoiceParam(eci, 0, eciSpeed,
                    (setting * 40 / SPK_DEFAULT_RATE) + 10);
}

static void
spk_volume (int setting) {
   double fraction = (double)setting / (double)SPK_MAXIMUM_VOLUME;
   int volume = (int)(fraction * (2.0 - fraction) * 100.0);
   eciSetVoiceParam(eci, 0, eciVolume, volume);
}
