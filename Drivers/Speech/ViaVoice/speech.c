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
 * Web Page: http://brltty.app/
 *
 * This software is maintained by Dave Mielke <dave@mielke.cc>.
 */

/* ViaVoice/speech.c */

#include "prologue.h"

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include <eci.h>

#include "log.h"
#include "parse.h"

#define MAXIMUM_SAMPLES 0X800

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

#include "spk_driver.h"
#include "speech.h"

static ECIHand eci = NULL_ECI_HAND;
static int currentUnits;

static char *sayBuffer = NULL;
static int saySize = 0;

static FILE *pcmStream = NULL;
static short *pcmBuffer = NULL;

static const int languageMap[] = {
   eciGeneralAmericanEnglish,
   eciBritishEnglish,
   eciCastilianSpanish,
   eciMexicanSpanish,
   eciStandardFrench,
   eciCanadianFrench,
   eciStandardGerman,
   eciStandardItalian,
   eciMandarinChinese,
   eciMandarinChineseGB,
   eciMandarinChinesePinYin,
   eciMandarinChineseUCS,
   eciTaiwaneseMandarin,
   eciTaiwaneseMandarinBig5,
   eciTaiwaneseMandarinZhuYin,
   eciTaiwaneseMandarinPinYin,
   eciTaiwaneseMandarinUCS,
   eciBrazilianPortuguese,
   eciStandardJapanese,
   eciStandardJapaneseSJIS,
   eciStandardJapaneseUCS,
   eciStandardFinnish,
   eciStandardKorean,
   eciStandardKoreanUHC,
   eciStandardKoreanUCS,
   eciStandardCantonese,
   eciStandardCantoneseGB,
   eciStandardCantoneseUCS,
   eciHongKongCantonese,
   eciHongKongCantoneseBig5,
   eciHongKongCantoneseUCS,
   eciStandardDutch,
   eciStandardNorwegian,
   eciStandardSwedish,
   eciStandardDanish,
   eciStandardReserved,
   eciStandardThai,
   eciStandardThaiTIS,
   NODEFINEDCODESET
};

static const char *const languageNames[] = {
   "GeneralAmericanEnglish",
   "BritishEnglish",
   "CastilianSpanish",
   "MexicanSpanish",
   "StandardFrench",
   "CanadianFrench",
   "StandardGerman",
   "StandardItalian",
   "MandarinChinese",
   "MandarinChineseGB",
   "MandarinChinesePinYin",
   "MandarinChineseUCS",
   "TaiwaneseMandarin",
   "TaiwaneseMandarinBig5",
   "TaiwaneseMandarinZhuYin",
   "TaiwaneseMandarinPinYin",
   "TaiwaneseMandarinUCS",
   "BrazilianPortuguese",
   "StandardJapanese",
   "StandardJapaneseSJIS",
   "StandardJapaneseUCS",
   "StandardFinnish",
   "StandardKorean",
   "StandardKoreanUHC",
   "StandardKoreanUCS",
   "StandardCantonese",
   "StandardCantoneseGB",
   "StandardCantoneseUCS",
   "HongKongCantonese",
   "HongKongCantoneseBig5",
   "HongKongCantoneseUCS",
   "StandardDutch",
   "StandardNorwegian",
   "StandardSwedish",
   "StandardDanish",
   "StandardReserved",
   "StandardThai",
   "StandardThaiTIS",
   "NoDefinedCodeSet",
   NULL
};

static void
reportError (ECIHand eci, const char *routine) {
   int status = eciProgStatus(eci);
   char message[100];
   eciErrorMessage(eci, message);
   logMessage(LOG_ERR, "%s error %4.4X: %s", routine, status, message);
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

         choice += 1;
      }
   }

   if (value == buffer) snprintf(buffer, sizeof(buffer), "%d", setting);
   logMessage(LOG_DEBUG, "ViaVoice Parameter: %s = %s", description, value);
}

static void
reportEnvironmentParameter (ECIHand eci, const char *description, enum ECIParam parameter, int setting, const char *const *choices, const int *map) {
   if (parameter != eciNumParams) setting = eciGetParam(eci, parameter);
   reportParameter(description, setting, choices, map);
}

static int
setEnvironmentParameter (ECIHand eci, const char *description, enum ECIParam parameter, int setting) {
   if (parameter == eciNumParams) {
      int ok = eciCopyVoice(eci, setting, 0);
      if (!ok) reportError(eci, "eciCopyVoice");
      return ok;
   }

   return eciSetParam(eci, parameter, setting) >= 0;
}

static int
choiceEnvironmentParameter (ECIHand eci, const char *description, const char *value, enum ECIParam parameter, const char *const *choices, const int *map) {
   int ok = 0;
   int assume = 1;

   if (*value) {
      unsigned int setting;

      if (validateChoice(&setting, value, choices)) {
	 if (map) setting = map[setting];

         if (setEnvironmentParameter(eci, description, parameter, setting)) {
	    ok = 1;
	    assume = setting;
	 }
      } else {
        logMessage(LOG_WARNING, "invalid %s setting: %s", description, value);
      }
   }

   reportEnvironmentParameter(eci, description, parameter, assume, choices, map);
   return ok;
}

static int rangeEnvironmentParameter (ECIHand eci, const char *description, const char *value, enum ECIParam parameter, int minimum, int maximum) UNUSED;
static int
rangeEnvironmentParameter (ECIHand eci, const char *description, const char *value, enum ECIParam parameter, int minimum, int maximum) {
   int ok = 0;
   int assume = 1;
   if (*value) {
      int setting;
      if (validateInteger(&setting, value, &minimum, &maximum)) {
         if (setEnvironmentParameter(eci, description, parameter, setting)) {
	    ok = 1;
	    assume = setting;
	 }
      } else {
        logMessage(LOG_WARNING, "invalid %s setting: %s", description, value);
      }
   }
   reportEnvironmentParameter(eci, description, parameter, assume, NULL, NULL);
   return ok;
}

static int
setUnits (int newUnits) {
   if (newUnits != currentUnits) {
      if (!setEnvironmentParameter(eci, "units", eciRealWorldUnits, newUnits)) return 0;
      currentUnits = newUnits;
   }

   return 1;
}

static int
setInternalUnits (void) {
   return setUnits(0);
}

static int
setExternalUnits (void) {
   return setUnits(1);
}

static int
setParameterUnits (ECIHand eci, enum ECIVoiceParam parameter) {
   switch (parameter) {
      case eciVolume:
         if (!setInternalUnits()) return 0;
         break;

      case eciPitchBaseline:
      case eciSpeed:
         if (!setExternalUnits()) return 0;
         break;

      default:
         break;
   }

   return 1;
}

static int
getVoiceParameter (ECIHand eci, enum ECIVoiceParam parameter) {
   if (!setParameterUnits(eci, parameter)) return 0;
   return eciGetVoiceParam(eci, 0, parameter);
}

static void
reportVoiceParameter (ECIHand eci, const char *description, enum ECIVoiceParam parameter, const char *const *choices, const int *map) {
   reportParameter(description, getVoiceParameter(eci, parameter), choices, map);
}

static int
setVoiceParameter (ECIHand eci, const char *description, enum ECIVoiceParam parameter, int setting) {
   if (!setParameterUnits(eci, parameter)) return 0;
   return eciSetVoiceParam(eci, 0, parameter, setting) >= 0;
}

static int
choiceVoiceParameter (ECIHand eci, const char *description, const char *value, enum ECIVoiceParam parameter, const char *const *choices, const int *map) {
   int ok = 0;

   if (*value) {
      unsigned int setting;

      if (validateChoice(&setting, value, choices)) {
	 if (map) setting = map[setting];
         if (setVoiceParameter(eci, description, parameter, setting)) ok = 1;
      } else {
        logMessage(LOG_WARNING, "invalid %s setting: %s", description, value);
      }
   }

   reportVoiceParameter(eci, description, parameter, choices, map);
   return ok;
}

static int
rangeVoiceParameter (ECIHand eci, const char *description, const char *value, enum ECIVoiceParam parameter, int minimum, int maximum) {
   int ok = 0;
   if (*value) {
      int setting;
      if (validateInteger(&setting, value, &minimum, &maximum)) {
         if (setVoiceParameter(eci, description, parameter, setting)) {
	    ok = 1;
	 }
      } else {
        logMessage(LOG_WARNING, "invalid %s setting: %s", description, value);
      }
   }
   reportVoiceParameter(eci, description, parameter, NULL, NULL);
   return ok;
}

static int
setIni (const char *path) {
   const char *variable = INI_VARIABLE;
   logMessage(LOG_DEBUG, "ViaVoice Ini Variable: %s", variable);

   if (!*path) {
      const char *value = getenv(variable);
      if (value) {
         path = value;
	 goto isSet;
      }
      value = INI_DEFAULT;
   }
   if (setenv(variable, path, 1) == -1) {
      logSystemError("environment variable assignment");
      return 0;
   }
isSet:

   logMessage(LOG_INFO, "ViaVoice Ini File: %s", path);
   return 1;
}

static int
ensureSayBuffer (int size) {
   if (size > saySize) {
      size |= 0XFF;
      size += 1;
      char *newBuffer = malloc(size);

      if (!newBuffer) {
         logSystemError("speech buffer allocation");
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
spk_say (volatile SpeechSynthesizer *spk, const unsigned char *buffer, size_t length, size_t count, const unsigned char *attributes) {
   if (eci) {
      int onSpace = -1;
      int sayFrom = 0;
      int index;

      for (index=0; index<length; index+=1) {
	 int isSpace = isspace(buffer[index])? 1: 0;

	 if (isSpace != onSpace) {
	    onSpace = isSpace;

	    if (index > sayFrom) {
	       if (!saySegment(eci, buffer, sayFrom, index)) break;
	       sayFrom = index;
	    }
	 }
      }

      if (index == length) {
	 if (saySegment(eci, buffer, sayFrom, index)) {
	    if (eciSynthesize(eci)) {
               if (eciSynchronize(eci)) {
                  fflush(pcmStream);
               } else {
                  reportError(eci, "eciSynchronize");
               }

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
spk_mute (volatile SpeechSynthesizer *spk) {
   if (eci) {
      if (eciStop(eci)) {
      } else {
         reportError(eci, "eciStop");
      }
   }
}

static void
spk_setVolume (volatile SpeechSynthesizer *spk, unsigned char setting) {
   setVoiceParameter(eci, "volume", eciVolume, getIntegerSpeechVolume(setting, 100));
}

static void
spk_setRate (volatile SpeechSynthesizer *spk, unsigned char setting) {
   setVoiceParameter (eci, "rate", eciSpeed, (int)(getFloatSpeechRate(setting) * 210.0));
}

static enum ECICallbackReturn
clientCallback (ECIHand eci, enum ECIMessage message, long parameter, void *data) {
   switch (message) {
      case eciWaveformBuffer:
         fwrite(pcmBuffer, sizeof(*pcmBuffer), parameter, pcmStream);
         if (ferror(pcmStream)) return eciDataAbort;
         break;

      default:
         break;
   }

   return eciDataProcessed;
}

static int
spk_construct (volatile SpeechSynthesizer *spk, char **parameters) {
   spk->setVolume = spk_setVolume;
   spk->setRate = spk_setRate;

   if (!eci) {
      if (setIni(parameters[PARM_IniFile])) {
         {
            char version[0X80];
            eciVersion(version);
            logMessage(LOG_INFO, "ViaVoice Engine: version %s", version);
         }

	 if ((eci = eciNew()) != NULL_ECI_HAND) {
            eciRegisterCallback(eci, clientCallback, NULL);

            currentUnits = eciGetParam(eci, eciRealWorldUnits);

            const char *sampleRates[] = {"8000", "11025", "22050", NULL};
            const char *abbreviationModes[] = {"on", "off", NULL};
            const char *numberModes[] = {"word", "year", NULL};
            const char *synthModes[] = {"sentence", "none", NULL};
            const char *textModes[] = {"talk", "spell", "literal", "phonetic", NULL};
            const char *voices[] = {"", "dad", "mom", "child", "", "", "", "grandma", "grandpa", NULL};
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

            if ((pcmBuffer = calloc(MAXIMUM_SAMPLES, sizeof(*pcmBuffer)))) {
               if (eciSetOutputBuffer(eci, MAXIMUM_SAMPLES, pcmBuffer)) {
                  int rate = eciGetParam(eci, eciSampleRate);
                  if (rate >= 0) rate = atoi(sampleRates[rate]);

                  char command[0X100];
                  snprintf(
                     command, sizeof(command),
                     "sox -q -t raw -c 1 -b %" PRIsize " -e signed-integer -r %d - -d",
                     (sizeof(*pcmBuffer) * 8), rate
                  );

                  if ((pcmStream = popen(command, "w"))) {
                     return 1;
                  } else {
                     logMessage(LOG_WARNING, "can't start command: %s", strerror(errno));
                  }
               } else {
                  reportError(eci, "eciSetOutputBuffer");
               }

               free(pcmBuffer);
               pcmBuffer = NULL;
            } else {
               logMallocError();
            }

            eciDelete(eci);
            eci = NULL_ECI_HAND;
	 } else {
	    logMessage(LOG_ERR, "ViaVoice initialization error");
	 }
      }
   }
   return 0;
}

static void
spk_destruct (volatile SpeechSynthesizer *spk) {
   if (eci) {
      eciDelete(eci);
      eci = NULL_ECI_HAND;
   }

   if (pcmStream) {
      pclose(pcmStream);
      pcmStream = NULL;
   }

   if (pcmBuffer) {
      free(pcmBuffer);
      pcmBuffer = NULL;
   }
}
