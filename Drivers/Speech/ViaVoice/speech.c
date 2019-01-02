/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2019 by The BRLTTY Developers.
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

#ifdef HAVE_ICONV_H
#include <iconv.h>
#define ICONV_NULL ((iconv_t)-1)
#else /* HAVE_ICONV_H */
#warning iconv is not available
#endif /* HAVE_ICONV_H */

#include "log.h"
#include "parse.h"

typedef enum {
   PARM_Quality,
   PARM_Mode,
   PARM_Synthesize,
   PARM_Abbreviations,
   PARM_Years,
   PARM_Language,
   PARM_Voice,
   PARM_Gender,
   PARM_HeadSize,
   PARM_Pitch,
   PARM_Expressiveness,
   PARM_Roughness,
   PARM_Breathiness,
   PARM_Volume,
   PARM_Speed
} DriverParameter;
#define SPKPARMS "quality", "mode", "synthesize", "abbreviations", "years", "language", "voice", "gender", "headsize", "pitch", "expressiveness", "roughness", "breathiness", "volume", "speed"

#include "spk_driver.h"

#define MAXIMUM_SAMPLES 0X800

struct SpeechDataStruct {
   struct {
      int binary;
      char text[20];
   } version;

   struct {
      ECIHand handle;
      unsigned useSSML:1;
   } eci;

   struct {
      short *buffer;
      FILE *stream;
      char *command;
   } pcm;

   struct {
      char *buffer;
      size_t size;
   } say;

   struct {
      int unitType;
      int inputType;
   } current;

#ifdef ICONV_NULL
   struct {
      iconv_t handle;
   } iconv;
#endif /* ICONV_NULL */
};

typedef int MapFunction (int index);

typedef struct {
   const char *name; // must be first
   int rate;
} QualityChoice;

static const QualityChoice qualityChoices[] = {
   {  .name = "poor",
      .rate = 8000
   },

   {  .name = "fair",
      .rate = 11025
   },

   {  .name = "good",
      .rate = 22050
   },

   {  .name = NULL  }
};

static const char *abbreviationsChoices[] = {"on", "off", NULL};
static const char *yearsChoices[] = {"off", "on", NULL};
static const char *synthesizeChoices[] = {"sentences", "all", NULL};
static const char *modeChoices[] = {"words", "letters", "punctuation", "phonetic", NULL};
static const char *voiceChoices[] = {"", "Dad", "Mom", "child", "", "", "", "Grandma", "Grandpa", NULL};
static const char *genderChoices[] = {"male", "female", NULL};

typedef struct {
   const char *name; // must be first
   const char *language;
   const char *territory;
   const char *encoding;
   int identifier;
} LanguageChoice;

static const LanguageChoice languageChoices[] = {
   {  .identifier = eciGeneralAmericanEnglish,
      .name = "American-English",
      .language = "en",
      .territory = "US",
      .encoding = "ISO-8859-1"
   },

   {  .identifier = eciBritishEnglish,
      .name = "British-English",
      .language = "en",
      .territory = "GB",
      .encoding = "ISO-8859-1"
   },

   {  .identifier = eciCastilianSpanish,
      .name = "Castilian-Spanish",
      .language = "es",
      .territory = "ES",
      .encoding = "ISO-8859-1"
   },

   {  .identifier = eciMexicanSpanish,
      .name = "Mexican-Spanish",
      .language = "es",
      .territory = "MX",
      .encoding = "ISO-8859-1"
   },

   {  .identifier = eciStandardFrench,
      .name = "Standard-French",
      .language = "fr",
      .territory = "FR",
      .encoding = "ISO-8859-1"
   },

   {  .identifier = eciCanadianFrench,
      .name = "Canadian-French",
      .language = "fr",
      .territory = "CA",
      .encoding = "ISO-8859-1"
   },

   {  .identifier = eciStandardGerman,
      .name = "Standard-German",
      .language = "de",
      .territory = "DE",
      .encoding = "ISO-8859-1"
   },

   {  .identifier = eciStandardItalian,
      .name = "Standard-Italian",
      .language = "it",
      .territory = "IT",
      .encoding = "ISO-8859-1"
   },

   {  .identifier = eciMandarinChinese,
      .name = "Standard-Mandarin",
      .language = "zh",
      .territory = "CN",
      .encoding = "GBK"
   },

   {  .identifier = eciMandarinChineseGB,
      .name = "Standard-Mandarin-GB",
      .language = "zh",
      .territory = "CN_GB",
      .encoding = "GBK"
   },

   {  .identifier = eciMandarinChinesePinYin,
      .name = "Standard-Mandarin-PinYin",
      .language = "zh",
      .territory = "CN_PinYin",
      .encoding = "GBK"
   },

   {  .identifier = eciMandarinChineseUCS,
      .name = "Standard-Mandarin-UCS",
      .language = "zh",
      .territory = "CN_UCS",
      .encoding = "UCS2"
   },

   {  .identifier = eciTaiwaneseMandarin,
      .name = "Taiwanese-Mandarin",
      .language = "zh",
      .territory = "TW",
      .encoding = "BIG5"
   },

   {  .identifier = eciTaiwaneseMandarinBig5,
      .name = "Taiwanese-Mandarin-Big5",
      .language = "zh",
      .territory = "TW_Big5",
      .encoding = "BIG5"
   },

   {  .identifier = eciTaiwaneseMandarinZhuYin,
      .name = "Taiwanese-Mandarin-ZhuYin",
      .language = "zh",
      .territory = "TW_ZhuYin",
      .encoding = "BIG5"
   },

   {  .identifier = eciTaiwaneseMandarinPinYin,
      .name = "Taiwanese-Mandarin-PinYin",
      .language = "zh",
      .territory = "TW_PinYin",
      .encoding = "BIG5"
   },

   {  .identifier = eciTaiwaneseMandarinUCS,
      .name = "Taiwanese-Mandarin-UCS",
      .language = "zh",
      .territory = "TW_UCS",
      .encoding = "UCS2"
   },

   {  .identifier = eciBrazilianPortuguese,
      .name = "Brazilian-Portuguese",
      .language = "pt",
      .territory = "BR",
      .encoding = "ISO-8859-1"
   },

   {  .identifier = eciStandardJapanese,
      .name = "Standard-Japanese",
      .language = "ja",
      .territory = "JP",
      .encoding = "SJIS"
   },

   {  .identifier = eciStandardJapaneseSJIS,
      .name = "Standard-Japanese-SJIS",
      .language = "ja",
      .territory = "JP_SJIS",
      .encoding = "SJIS"
   },

   {  .identifier = eciStandardJapaneseUCS,
      .name = "Standard-Japanese-UCS",
      .language = "ja",
      .territory = "JP_UCS",
      .encoding = "UCS2"
   },

   {  .identifier = eciStandardFinnish,
      .name = "Standard-Finnish",
      .language = "fi",
      .territory = "FI",
      .encoding = "ISO-8859-1"
   },

   {  .identifier = eciStandardKorean,
      .name = "Standard-Korean",
      .language = "ko",
      .territory = "KR",
      .encoding = "UHC"
   },

   {  .identifier = eciStandardKoreanUHC,
      .name = "Standard-Korean-UHC",
      .language = "ko",
      .territory = "KR_UHC",
      .encoding = "UHC"
   },

   {  .identifier = eciStandardKoreanUCS,
      .name = "Standard-Korean-UCS",
      .language = "ko",
      .territory = "KR_UCS",
      .encoding = "UCS2"
   },

   {  .identifier = eciStandardCantonese,
      .name = "Standard-Cantonese",
      .language = "zh",
      .territory = "HK",
      .encoding = "GBK"
   },

   {  .identifier = eciStandardCantoneseGB,
      .name = "Standard-Cantonese-GB",
      .language = "zh",
      .territory = "HK_GB",
      .encoding = "GBK"
   },

   {  .identifier = eciStandardCantoneseUCS,
      .name = "Standard-Cantonese-UCS",
      .language = "zh",
      .territory = "HK_UCS",
      .encoding = "UCS2"
   },

   {  .identifier = eciHongKongCantonese,
      .name = "HongKong-Cantonese",
      .language = "zh",
      .territory = "HK",
      .encoding = "BIG5"
   },

   {  .identifier = eciHongKongCantoneseBig5,
      .name = "HongKong-Cantonese-Big5",
      .language = "zh",
      .territory = "HK_BIG5",
      .encoding = "BIG5"
   },

   {  .identifier = eciHongKongCantoneseUCS,
      .name = "HongKong-Cantonese-UCS",
      .language = "zh",
      .territory = "HK_UCS",
      .encoding = "UCS-2"
   },

   {  .identifier = eciStandardDutch,
      .name = "Standard-Dutch",
      .language = "nl",
      .territory = "NL",
      .encoding = "ISO-8859-1"
   },

   {  .identifier = eciStandardNorwegian,
      .name = "Standard-Norwegian",
      .language = "no",
      .territory = "NO",
      .encoding = "ISO-8859-1"
   },

   {  .identifier = eciStandardSwedish,
      .name = "Standard-Swedish",
      .language = "sv",
      .territory = "SE",
      .encoding = "ISO-8859-1"
   },

   {  .identifier = eciStandardDanish,
      .name = "Standard-Danish",
      .language = "da",
      .territory = "DK",
      .encoding = "ISO-8859-1"
   },

   {  .identifier = eciStandardThai,
      .name = "Standard-Thai",
      .language = "th",
      .territory = "TH",
      .encoding = "TIS-620"
   },

   {  .identifier = eciStandardThaiTIS,
      .name = "Standard-Thai-TIS",
      .language = "th",
      .territory = "TH_TIS",
      .encoding = "TIS-620"
   },

   {  .identifier = NODEFINEDCODESET  }
};

static int
mapLanguage (int index) {
   return languageChoices[index].identifier;
}

static void
reportError (volatile SpeechSynthesizer *spk, const char *routine) {
   int status = eciProgStatus(spk->driver.data->eci.handle);
   char message[100];
   eciErrorMessage(spk->driver.data->eci.handle, message);
   logMessage(LOG_ERR, "%s error %4.4X: %s", routine, status, message);
}

static void
reportParameter (const char *type, const char *description, int setting, const void *choices, size_t size, MapFunction *map, const char *unit) {
   char buffer[0X10];
   const char *value = buffer;

   if (setting == -1) {
      value = "unknown";
   } else if (choices) {
      const void *choice = choices;

      while (1) {
         typedef struct {
            const char *name;
         } Entry;

         const Entry *entry = choice;
         const char *name = entry->name;
         if (!name) break;
         int index = (choice - choices) / size;

	 if (setting == (map? map(index): index)) {
	    value = name;
	    break;
	 }

         choice += size;
      }
   }

   if (value == buffer) snprintf(buffer, sizeof(buffer), "%d", setting);
   if (!unit) unit = "";
   logMessage(LOG_DEBUG, "ViaVoice %s parameter: %s = %s%s", type, description, value, unit);
}

static int
getEnvironmentParameter (volatile SpeechSynthesizer *spk, enum ECIParam parameter) {
   return eciGetParam(spk->driver.data->eci.handle, parameter);
}

static const char *
getEnvironmentParameterUnit (enum ECIParam parameter) {
   switch (parameter) {
      default:
         return "";
   }
}

static void
reportEnvironmentParameter (volatile SpeechSynthesizer *spk, const char *description, enum ECIParam parameter, int setting, const void *choices, size_t size, MapFunction *map) {
   if (parameter != eciNumParams) setting = getEnvironmentParameter(spk, parameter);
   reportParameter("environment", description, setting, choices, size, map, getEnvironmentParameterUnit(parameter));
}

static int
setEnvironmentParameter (volatile SpeechSynthesizer *spk, const char *description, enum ECIParam parameter, int setting) {
   if (parameter == eciNumParams) {
      logMessage(LOG_CATEGORY(SPEECH_DRIVER), "copy voice: %d", setting);
      int ok = eciCopyVoice(spk->driver.data->eci.handle, setting, 0);
      if (!ok) reportError(spk, "eciCopyVoice");
      return ok;
   }

   logMessage(LOG_CATEGORY(SPEECH_DRIVER), "set environment parameter: %s: %d=%d", description, parameter, setting);
   return eciSetParam(spk->driver.data->eci.handle, parameter, setting) >= 0;
}

static int
choiceEnvironmentParameter (volatile SpeechSynthesizer *spk, const char *description, const char *value, enum ECIParam parameter, const void *choices, size_t size, MapFunction *map) {
   int ok = !*value;
   int assume = 1;

   if (!ok) {
      unsigned int setting;

      if (validateChoiceEx(&setting, value, choices, size)) {
	 if (map) setting = map(setting);

         if (setEnvironmentParameter(spk, description, parameter, setting)) {
	    ok = 1;
	    assume = setting;
         } else {
            logMessage(LOG_WARNING, "%s not supported: %s", description, value);
	 }
      } else {
        logMessage(LOG_WARNING, "invalid %s setting: %s", description, value);
      }
   }

   reportEnvironmentParameter(spk, description, parameter, assume, choices, size, map);
   return ok;
}

static int
setUnits (volatile SpeechSynthesizer *spk, int newUnits) {
   if (newUnits != spk->driver.data->current.unitType) {
      if (!setEnvironmentParameter(spk, "real world units", eciRealWorldUnits, newUnits)) return 0;
      spk->driver.data->current.unitType = newUnits;
   }

   return 1;
}

static int
useInternalUnit (volatile SpeechSynthesizer *spk) {
   return setUnits(spk, 0);
}

static int
useExternalUnit (volatile SpeechSynthesizer *spk) {
   return setUnits(spk, 1);
}

static int
useParameterUnit (volatile SpeechSynthesizer *spk, enum ECIVoiceParam parameter) {
   switch (parameter) {
      case eciVolume:
         if (!useInternalUnit(spk)) return 0;
         break;

      case eciPitchBaseline:
      case eciSpeed:
         if (!useExternalUnit(spk)) return 0;
         break;

      default:
         break;
   }

   return 1;
}

static int
getVoiceParameter (volatile SpeechSynthesizer *spk, enum ECIVoiceParam parameter) {
   if (!useParameterUnit(spk, parameter)) return 0;
   return eciGetVoiceParam(spk->driver.data->eci.handle, 0, parameter);
}

static const char *
getVoiceParameterUnit (enum ECIVoiceParam parameter) {
   switch (parameter) {
      case eciVolume:
         return "%";

      case eciPitchBaseline:
         return "Hz";

      case eciSpeed:
         return "wpm";

      default:
         return "";
   }
}

static void
reportVoiceParameter (volatile SpeechSynthesizer *spk, const char *description, enum ECIVoiceParam parameter, const char *const *choices, MapFunction *map) {
   reportParameter("voice", description, getVoiceParameter(spk, parameter), choices, sizeof(*choices), map, getVoiceParameterUnit(parameter));
}

static int
setVoiceParameter (volatile SpeechSynthesizer *spk, const char *description, enum ECIVoiceParam parameter, int setting) {
   if (!useParameterUnit(spk, parameter)) return 0;
   logMessage(LOG_CATEGORY(SPEECH_DRIVER), "set voice parameter: %s: %d=%d%s", description, parameter, setting, getVoiceParameterUnit(parameter));
   return eciSetVoiceParam(spk->driver.data->eci.handle, 0, parameter, setting) >= 0;
}

static int
choiceVoiceParameter (volatile SpeechSynthesizer *spk, const char *description, const char *value, enum ECIVoiceParam parameter, const char *const *choices, MapFunction *map) {
   int ok = !*value;

   if (!ok) {
      unsigned int setting;

      if (validateChoice(&setting, value, choices)) {
	 if (map) setting = map(setting);

         if (setVoiceParameter(spk, description, parameter, setting)) {
            ok = 1;
         } else {
            logMessage(LOG_WARNING, "%s not supported: %s", description, value);
         }
      } else {
        logMessage(LOG_WARNING, "invalid %s setting: %s", description, value);
      }
   }

   reportVoiceParameter(spk, description, parameter, choices, map);
   return ok;
}

static int
rangeVoiceParameter (volatile SpeechSynthesizer *spk, const char *description, const char *value, enum ECIVoiceParam parameter, int minimum, int maximum) {
   int ok = 0;

   if (*value) {
      int setting;

      if (validateInteger(&setting, value, &minimum, &maximum)) {
         if (setVoiceParameter(spk, description, parameter, setting)) {
	    ok = 1;
	 }
      } else {
        logMessage(LOG_WARNING, "invalid %s setting: %s", description, value);
      }
   }

   reportVoiceParameter(spk, description, parameter, NULL, NULL);
   return ok;
}

static void
spk_setVolume (volatile SpeechSynthesizer *spk, unsigned char setting) {
   setVoiceParameter(spk, "volume", eciVolume, getIntegerSpeechVolume(setting, 100));
}

static void
spk_setRate (volatile SpeechSynthesizer *spk, unsigned char setting) {
   setVoiceParameter(spk, "rate", eciSpeed, (int)(getFloatSpeechRate(setting) * 210.0));
}

static void
stopSpeech (volatile SpeechSynthesizer *spk) {
   logMessage(LOG_CATEGORY(SPEECH_DRIVER), "stop");

   if (eciStop(spk->driver.data->eci.handle)) {
   } else {
      reportError(spk, "eciStop");
   }
}

static void
spk_mute (volatile SpeechSynthesizer *spk) {
   stopSpeech(spk);
}

static int
pcmMakeCommand (volatile SpeechSynthesizer *spk) {
   int rate = getEnvironmentParameter(spk, eciSampleRate);
   char buffer[0X100];

   snprintf(
      buffer, sizeof(buffer),
      "sox -q -t raw -c 1 -b %" PRIsize " -e signed-integer -r %d - -d",
      (sizeof(*spk->driver.data->pcm.buffer) * 8), qualityChoices[rate].rate
   );

   logMessage(LOG_CATEGORY(SPEECH_DRIVER), "PCM command: %s", buffer);
   spk->driver.data->pcm.command = strdup(buffer);
   if (spk->driver.data->pcm.command) return 1;
   logMallocError();
   return 0;
}

static int
pcmOpenStream (volatile SpeechSynthesizer *spk) {
   if (!spk->driver.data->pcm.stream) {
      if (!spk->driver.data->pcm.command) {
         if (!pcmMakeCommand(spk)) {
            return 0;
         }
      }

      if (!(spk->driver.data->pcm.stream = popen(spk->driver.data->pcm.command, "w"))) {
         logMessage(LOG_WARNING, "can't start command: %s", strerror(errno));
         return 0;
      }

      setvbuf(spk->driver.data->pcm.stream, NULL, _IONBF, 0);
   }

   return 1;
}

static void
pcmCloseStream (volatile SpeechSynthesizer *spk) {
   if (spk->driver.data->pcm.stream) {
      pclose(spk->driver.data->pcm.stream);
      spk->driver.data->pcm.stream = NULL;
   }
}

static enum ECICallbackReturn
clientCallback (ECIHand eci, enum ECIMessage message, long parameter, void *data) {
   volatile SpeechSynthesizer *spk = data;

   switch (message) {
      case eciWaveformBuffer:
         logMessage(LOG_CATEGORY(SPEECH_DRIVER), "write samples: %ld", parameter);
         fwrite(spk->driver.data->pcm.buffer, sizeof(*spk->driver.data->pcm.buffer), parameter, spk->driver.data->pcm.stream);
         if (ferror(spk->driver.data->pcm.stream)) return eciDataAbort;
         break;

      case eciIndexReply:
         logMessage(LOG_CATEGORY(SPEECH_DRIVER), "index reply: %ld", parameter);
         tellSpeechLocation(spk, parameter);
         break;

      default:
         break;
   }

   return eciDataProcessed;
}

static int
addText (volatile SpeechSynthesizer *spk, const char *text) {
   logMessage(LOG_CATEGORY(SPEECH_DRIVER), "add text: \"%s\"", text);
   if (eciAddText(spk->driver.data->eci.handle, text)) return 1;
   reportError(spk, "eciAddText");
   return 0;
}

static int
insertIndex (volatile SpeechSynthesizer *spk, int index) {
   logMessage(LOG_CATEGORY(SPEECH_DRIVER), "insert index: %d", index);
   if (eciInsertIndex(spk->driver.data->eci.handle, index)) return 1;
   reportError(spk, "eciInsertIndex");
   return 0;
}

static int
setInputType (volatile SpeechSynthesizer *spk, int newInputType) {
   if (newInputType != spk->driver.data->current.inputType) {
      if (!setEnvironmentParameter(spk, "input type", eciInputType, newInputType)) return 0;
      spk->driver.data->current.inputType = newInputType;
   }

   return 1;
}

static int
disableAnnotations (volatile SpeechSynthesizer *spk) {
   return setInputType(spk, 0);
}

static int
enableAnnotations (volatile SpeechSynthesizer *spk) {
   return setInputType(spk, 1);
}

static int
writeAnnotation (volatile SpeechSynthesizer *spk, const char *annotation) {
   if (!enableAnnotations(spk)) return 0;

   char text[0X100];
   snprintf(text, sizeof(text), " `%s ", annotation);
   return addText(spk, text);
}

#ifdef ICONV_NULL
static int
prepareTextConversion (volatile SpeechSynthesizer *spk) {
   spk->driver.data->iconv.handle = ICONV_NULL;

   int identifier = getEnvironmentParameter(spk, eciLanguageDialect);
   const LanguageChoice *choice = languageChoices;

   while (choice->name) {
      if (choice->identifier == identifier) {
         iconv_t *handle = iconv_open(choice->encoding, "UTF-8");

         if (handle == ICONV_NULL) {
            logMessage(LOG_WARNING, "character encoding not supported: %s: %s", choice->encoding, strerror(errno));
            return 0;
         }

         logMessage(LOG_CATEGORY(SPEECH_DRIVER), "using character encoding: %s", choice->encoding);
         spk->driver.data->iconv.handle = handle;
         return 1;
      }

      choice += 1;
   }

   logMessage(LOG_WARNING, "language identifier not defined: 0X%08X", identifier);
   return 0;
}
#endif /* ICONV_NULL */

static int
ensureSayBufferSize (volatile SpeechSynthesizer *spk, size_t size) {
   if (size > spk->driver.data->say.size) {
      size |= 0XFF;
      size += 1;
      char *newBuffer = malloc(size);

      if (!newBuffer) {
         logSystemError("speech buffer allocation");
	 return 0;
      }

      if (spk->driver.data->say.buffer) free(spk->driver.data->say.buffer);
      spk->driver.data->say.buffer = newBuffer;
      spk->driver.data->say.size = size;
   }

   return 1;
}

static int
addCharacters (volatile SpeechSynthesizer *spk, const unsigned char *buffer, int from, int to) {
   int length = to - from;
   if (!length) return 1;

   if (!ensureSayBufferSize(spk, length+1)) return 0;
   memcpy(spk->driver.data->say.buffer, &buffer[from], length);
   spk->driver.data->say.buffer[length] = 0;
   return addText(spk, spk->driver.data->say.buffer);
}

static int
addSegment (volatile SpeechSynthesizer *spk, const unsigned char *buffer, int from, int to, const int *indexMap) {
   if (spk->driver.data->eci.useSSML) {
      for (int index=from; index<to; index+=1) {
         const char *entity = NULL;

         switch (buffer[index]) {
            case '<':
               entity = "lt";
               break;

            case '>':
               entity = "gt";
               break;

            case '&':
               entity = "amp";
               break;

            case '"':
               entity = "quot";
               break;

            case '\'':
               entity = "apos";
               break;

            default:
               continue;
         }

         if (!addCharacters(spk, buffer, from, index)) return 0;
         from = index + 1;

         size_t length = strlen(entity);
         char text[1 + length + 2];
         snprintf(text, sizeof(text), "&%s;", entity);
         if (!addText(spk, text)) return 0;
      }

      if (!addCharacters(spk, buffer, from, to)) return 0;
   } else {
#ifdef ICONV_NULL
      char *inputStart = (char *)&buffer[from];
      size_t inputLeft = to - from;
      size_t outputLeft = inputLeft * 10;
      char outputBuffer[outputLeft];
      char *outputStart = outputBuffer;
      int result = iconv(spk->driver.data->iconv.handle, &inputStart, &inputLeft, &outputStart, &outputLeft);

      if (result == -1) {
         logSystemError("iconv");
         return 0;
      }

      if (!addCharacters(spk, (unsigned char *)outputBuffer, 0, (outputStart - outputBuffer))) return 0;
#else /* ICONV_NULL */
      if (!addCharacters(spk, buffer, from, to)) return 0;
#endif /* ICONV_NULL */
   }

   return insertIndex(spk, indexMap[to]);
}

static int
addSegments (volatile SpeechSynthesizer *spk, const unsigned char *buffer, size_t length, const int *indexMap) {
   if (spk->driver.data->eci.useSSML && !addText(spk, "<speak>")) return 0;

   int wasSpace = 1;
   int from = 0;
   int to;

   for (to=0; to<length; to+=1) {
      int isSpace = isspace(buffer[to])? 1: 0;

      if (isSpace != wasSpace) {
         wasSpace = isSpace;

         if (to > from) {
            if (!addSegment(spk, buffer, from, to, indexMap)) return 0;
            from = to;
         }
      }
   }

   if (!addSegment(spk, buffer, from, to, indexMap)) return 0;
   if (spk->driver.data->eci.useSSML && !addText(spk, "</speak>")) return 0;
   return 1;
}

static void
spk_say (volatile SpeechSynthesizer *spk, const unsigned char *buffer, size_t length, size_t count, const unsigned char *attributes) {
   int ok = 0;

   if (pcmOpenStream(spk)) {
      int indexMap[length + 1];

      {
         int from = 0;
         int to = 0;

         while (from < length) {
            char character = buffer[from];
            indexMap[from++] = ((character & 0X80) && !(character & 0X40))? -1: to++;
         }

         indexMap[from] = to;
      }

      if (addSegments(spk, buffer, length, indexMap)) {
         logMessage(LOG_CATEGORY(SPEECH_DRIVER), "synthesize");

         if (eciSynthesize(spk->driver.data->eci.handle)) {
            logMessage(LOG_CATEGORY(SPEECH_DRIVER), "synchronize");

            if (eciSynchronize(spk->driver.data->eci.handle)) {
               logMessage(LOG_CATEGORY(SPEECH_DRIVER), "finished");
               tellSpeechFinished(spk);
               ok = 1;
            } else {
               reportError(spk, "eciSynchronize");
            }
         } else {
            reportError(spk, "eciSynthesize");
         }
      }

      if (!ok) stopSpeech(spk);
      pcmCloseStream(spk);
   }
}

static void
setParameters (volatile SpeechSynthesizer *spk, char **parameters) {
   choiceEnvironmentParameter(spk, "quality (sample rate)", parameters[PARM_Quality], eciSampleRate, qualityChoices, sizeof(*qualityChoices), NULL);
   choiceEnvironmentParameter(spk, "mode (text mode)", parameters[PARM_Mode], eciTextMode, modeChoices, sizeof(*modeChoices), NULL);
   choiceEnvironmentParameter(spk, "synthesize (synth mode)", parameters[PARM_Synthesize], eciSynthMode, synthesizeChoices, sizeof(*synthesizeChoices), NULL);
   choiceEnvironmentParameter(spk, "abbreviations (dictionary)", parameters[PARM_Abbreviations], eciDictionary, abbreviationsChoices, sizeof(*abbreviationsChoices), NULL);
   choiceEnvironmentParameter(spk, "years (number mode)", parameters[PARM_Years], eciNumberMode, yearsChoices, sizeof(*yearsChoices), NULL);
   choiceEnvironmentParameter(spk, "language", parameters[PARM_Language], eciLanguageDialect, languageChoices, sizeof(*languageChoices), mapLanguage);
   choiceEnvironmentParameter(spk, "voice name", parameters[PARM_Voice], eciNumParams, voiceChoices, sizeof(*voiceChoices), NULL);

   choiceVoiceParameter(spk, "gender", parameters[PARM_Gender], eciGender, genderChoices, NULL);
   rangeVoiceParameter(spk, "head size", parameters[PARM_HeadSize], eciHeadSize, 0, 100);
   rangeVoiceParameter(spk, "pitch (pitch baseline)", parameters[PARM_Pitch], eciPitchBaseline, 40, 422);
   rangeVoiceParameter(spk, "expressiveness (pitch fluctuation)", parameters[PARM_Expressiveness], eciPitchFluctuation, 0, 100);
   rangeVoiceParameter(spk, "roughness", parameters[PARM_Roughness], eciRoughness, 0, 100);
   rangeVoiceParameter(spk, "breathiness", parameters[PARM_Breathiness], eciBreathiness, 0, 100);
   rangeVoiceParameter(spk, "volume", parameters[PARM_Volume], eciVolume, 0, 100);
   rangeVoiceParameter(spk, "speed", parameters[PARM_Speed], eciSpeed, 70, 1297);

#ifdef ICONV_NULL
   spk->driver.data->eci.useSSML = !prepareTextConversion(spk);
#else /* ICONV_NULL */
   spk->driver.data->eci.useSSML = 1;
#endif /* ICONV_NULL */

   if (spk->driver.data->eci.useSSML) {
      logMessage(LOG_CATEGORY(SPEECH_DRIVER), "using SSML");
   }
}

static void
writeAnnotations (volatile SpeechSynthesizer *spk) {
   if (spk->driver.data->eci.useSSML) {
      writeAnnotation(spk, "gfa1"); // enable SSML
      writeAnnotation(spk, "gfa2");
   }

   disableAnnotations(spk);
}

static int
parseVersion (const char *text) {
   int result = 0;
   int width = 8;
   int shift = width * 4;

   while (*text && shift) {
      char *end;
      long digit = strtol(text, &end, 10);

      shift -= width;
      result |= digit << shift;

      text = end;
      if (*text) text += 1;
   }

   return result;
}

static int
spk_construct (volatile SpeechSynthesizer *spk, char **parameters) {
   spk->setVolume = spk_setVolume;
   spk->setRate = spk_setRate;

   if ((spk->driver.data = malloc(sizeof(*spk->driver.data)))) {
      memset(spk->driver.data, 0, sizeof(*spk->driver.data));

      eciVersion(spk->driver.data->version.text);
      logMessage(LOG_INFO, "ViaVoice Engine Version: %s", spk->driver.data->version.text);
      spk->driver.data->version.binary = parseVersion(spk->driver.data->version.text);

      spk->driver.data->eci.handle = NULL_ECI_HAND;
      spk->driver.data->eci.useSSML = 0;

      spk->driver.data->pcm.buffer = NULL;
      spk->driver.data->pcm.stream = NULL;
      spk->driver.data->pcm.command = NULL;

      spk->driver.data->say.buffer = NULL;
      spk->driver.data->say.size = 0;

      if ((spk->driver.data->eci.handle = eciNew()) != NULL_ECI_HAND) {
         eciRegisterCallback(spk->driver.data->eci.handle, clientCallback, (void *)spk);

         spk->driver.data->current.unitType = getEnvironmentParameter(spk, eciRealWorldUnits);
         spk->driver.data->current.inputType = getEnvironmentParameter(spk, eciInputType);

         if ((spk->driver.data->pcm.buffer = calloc(MAXIMUM_SAMPLES, sizeof(*spk->driver.data->pcm.buffer)))) {
            if (eciSetOutputBuffer(spk->driver.data->eci.handle, MAXIMUM_SAMPLES, spk->driver.data->pcm.buffer)) {
               setParameters(spk, parameters);
               writeAnnotations(spk);
               return 1;
            } else {
               reportError(spk, "eciSetOutputBuffer");
            }

            free(spk->driver.data->pcm.buffer);
         } else {
            logMallocError();
         }

         eciDelete(spk->driver.data->eci.handle);
      } else {
         logMessage(LOG_ERR, "ViaVoice initialization error");
      }

      free(spk->driver.data);
      spk->driver.data = NULL;
   } else {
      logMallocError();
   }

   return 0;
}

static void
spk_destruct (volatile SpeechSynthesizer *spk) {
   if (spk->driver.data) {
      if (spk->driver.data->eci.handle) eciDelete(spk->driver.data->eci.handle);
      if (spk->driver.data->say.buffer) free(spk->driver.data->say.buffer);
      if (spk->driver.data->pcm.buffer) free(spk->driver.data->pcm.buffer);
      if (spk->driver.data->pcm.command) free(spk->driver.data->pcm.command);
      pcmCloseStream(spk);

#ifdef ICONV_NULL
      if (spk->driver.data->iconv.handle != ICONV_NULL) iconv_close(spk->driver.data->iconv.handle);
#endif /* ICONV_NULL */

      free(spk->driver.data);
      spk->driver.data = NULL;
   }
}
