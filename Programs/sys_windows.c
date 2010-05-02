/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2010 by The BRLTTY Developers.
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

#include <fcntl.h>
#include <errno.h>

#include "log.h"
#include "timing.h"
#include "system.h"
#include "sys_windows.h"

#include "sys_prog_windows.h"

#include "sys_boot_none.h"

#include "sys_mount_none.h"

#include "sys_beep_windows.h"

#include "sys_ports_windows.h"

#ifdef __CYGWIN32__

#include "sys_exec_unix.h"

#ifdef ENABLE_SHARED_OBJECTS
#define SHARED_OBJECT_LOAD_FLAGS (RTLD_NOW | RTLD_GLOBAL)
#include "sys_shlib_dlfcn.h"
#endif /* ENABLE_SHARED_OBJECTS */

#ifdef ENABLE_PCM_SUPPORT
#define PCM_OSS_DEVICE_PATH "/dev/dsp"
#include "sys_pcm_oss.h"
#endif /* ENABLE_PCM_SUPPORT */

#ifdef ENABLE_MIDI_SUPPORT
#define MIDI_OSS_DEVICE_PATH "/dev/sequencer"
#include "sys_midi_oss.h"
#endif /* ENABLE_MIDI_SUPPORT */

#else /* __CYGWIN32__ */

#include "sys_exec_windows.h"

#ifdef ENABLE_SHARED_OBJECTS
#include "sys_shlib_windows.h"
#endif /* ENABLE_SHARED_OBJECTS */

#ifdef ENABLE_PCM_SUPPORT
#include "sys_pcm_windows.h"
#endif /* ENABLE_PCM_SUPPORT */

#ifdef ENABLE_MIDI_SUPPORT
#include "sys_midi_windows.h"
#endif /* ENABLE_MIDI_SUPPORT */

#endif /* __CYGWIN32__ */


/* ntdll.dll */
WIN_PROC_STUB(NtSetInformationProcess);


/* kernel32.dll: console */
WIN_PROC_STUB(AttachConsole);


/* user32.dll */
WIN_PROC_STUB(GetAltTabInfoA);
WIN_PROC_STUB(SendInput);


#ifdef __MINGW32__
/* ws2_32.dll */
WIN_PROC_STUB(getaddrinfo);
WIN_PROC_STUB(freeaddrinfo);
#endif /* __MINGW32__ */


static void *
loadLibrary (const char *name) {
  HMODULE module = LoadLibrary(name);
  if (!module) LogPrint(LOG_DEBUG, "%s: %s", gettext("cannot load library"), name);
  return module;
}

static void *
getProcedure (HMODULE module, const char *name) {
  void *address = module? GetProcAddress(module, name): NULL;
  if (!address) LogPrint(LOG_DEBUG, "%s: %s", gettext("cannot find procedure"), name);
  return address;
}

static int
addWindowsCommandLineCharacter (char **buffer, int *size, int *length, char character) {
  if (*length == *size) {
    char *newBuffer = realloc(*buffer, (*size = *size? *size<<1: 0X80));
    if (!newBuffer) {
      LogError("realloc");
      return 0;
    }
    *buffer = newBuffer;
  }

  (*buffer)[(*length)++] = character;
  return 1;
}

char *
makeWindowsCommandLine (const char *const *arguments) {
  const char backslash = '\\';
  const char quote = '"';
  char *buffer = NULL;
  int size = 0;
  int length = 0;

#define ADD(c) if (!addWindowsCommandLineCharacter(&buffer, &size, &length, (c))) goto error
  while (*arguments) {
    const char *character = *arguments;
    int backslashCount = 0;
    int needQuotes = 0;
    int start = length;

    while (*character) {
      if (*character == backslash) {
        ++backslashCount;
      } else {
        if (*character == quote) {
          needQuotes = 1;
          backslashCount = (backslashCount * 2) + 1;
        } else if ((*character == ' ') || (*character == '\t')) {
          needQuotes = 1;
        }

        while (backslashCount > 0) {
          ADD(backslash);
          --backslashCount;
        }

        ADD(*character);
      }

      ++character;
    }

    if (needQuotes) backslashCount *= 2;
    while (backslashCount > 0) {
      ADD(backslash);
      --backslashCount;
    }

    if (needQuotes) {
      ADD(quote);
      ADD(quote);
      memmove(&buffer[start+1], &buffer[start], length-start-1);
      buffer[start] = quote;
    }

    ADD(' ');
    ++arguments;
  }
#undef ADD

  buffer[length-1] = 0;
  {
    char *line = realloc(buffer, length);
    if (line) return line;
    LogError("realloc");
  }

error:
  if (buffer) free(buffer);
  return NULL;
}

int
installService (const char *name, const char *description) {
  int installed = 0;
  const char *const arguments[] = {
    getProgramPath(),
    NULL
  };
  char *command = makeWindowsCommandLine(arguments);

  if (command) {
    SC_HANDLE scm = OpenSCManager(NULL, NULL, SC_MANAGER_CREATE_SERVICE);

    if (scm) {
      SC_HANDLE service = CreateService(scm, name, description, SERVICE_ALL_ACCESS,
                                        SERVICE_WIN32_OWN_PROCESS | SERVICE_INTERACTIVE_PROCESS,
                                        SERVICE_AUTO_START, SERVICE_ERROR_NORMAL,
                                        command, NULL, NULL, NULL, NULL, NULL);

      if (service) {
        LogPrint(LOG_NOTICE, "service installed: %s", name);
        installed = 1;

        CloseServiceHandle(service);
      } else if (GetLastError() == ERROR_SERVICE_EXISTS) {
        LogPrint(LOG_WARNING, "service already installed: %s", name);
        installed = 1;
      } else {
        LogWindowsError("CreateService");
      }

      CloseServiceHandle(scm);
    } else {
      LogWindowsError("OpenSCManager");
    }

    free(command);
  }

  return installed;
}

int
removeService (const char *name) {
  int removed = 0;
  SC_HANDLE scm = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);

  if (scm) {
    SC_HANDLE service = OpenService(scm, name, DELETE);

    if (service) {
      if (DeleteService(service)) {
        LogPrint(LOG_NOTICE, "service removed: %s", name);
        removed = 1;
      } else if (GetLastError() == ERROR_SERVICE_MARKED_FOR_DELETE) {
        LogPrint(LOG_WARNING, "service already being removed: %s", name);
        removed = 1;
      } else {
        LogWindowsError("DeleteService");
      }

      CloseServiceHandle(service);
    } else if (GetLastError() == ERROR_SERVICE_DOES_NOT_EXIST) {
      LogPrint(LOG_WARNING, "service not installed: %s", name);
      removed = 1;
    } else {
      LogWindowsError("OpenService");
    }

    CloseServiceHandle(scm);
  } else {
    LogWindowsError("OpenSCManager");
  }

  return removed;
}

void
sysInit (void) {
  HMODULE library;

#define LOAD_LIBRARY(name) (library = loadLibrary(name))
#define GET_PROC(name) (name##Proc = getProcedure(library, #name))

  if (LOAD_LIBRARY("ntdll.dll")) {
    GET_PROC(NtSetInformationProcess);
  }

  if (LOAD_LIBRARY("kernel32.dll")) {
    GET_PROC(AttachConsole);
  }

  if (LOAD_LIBRARY("user32.dll")) {
    GET_PROC(GetAltTabInfoA);
    GET_PROC(SendInput);
  }

#ifdef __MINGW32__
  if (LOAD_LIBRARY("ws2_32.dll")) {
    GET_PROC(getaddrinfo);
    GET_PROC(freeaddrinfo);
  }
#endif /* __MINGW32__ */
}

#ifdef __MINGW32__
#include "win_errno.h"

#ifndef SUBLANG_ENGLISH_IRELAND
#define SUBLANG_ENGLISH_IRELAND SUBLANG_ENGLISH_EIRE
#endif /* SUBLANG_ENGLISH_IRELAND */

#ifndef SUBLANG_LITHUANIAN_LITHUANIA
#define SUBLANG_LITHUANIAN_LITHUANIA SUBLANG_LITHUANIAN
#endif /* SUBLANG_LITHUANIAN_LITHUANIA */

#ifndef SUBLANG_PORTUGUESE_PORTUGAL
#define SUBLANG_PORTUGUESE_PORTUGAL SUBLANG_PORTUGUESE
#endif /* SUBLANG_PORTUGUESE_PORTUGAL */

#ifndef SUBLANG_SWEDISH_SWEDEN
#define SUBLANG_SWEDISH_SWEDEN SUBLANG_SWEDISH
#endif /* SUBLANG_SWEDISH_SWEDEN */

const char *
win_getLocaleName (void) {
  DWORD language;
  int result = GetLocaleInfo(GetThreadLocale(),
                             LOCALE_ILANGUAGE | LOCALE_RETURN_NUMBER,
                             (char *)&language, sizeof(language)/sizeof(TCHAR));

  if (result > 0) {
#define TERRITORY(l,t,s) case MAKELANGID(LANG_##l, SUBLANG_##l##_##t): return (s);
    switch (language) {
      TERRITORY(AFRIKAANS, SOUTH_AFRICA, "af_ZA");
      TERRITORY(ALBANIAN, ALBANIA, "sq_AL");
      TERRITORY(ALSATIAN, FRANCE, "xx_FR");
      TERRITORY(AMHARIC, ETHIOPIA, "am_ET");
      TERRITORY(ARABIC, ALGERIA, "ar_DZ");
      TERRITORY(ARABIC, BAHRAIN, "ar_BH");
      TERRITORY(ARABIC, EGYPT, "ar_EG");
      TERRITORY(ARABIC, IRAQ, "ar_IQ");
      TERRITORY(ARABIC, JORDAN, "ar_JO");
      TERRITORY(ARABIC, KUWAIT, "ar_QW");
      TERRITORY(ARABIC, LEBANON, "ar_LB");
      TERRITORY(ARABIC, LIBYA, "ar_LY");
      TERRITORY(ARABIC, MOROCCO, "ar_MA");
      TERRITORY(ARABIC, OMAN, "ar_OM");
      TERRITORY(ARABIC, QATAR, "ar_QA");
      TERRITORY(ARABIC, SAUDI_ARABIA, "ar_SA");
      TERRITORY(ARABIC, SYRIA, "ar_SY");
      TERRITORY(ARABIC, TUNISIA, "ar_TN");
      TERRITORY(ARABIC, UAE, "ar_AE");
      TERRITORY(ARABIC, YEMEN, "ar_YE");
      TERRITORY(ARMENIAN, ARMENIA, "hy_AM");
      TERRITORY(ASSAMESE, INDIA, "as_IN");
      TERRITORY(AZERI, CYRILLIC, "xx_XX");
      TERRITORY(AZERI, LATIN, "xx_XX");
      TERRITORY(BASHKIR, RUSSIA, "ba_RU");
      TERRITORY(BASQUE, BASQUE, "eu_XX");
      TERRITORY(BELARUSIAN, BELARUS, "be_BY");
      TERRITORY(BENGALI, BANGLADESH, "bn_HD");
      TERRITORY(BENGALI, INDIA, "bn_IN");
      TERRITORY(BOSNIAN, BOSNIA_HERZEGOVINA_CYRILLIC, "bs_BA");
      TERRITORY(BOSNIAN, BOSNIA_HERZEGOVINA_LATIN, "bs_BA");
      TERRITORY(BRETON, FRANCE, "br_FR");
      TERRITORY(BULGARIAN, BULGARIA, "bg_BG");
      TERRITORY(CATALAN, CATALAN, "ca_XX");
      TERRITORY(CHINESE, HONGKONG, "zh_HK");
      TERRITORY(CHINESE, MACAU, "zh_MO");
      TERRITORY(CHINESE, SIMPLIFIED, "zh_XX");
      TERRITORY(CHINESE, SINGAPORE, "zh_SG");
      TERRITORY(CHINESE, TRADITIONAL, "zh_XX");
      TERRITORY(CORSICAN, FRANCE, "co_FR");
      TERRITORY(CROATIAN, BOSNIA_HERZEGOVINA_LATIN, "hr_BA");
      TERRITORY(CROATIAN, CROATIA, "hr_HR");
      TERRITORY(CZECH, CZECH_REPUBLIC, "cs_CZ");
      TERRITORY(DANISH, DENMARK, "da_DK");
      TERRITORY(DIVEHI, MALDIVES, "dv_MV");
      TERRITORY(DUTCH, BELGIAN, "nl_BE");
      TERRITORY(ENGLISH, AUS, "en_AU");
      TERRITORY(ENGLISH, BELIZE, "en_BZ");
      TERRITORY(ENGLISH, CAN, "en_CA");
      TERRITORY(ENGLISH, CARIBBEAN, "en_XX");
      TERRITORY(ENGLISH, IRELAND, "en_IE");
      TERRITORY(ENGLISH, INDIA, "en_IN");
      TERRITORY(ENGLISH, JAMAICA, "en_JM");
      TERRITORY(ENGLISH, MALAYSIA, "en_MY");
      TERRITORY(ENGLISH, NZ, "en_NZ");
      TERRITORY(ENGLISH, PHILIPPINES, "en_PH");
      TERRITORY(ENGLISH, SINGAPORE, "en_SG");
      TERRITORY(ENGLISH, SOUTH_AFRICA, "en_ZA");
      TERRITORY(ENGLISH, TRINIDAD, "en_TT");
      TERRITORY(ENGLISH, UK, "en_GB");
      TERRITORY(ENGLISH, US, "en_US");
      TERRITORY(ENGLISH, ZIMBABWE, "en_ZW");
      TERRITORY(ESTONIAN, ESTONIA, "et_EE");
      TERRITORY(FAEROESE, FAROE_ISLANDS, "fo_FO");
      TERRITORY(FILIPINO, PHILIPPINES, "xx_PH");
      TERRITORY(FINNISH, FINLAND, "fi_FI");
      TERRITORY(FRENCH, BELGIAN, "fr_BE");
      TERRITORY(FRENCH, CANADIAN, "fr_CA");
      TERRITORY(FRENCH, LUXEMBOURG, "fr_LU");
      TERRITORY(FRENCH, MONACO, "fr_MC");
      TERRITORY(FRENCH, SWISS, "fr_CH");
      TERRITORY(FRISIAN, NETHERLANDS, "fy_NL");
      TERRITORY(GALICIAN, GALICIAN, "gl_XX");
      TERRITORY(GEORGIAN, GEORGIA, "ka_GE");
      TERRITORY(GERMAN, AUSTRIAN, "de_AT");
      TERRITORY(GERMAN, LIECHTENSTEIN, "de_LI");
      TERRITORY(GERMAN, LUXEMBOURG, "de_LU");
      TERRITORY(GERMAN, SWISS, "de_CH");
      TERRITORY(GREEK, GREECE, "el_GR");
      TERRITORY(GREENLANDIC, GREENLAND, "kl_GL");
      TERRITORY(GUJARATI, INDIA, "gu_IN");
      TERRITORY(HAUSA, NIGERIA, "ha_NG");
      TERRITORY(HEBREW, ISRAEL, "he_IL");
      TERRITORY(HINDI, INDIA, "hi_IN");
      TERRITORY(HUNGARIAN, HUNGARY, "hu_HU");
      TERRITORY(ICELANDIC, ICELAND, "is_IS");
      TERRITORY(IGBO, NIGERIA, "ig_NG");
      TERRITORY(INDONESIAN, INDONESIA, "id_ID");
      TERRITORY(INUKTITUT, CANADA, "iu_CA");
      TERRITORY(IRISH, IRELAND, "ga_IE");
      TERRITORY(ITALIAN, SWISS, "it_CH");
      TERRITORY(JAPANESE, JAPAN, "ja_JP");
      TERRITORY(KASHMIRI, INDIA, "ks_IN");
      TERRITORY(KAZAK, KAZAKHSTAN, "kk_KZ");
      TERRITORY(KHMER, CAMBODIA, "km_KH");
      TERRITORY(KICHE, GUATEMALA, "xx_GT");
      TERRITORY(KINYARWANDA, RWANDA, "rw_RW");
      TERRITORY(KONKANI, INDIA, "xx_IN");
      TERRITORY(KYRGYZ, KYRGYZSTAN, "ky_KG");
      TERRITORY(LAO, LAO_PDR, "lo_LA");
      TERRITORY(LATVIAN, LATVIA, "lv_LV");
      TERRITORY(LITHUANIAN, LITHUANIA, "lt_LT");
      TERRITORY(LOWER_SORBIAN, GERMANY, "xx_DE");
      TERRITORY(LUXEMBOURGISH, LUXEMBOURG, "lb_LU");
      TERRITORY(MACEDONIAN, MACEDONIA, "mk_MK");
      TERRITORY(MALAY, BRUNEI_DARUSSALAM, "ms_BN");
      TERRITORY(MALAY, MALAYSIA, "ms_MY");
      TERRITORY(MALAYALAM, INDIA, "ml_IN");
      TERRITORY(MALTESE, MALTA, "mt_MT");
      TERRITORY(MAORI, NEW_ZEALAND, "mi_NZ");
      TERRITORY(MAPUDUNGUN, CHILE, "xx_CL");
      TERRITORY(MARATHI, INDIA, "mr_IN");
      TERRITORY(MOHAWK, MOHAWK, "xx_XX");
      TERRITORY(MONGOLIAN, CYRILLIC_MONGOLIA, "mn_MN");
      TERRITORY(MONGOLIAN, PRC, "mn_XX");
      TERRITORY(NEPALI, INDIA, "ne_IN");
      TERRITORY(NEPALI, NEPAL, "ne_NP");
      TERRITORY(NORWEGIAN, BOKMAL, "no_XX");
      TERRITORY(NORWEGIAN, NYNORSK, "no_XX");
      TERRITORY(OCCITAN, FRANCE, "oc_FR");
      TERRITORY(ORIYA, INDIA, "or_IN");
      TERRITORY(PASHTO, AFGHANISTAN, "ps_AF");
      TERRITORY(PERSIAN, IRAN, "fa_IR");
      TERRITORY(POLISH, POLAND, "pl_PL");
      TERRITORY(PORTUGUESE, BRAZILIAN, "pt_BR");
      TERRITORY(PORTUGUESE, PORTUGAL, "pt_PT");
      TERRITORY(PUNJABI, INDIA, "pa_IN");
      TERRITORY(PUNJABI, PAKISTAN, "pa_PK");
      TERRITORY(QUECHUA, BOLIVIA, "qu_BO");
      TERRITORY(QUECHUA, ECUADOR, "qu_EC");
      TERRITORY(QUECHUA, PERU, "qu_PE");
      TERRITORY(ROMANIAN, MOLDOVA, "ro_MD");
      TERRITORY(ROMANIAN, ROMANIA, "ro_RO");
      TERRITORY(RUSSIAN, RUSSIA, "ru_RU");
      TERRITORY(SAMI, LULE_NORWAY, "se_XX");
      TERRITORY(SAMI, LULE_SWEDEN, "se_XX");
      TERRITORY(SAMI, NORTHERN_FINLAND, "se_XX");
      TERRITORY(SAMI, NORTHERN_NORWAY, "se_XX");
      TERRITORY(SAMI, NORTHERN_SWEDEN, "se_XX");
      TERRITORY(SAMI, SOUTHERN_NORWAY, "se_XX");
      TERRITORY(SAMI, SOUTHERN_SWEDEN, "se_XX");
      TERRITORY(SANSKRIT, INDIA, "sa_IN");
      TERRITORY(SERBIAN, BOSNIA_HERZEGOVINA_CYRILLIC, "sr_BA");
      TERRITORY(SERBIAN, BOSNIA_HERZEGOVINA_LATIN, "sr_BA");
      TERRITORY(SERBIAN, CYRILLIC, "sr_XX");
      TERRITORY(SERBIAN, LATIN, "sr_XX");
      TERRITORY(SINDHI, AFGHANISTAN, "sd_AF");
      TERRITORY(SINHALESE, SRI_LANKA, "si_LK");
      TERRITORY(SLOVAK, SLOVAKIA, "sk_SK");
      TERRITORY(SLOVENIAN, SLOVENIA, "sl_SI");
      TERRITORY(SOTHO, NORTHERN_SOUTH_AFRICA, "st_XX");
      TERRITORY(SPANISH, ARGENTINA, "es_AR");
      TERRITORY(SPANISH, BOLIVIA, "es_BO");
      TERRITORY(SPANISH, CHILE, "es_CL");
      TERRITORY(SPANISH, COLOMBIA, "es_CO");
      TERRITORY(SPANISH, COSTA_RICA, "es_CR");
      TERRITORY(SPANISH, DOMINICAN_REPUBLIC, "es_DO");
      TERRITORY(SPANISH, ECUADOR, "es_EC");
      TERRITORY(SPANISH, EL_SALVADOR, "es_SV");
      TERRITORY(SPANISH, GUATEMALA, "es_GT");
      TERRITORY(SPANISH, HONDURAS, "es_HN");
      TERRITORY(SPANISH, MEXICAN, "es_MX");
      TERRITORY(SPANISH, MODERN, "es_XX");
      TERRITORY(SPANISH, NICARAGUA, "es_NI");
      TERRITORY(SPANISH, PANAMA, "es_PA");
      TERRITORY(SPANISH, PARAGUAY, "es_PY");
      TERRITORY(SPANISH, PERU, "es_PE");
      TERRITORY(SPANISH, PUERTO_RICO, "es_PR");
      TERRITORY(SPANISH, URUGUAY, "es_UY");
      TERRITORY(SPANISH, US, "es_US");
      TERRITORY(SPANISH, VENEZUELA, "es_VE");
      TERRITORY(SWEDISH, FINLAND, "sv_FI");
      TERRITORY(SWEDISH, SWEDEN, "sv_SE");
      TERRITORY(TAMAZIGHT, ALGERIA_LATIN, "xx_DZ");
      TERRITORY(TAMIL, INDIA, "ta_IN");
      TERRITORY(TATAR, RUSSIA, "tt_RU");
      TERRITORY(TELUGU, INDIA, "te_IN");
      TERRITORY(THAI, THAILAND, "th_TH");
      TERRITORY(TIBETAN, BHUTAN, "bo_BT");
      TERRITORY(TIBETAN, PRC, "bo_XX");
      TERRITORY(TIGRIGNA, ERITREA, "ti_ER");
      TERRITORY(TSWANA, SOUTH_AFRICA, "tn_ZA");
      TERRITORY(TURKISH, TURKEY, "tr_TR");
      TERRITORY(UIGHUR, PRC, "ug_XX");
      TERRITORY(UKRAINIAN, UKRAINE, "uk_UA");
    //TERRITORY(UPPER_SORBIAN, GERMANY, "xx_DE");
      TERRITORY(URDU, INDIA, "ur_IN");
      TERRITORY(URDU, PAKISTAN, "ur_PK");
      TERRITORY(UZBEK, CYRILLIC, "uz_XX");
      TERRITORY(UZBEK, LATIN, "uz_XX");
      TERRITORY(VIETNAMESE, VIETNAM, "vi_VN");
      TERRITORY(WELSH, UNITED_KINGDOM, "cy_GB");
      TERRITORY(WOLOF, SENEGAL, "fy_SN");
      TERRITORY(XHOSA, SOUTH_AFRICA, "xh_ZA");
      TERRITORY(YAKUT, RUSSIA, "xx_RU");
      TERRITORY(YI, PRC, "yi_XX");
      TERRITORY(YORUBA, NIGERIA, "yo_NG");
      TERRITORY(ZULU, SOUTH_AFRICA, "zu_ZA");
    }
#undef TERRITORY

#define LANGUAGE(l,s) case LANG_##l: return (s);
    switch (PRIMARYLANGID(language)) {
      LANGUAGE(AFRIKAANS, "af");
      LANGUAGE(ALBANIAN, "sq");
      LANGUAGE(ALSATIAN, "xx");
      LANGUAGE(AMHARIC, "am");
      LANGUAGE(ARABIC, "ar");
      LANGUAGE(ARMENIAN, "hy");
      LANGUAGE(ASSAMESE, "as");
      LANGUAGE(AZERI, "xx");
      LANGUAGE(BASHKIR, "ba");
      LANGUAGE(BASQUE, "eu");
      LANGUAGE(BELARUSIAN, "be");
      LANGUAGE(BENGALI, "bn");
      LANGUAGE(BOSNIAN, "bs");
      LANGUAGE(BOSNIAN_NEUTRAL, "bs");
      LANGUAGE(BRETON, "br");
      LANGUAGE(BULGARIAN, "bg");
      LANGUAGE(CATALAN, "ca");
      LANGUAGE(CHINESE, "zh");
      LANGUAGE(CORSICAN, "co");
    //LANGUAGE(CROATIAN, "hr");
      LANGUAGE(CZECH, "cs");
      LANGUAGE(DANISH, "da");
      LANGUAGE(DARI, "xx");
      LANGUAGE(DIVEHI, "dv");
      LANGUAGE(DUTCH, "nl");
      LANGUAGE(ENGLISH, "en");
      LANGUAGE(ESTONIAN, "et");
      LANGUAGE(FAEROESE, "fo");
      LANGUAGE(FILIPINO, "xx");
      LANGUAGE(FINNISH, "fi");
      LANGUAGE(FRENCH, "fr");
      LANGUAGE(FRISIAN, "fy");
      LANGUAGE(GALICIAN, "gl");
      LANGUAGE(GEORGIAN, "ka");
      LANGUAGE(GERMAN, "de");
      LANGUAGE(GREEK, "el");
      LANGUAGE(GREENLANDIC, "kl");
      LANGUAGE(GUJARATI, "gu");
      LANGUAGE(HAUSA, "ha");
      LANGUAGE(HEBREW, "he");
      LANGUAGE(HINDI, "hi");
      LANGUAGE(HUNGARIAN, "hu");
      LANGUAGE(ICELANDIC, "is");
      LANGUAGE(IGBO, "ig");
      LANGUAGE(INDONESIAN, "id");
      LANGUAGE(INUKTITUT, "iu");
      LANGUAGE(IRISH, "ga");
      LANGUAGE(ITALIAN, "it");
      LANGUAGE(JAPANESE, "ja");
      LANGUAGE(KANNADA, "kn");
      LANGUAGE(KASHMIRI, "ks");
      LANGUAGE(KAZAK, "kk");
      LANGUAGE(KHMER, "km");
      LANGUAGE(KICHE, "xx");
      LANGUAGE(KINYARWANDA, "rw");
      LANGUAGE(KONKANI, "xx");
      LANGUAGE(KOREAN, "ko");
      LANGUAGE(KYRGYZ, "ky");
      LANGUAGE(LAO, "lo");
      LANGUAGE(LATVIAN, "lv");
      LANGUAGE(LITHUANIAN, "lt");
      LANGUAGE(LOWER_SORBIAN, "xx");
      LANGUAGE(LUXEMBOURGISH, "lb");
      LANGUAGE(MACEDONIAN, "mk");
      LANGUAGE(MALAGASY, "mg");
      LANGUAGE(MALAY, "ms");
      LANGUAGE(MALAYALAM, "ml");
      LANGUAGE(MALTESE, "mt");
      LANGUAGE(MANIPURI, "xx");
      LANGUAGE(MAORI, "mi");
      LANGUAGE(MAPUDUNGUN, "xx");
      LANGUAGE(MARATHI, "mr");
      LANGUAGE(MOHAWK, "xx");
      LANGUAGE(MONGOLIAN, "mn");
      LANGUAGE(NEPALI, "ne");
      LANGUAGE(NORWEGIAN, "no");
      LANGUAGE(OCCITAN, "oc");
      LANGUAGE(ORIYA, "or");
      LANGUAGE(PASHTO, "ps");
      LANGUAGE(PERSIAN, "fa");
      LANGUAGE(POLISH, "pl");
      LANGUAGE(PORTUGUESE, "pt");
      LANGUAGE(PUNJABI, "pa");
      LANGUAGE(QUECHUA, "qu");
      LANGUAGE(ROMANIAN, "ro");
      LANGUAGE(RUSSIAN, "ru");
      LANGUAGE(SAMI, "se");
      LANGUAGE(SANSKRIT, "sa");
    //LANGUAGE(SERBIAN, "sr");
      LANGUAGE(SERBIAN_NEUTRAL, "sr");
      LANGUAGE(SINDHI, "sd");
      LANGUAGE(SINHALESE, "si");
      LANGUAGE(SLOVAK, "sk");
      LANGUAGE(SLOVENIAN, "sl");
      LANGUAGE(SOTHO, "st");
      LANGUAGE(SPANISH, "es");
      LANGUAGE(SWAHILI, "sw");
      LANGUAGE(SWEDISH, "sv");
      LANGUAGE(SYRIAC, "xx");
      LANGUAGE(TAMAZIGHT, "xx");
      LANGUAGE(TAMIL, "ta");
      LANGUAGE(TATAR, "tt");
      LANGUAGE(TELUGU, "te");
      LANGUAGE(THAI, "th");
      LANGUAGE(TIBETAN, "bo");
      LANGUAGE(TIGRIGNA, "ti");
      LANGUAGE(TSWANA, "tn");
      LANGUAGE(TURKISH, "tr");
      LANGUAGE(UIGHUR, "ug");
      LANGUAGE(UKRAINIAN, "uk");
    //LANGUAGE(UPPER_SORBIAN, "xx");
      LANGUAGE(URDU, "ur");
      LANGUAGE(UZBEK, "uz");
      LANGUAGE(VIETNAMESE, "vi");
      LANGUAGE(WELSH, "cy");
      LANGUAGE(WOLOF, "fy");
      LANGUAGE(XHOSA, "xh");
      LANGUAGE(YAKUT, "xx");
      LANGUAGE(YI, "yi");
      LANGUAGE(YORUBA, "yo");
      LANGUAGE(ZULU, "zu");
    }
#undef LANGUAGE
  }

  return "xx";
}

#if (__MINGW32_MAJOR_VERSION < 3) || ((__MINGW32_MAJOR_VERSION == 3) && (__MINGW32_MINOR_VERSION < 10))
int
gettimeofday (struct timeval *tvp, void *tzp) {
  DWORD time = GetTickCount();
  /* this is not 49.7 days-proof ! */
  tvp->tv_sec = time / 1000;
  tvp->tv_usec = (time % 1000) * 1000;
  return 0;
}
#endif /* gettimeofday() */

#if (__MINGW32_MAJOR_VERSION < 3) || ((__MINGW32_MAJOR_VERSION == 3) && (__MINGW32_MINOR_VERSION < 15))
void
usleep (int usec) {
  if (usec > 0) {
    approximateDelay((usec+999)/1000);
  }
}
#endif /* usleep() */
#endif /* __MINGW32__ */
