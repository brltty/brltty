/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2008 by The BRLTTY Developers.
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

#include "prologue.h"

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <eci.h>

#include "misc.h"

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
#include "spk_driver.h"
#include "speech.h"

static ECIHand eci = NULL_ECI_HAND;
static int units;

static char *sayBuffer = NULL;
static int saySize = 0;

static const int languageMap[] = {
#ifdef eciGeneralAmericanEnglish
   eciGeneralAmericanEnglish,
#endif /* eciGeneralAmericanEnglish */

#ifdef eciBritishEnglish
   eciBritishEnglish,
#endif /* eciBritishEnglish */

#ifdef eciCastilianSpanish
   eciCastilianSpanish,
#endif /* eciCastilianSpanish */

#ifdef eciMexicanSpanish
   eciMexicanSpanish,
#endif /* eciMexicanSpanish */

#ifdef eciStandardFrench
   eciStandardFrench,
#endif /* eciStandardFrench */

#ifdef eciCanadianFrench
   eciCanadianFrench,
#endif /* eciCanadianFrench */

#ifdef eciStandardGerman
   eciStandardGerman,
#endif /* eciStandardGerman */

#ifdef eciStandardItalian
   eciStandardItalian,
#endif /* eciStandardItalian */

#ifdef eciMandarinChinese
   eciMandarinChinese,
#endif /* eciMandarinChinese */

#ifdef eciMandarinChineseGB
   eciMandarinChineseGB,
#endif /* eciMandarinChineseGB */

#ifdef eciMandarinChinesePinYin
   eciMandarinChinesePinYin,
#endif /* eciMandarinChinesePinYin */

#ifdef eciMandarinChineseUCS
   eciMandarinChineseUCS,
#endif /* eciMandarinChineseUCS */

#ifdef eciTaiwaneseMandarin
   eciTaiwaneseMandarin,
#endif /* eciTaiwaneseMandarin */

#ifdef eciTaiwaneseMandarinBig5
   eciTaiwaneseMandarinBig5,
#endif /* eciTaiwaneseMandarinBig5 */

#ifdef eciTaiwaneseMandarinZhuYin
   eciTaiwaneseMandarinZhuYin,
#endif /* eciTaiwaneseMandarinZhuYin */

#ifdef eciTaiwaneseMandarinPinYin
   eciTaiwaneseMandarinPinYin,
#endif /* eciTaiwaneseMandarinPinYin */

#ifdef eciTaiwaneseMandarinUCS
   eciTaiwaneseMandarinUCS,
#endif /* eciTaiwaneseMandarinUCS */

#ifdef eciBrazilianPortuguese
   eciBrazilianPortuguese,
#endif /* eciBrazilianPortuguese */

#ifdef eciStandardJapanese
   eciStandardJapanese,
#endif /* eciStandardJapanese */

#ifdef eciStandardJapaneseSJIS
   eciStandardJapaneseSJIS,
#endif /* eciStandardJapaneseSJIS */

#ifdef eciStandardJapaneseUCS
   eciStandardJapaneseUCS,
#endif /* eciStandardJapaneseUCS */

#ifdef eciStandardFinnish
   eciStandardFinnish,
#endif /* eciStandardFinnish */

#ifdef eciStandardKorean
   eciStandardKorean,
#endif /* eciStandardKorean */

#ifdef eciStandardKoreanUHC
   eciStandardKoreanUHC,
#endif /* eciStandardKoreanUHC */

#ifdef eciStandardKoreanUCS
   eciStandardKoreanUCS,
#endif /* eciStandardKoreanUCS */

#ifdef eciStandardCantonese
   eciStandardCantonese,
#endif /* eciStandardCantonese */

#ifdef eciStandardCantoneseGB
   eciStandardCantoneseGB,
#endif /* eciStandardCantoneseGB */

#ifdef eciStandardCantoneseUCS
   eciStandardCantoneseUCS,
#endif /* eciStandardCantoneseUCS */

#ifdef eciHongKongCantonese
   eciHongKongCantonese,
#endif /* eciHongKongCantonese */

#ifdef eciHongKongCantoneseBig5
   eciHongKongCantoneseBig5,
#endif /* eciHongKongCantoneseBig5 */

#ifdef eciHongKongCantoneseUCS
   eciHongKongCantoneseUCS,
#endif /* eciHongKongCantoneseUCS */

#ifdef eciStandardDutch
   eciStandardDutch,
#endif /* eciStandardDutch */

#ifdef eciStandardNorwegian
   eciStandardNorwegian,
#endif /* eciStandardNorwegian */

#ifdef eciStandardSwedish
   eciStandardSwedish,
#endif /* eciStandardSwedish */

#ifdef eciStandardDanish
   eciStandardDanish,
#endif /* eciStandardDanish */

#ifdef eciStandardReserved
   eciStandardReserved,
#endif /* eciStandardReserved */

#ifdef eciStandardThai
   eciStandardThai,
#endif /* eciStandardThai */

#ifdef eciStandardThaiTIS
   eciStandardThaiTIS,
#endif /* eciStandardThaiTIS */

#ifdef NODEFINEDCODESET
   NODEFINEDCODESET,
#endif /* NODEFINEDCODESET */
};
static const char *const languageNames[] = {
#ifdef eciGeneralAmericanEnglish
   "GeneralAmericanEnglish",
#endif /* eciGeneralAmericanEnglish */

#ifdef eciBritishEnglish
   "BritishEnglish",
#endif /* eciBritishEnglish */

#ifdef eciCastilianSpanish
   "CastilianSpanish",
#endif /* eciCastilianSpanish */

#ifdef eciMexicanSpanish
   "MexicanSpanish",
#endif /* eciMexicanSpanish */

#ifdef eciStandardFrench
   "StandardFrench",
#endif /* eciStandardFrench */

#ifdef eciCanadianFrench
   "CanadianFrench",
#endif /* eciCanadianFrench */

#ifdef eciStandardGerman
   "StandardGerman",
#endif /* eciStandardGerman */

#ifdef eciStandardItalian
   "StandardItalian",
#endif /* eciStandardItalian */

#ifdef eciMandarinChinese
   "MandarinChinese",
#endif /* eciMandarinChinese */

#ifdef eciMandarinChineseGB
   "MandarinChineseGB",
#endif /* eciMandarinChineseGB */

#ifdef eciMandarinChinesePinYin
   "MandarinChinesePinYin",
#endif /* eciMandarinChinesePinYin */

#ifdef eciMandarinChineseUCS
   "MandarinChineseUCS",
#endif /* eciMandarinChineseUCS */

#ifdef eciTaiwaneseMandarin
   "TaiwaneseMandarin",
#endif /* eciTaiwaneseMandarin */

#ifdef eciTaiwaneseMandarinBig5
   "TaiwaneseMandarinBig5",
#endif /* eciTaiwaneseMandarinBig5 */

#ifdef eciTaiwaneseMandarinZhuYin
   "TaiwaneseMandarinZhuYin",
#endif /* eciTaiwaneseMandarinZhuYin */

#ifdef eciTaiwaneseMandarinPinYin
   "TaiwaneseMandarinPinYin",
#endif /* eciTaiwaneseMandarinPinYin */

#ifdef eciTaiwaneseMandarinUCS
   "TaiwaneseMandarinUCS",
#endif /* eciTaiwaneseMandarinUCS */

#ifdef eciBrazilianPortuguese
   "BrazilianPortuguese",
#endif /* eciBrazilianPortuguese */

#ifdef eciStandardJapanese
   "StandardJapanese",
#endif /* eciStandardJapanese */

#ifdef eciStandardJapaneseSJIS
   "StandardJapaneseSJIS",
#endif /* eciStandardJapaneseSJIS */

#ifdef eciStandardJapaneseUCS
   "StandardJapaneseUCS",
#endif /* eciStandardJapaneseUCS */

#ifdef eciStandardFinnish
   "StandardFinnish",
#endif /* eciStandardFinnish */

#ifdef eciStandardKorean
   "StandardKorean",
#endif /* eciStandardKorean */

#ifdef eciStandardKoreanUHC
   "StandardKoreanUHC",
#endif /* eciStandardKoreanUHC */

#ifdef eciStandardKoreanUCS
   "StandardKoreanUCS",
#endif /* eciStandardKoreanUCS */

#ifdef eciStandardCantonese
   "StandardCantonese",
#endif /* eciStandardCantonese */

#ifdef eciStandardCantoneseGB
   "StandardCantoneseGB",
#endif /* eciStandardCantoneseGB */

#ifdef eciStandardCantoneseUCS
   "StandardCantoneseUCS",
#endif /* eciStandardCantoneseUCS */

#ifdef eciHongKongCantonese
   "HongKongCantonese",
#endif /* eciHongKongCantonese */

#ifdef eciHongKongCantoneseBig5
   "HongKongCantoneseBig5",
#endif /* eciHongKongCantoneseBig5 */

#ifdef eciHongKongCantoneseUCS
   "HongKongCantoneseUCS",
#endif /* eciHongKongCantoneseUCS */

#ifdef eciStandardDutch
   "StandardDutch",
#endif /* eciStandardDutch */

#ifdef eciStandardNorwegian
   "StandardNorwegian",
#endif /* eciStandardNorwegian */

#ifdef eciStandardSwedish
   "StandardSwedish",
#endif /* eciStandardSwedish */

#ifdef eciStandardDanish
   "StandardDanish",
#endif /* eciStandardDanish */

#ifdef eciStandardReserved
   "StandardReserved",
#endif /* eciStandardReserved */

#ifdef eciStandardThai
   "StandardThai",
#endif /* eciStandardThai */

#ifdef eciStandardThaiTIS
   "StandardThaiTIS",
#endif /* eciStandardThaiTIS */

#ifdef NODEFINEDCODESET
   "NoDefinedCodeSet",
#endif /* NODEFINEDCODESET */

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
      if (validateChoice(&setting, value, choices)) {
	 if (map) setting = map[setting];
         if (setEnvironmentParameter(eci, description, parameter, setting)) {
	    ok = 1;
	    assume = setting;
	 }
      } else {
        LogPrint(LOG_WARNING, "invalid %s setting: %s", description, value);
      }
   }
   reportEnvironmentParameter(eci, description, parameter, assume, choices, map);
   return ok;
}

static int rangeEnvironmentParameter (ECIHand eci, const char *description, const char *value, ECIParam parameter, int minimum, int maximum) UNUSED;
static int
rangeEnvironmentParameter (ECIHand eci, const char *description, const char *value, ECIParam parameter, int minimum, int maximum) {
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
        LogPrint(LOG_WARNING, "invalid %s setting: %s", description, value);
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
      if (validateChoice(&setting, value, choices)) {
	 if (map) setting = map[setting];
         if (setVoiceParameter(eci, description, parameter, setting)) {
	    ok = 1;
	 }
      } else {
        LogPrint(LOG_WARNING, "invalid %s setting: %s", description, value);
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
      if (validateInteger(&setting, value, &minimum, &maximum)) {
         if (setVoiceParameter(eci, description, parameter, setting)) {
	    ok = 1;
	 }
      } else {
        LogPrint(LOG_WARNING, "invalid %s setting: %s", description, value);
      }
   }
   reportVoiceParameter(eci, description, parameter, NULL, NULL);
   return ok;
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
spk_construct (SpeechSynthesizer *spk, char **parameters) {
   if (!eci) {
      if (setIni(parameters[PARM_IniFile])) {
	 if ((eci = eciNew()) != NULL_ECI_HAND) {
            units = 0;
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
               {
                  char version[0X80];
                  eciVersion(version);
                  LogPrint(LOG_INFO, "ViaVoice Engine: version %s", version);
               }
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
spk_destruct (SpeechSynthesizer *spk) {
   if (eci) {
      eciDelete(eci);
      eci = NULL_ECI_HAND;
   }
}

static int
ensureSayBuffer (int size) {
   if (size > saySize) {
      char *newBuffer = malloc(size |= 0XFF);
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
spk_say (SpeechSynthesizer *spk, const unsigned char *buffer, size_t length, size_t count, const unsigned char *attributes) {
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
spk_mute (SpeechSynthesizer *spk) {
   if (eci) {
      if (eciStop(eci)) {
      } else {
         reportError(eci, "eciStop");
      }
   }
}

static void
spk_doTrack (SpeechSynthesizer *spk) {
   if (eci) {
      eciSpeaking(eci);
   }
}

static int
spk_getTrack (SpeechSynthesizer *spk) {
   if (eci) {
      int index = eciGetIndex(eci);
      LogPrint(LOG_DEBUG, "speech tracked at %d", index);
      return index;
   }
   return 0;
}

static int
spk_isSpeaking (SpeechSynthesizer *spk) {
   if (eci) {
      if (eciSpeaking(eci)) {
         return 1;
      }
   }
   return 0;
}

static int
setUnits (int setting) {
   if (setting != units) {
      if (!setVoiceParameter(eci, "units", eciRealWorldUnits, setting)) return 0;
      units = setting;
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

static void
spk_rate (SpeechSynthesizer *spk, float setting) {
   if (setExternalUnits()) setVoiceParameter (eci, "rate", eciSpeed, (int)(setting * 210.0));
}

static void
spk_volume (SpeechSynthesizer *spk, float setting) {
   if (setInternalUnits()) setVoiceParameter(eci, "volume", eciVolume, (int)(setting * 100.0));
}
