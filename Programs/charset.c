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

#include "prologue.h"

#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <ctype.h>
#include <errno.h>
#include <locale.h>

#if defined(WINDOWS)

#elif defined(__MSDOS__)
#include "sys_msdos.h"

#else /* Unix */
#include <langinfo.h>
#endif /* platform-specific includes */

#include "misc.h"
#include "lock.h"
#include "charset.h"

static char *currentCharset = NULL;

#if defined(HAVE_ICONV_H)
#include <iconv.h>

#define CHARSET_ICONV_NULL ((iconv_t)-1)
#define CHARSET_ICONV_HANDLE(name) iconv_t iconv##name = CHARSET_ICONV_NULL

static CHARSET_ICONV_HANDLE(CharToWchar);
static CHARSET_ICONV_HANDLE(WcharToChar);

#define CHARSET_CONVERT_TYPE_TO_TYPE(name, from, to, ret, eof) \
ret convert##name (from f) { \
  if (getCharset()) { \
    from *fp = &f; \
    size_t fs = sizeof(f); \
    to t; \
    to *tp = &t; \
    size_t ts = sizeof(t); \
    if (iconv(iconv##name, (void *)&fp, &fs, (void *)&tp, &ts) != -1) return t; \
    LogPrint(LOG_DEBUG, "iconv (" #from " -> " #to ") error: %s", strerror(errno)); \
  } \
  return eof; \
}
CHARSET_CONVERT_TYPE_TO_TYPE(CharToWchar, char, wchar_t, wint_t, WEOF)
CHARSET_CONVERT_TYPE_TO_TYPE(WcharToChar, wchar_t, unsigned char, int, EOF)
#undef CHARSET_CONVERT_TYPE_TO_TYPE

#elif defined(__MINGW32__)

#if defined(CP_THREAD_ACP)
#define CHARSET_WINDOWS_CODEPAGE CP_THREAD_ACP
#else /* Windows codepage */
#define CHARSET_WINDOWS_CODEPAGE CP_ACP
#endif /* Windows codepage */

wint_t
convertCharToWchar (char c) {
  wchar_t wc;
  int result = MultiByteToWideChar(CHARSET_WINDOWS_CODEPAGE, MB_ERR_INVALID_CHARS,
                                   &c, 1,  &wc, 1);
  if (result) return wc;
  LogWindowsError("MultiByteToWideChar[" STRINGIFY(CHARSET_WINDOWS_CODEPAGE) "]");
  return WEOF;
}

int
convertWcharToChar (wchar_t wc) {
  unsigned char c;
  int result = WideCharToMultiByte(CHARSET_WINDOWS_CODEPAGE, WC_NO_BEST_FIT_CHARS /* WC_ERR_INVALID_CHARS */,
                                   &wc, 1, &c, 1,
                                   NULL, NULL);
  if (result) return c;
  LogWindowsError("WideCharToMultiByte[" STRINGIFY(CHARSET_WINDOWS_CODEPAGE) "]");
  return EOF;
}

#else /* conversions */

#define CHARACTER_SET_SIZE 0X100

static const uint16_t toUnicode_CP437[CHARACTER_SET_SIZE] = {
  /* 00 */ 0X0000, 0X0001, 0X0002, 0X0003, 0X0004, 0X0005, 0X0006, 0X0007,
  /* 08 */ 0X0008, 0X0009, 0X000A, 0X000B, 0X000C, 0X000D, 0X000E, 0X000F,
  /* 10 */ 0X0010, 0X0011, 0X0012, 0X0013, 0X0014, 0X0015, 0X0016, 0X0017,
  /* 18 */ 0X0018, 0X0019, 0X001A, 0X001B, 0X001C, 0X001D, 0X001E, 0X001F,
  /* 20 */ 0X0020, 0X0021, 0X0022, 0X0023, 0X0024, 0X0025, 0X0026, 0X0027,
  /* 28 */ 0X0028, 0X0029, 0X002A, 0X002B, 0X002C, 0X002D, 0X002E, 0X002F,
  /* 30 */ 0X0030, 0X0031, 0X0032, 0X0033, 0X0034, 0X0035, 0X0036, 0X0037,
  /* 38 */ 0X0038, 0X0039, 0X003A, 0X003B, 0X003C, 0X003D, 0X003E, 0X003F,
  /* 40 */ 0X0040, 0X0041, 0X0042, 0X0043, 0X0044, 0X0045, 0X0046, 0X0047,
  /* 48 */ 0X0048, 0X0049, 0X004A, 0X004B, 0X004C, 0X004D, 0X004E, 0X004F,
  /* 50 */ 0X0050, 0X0051, 0X0052, 0X0053, 0X0054, 0X0055, 0X0056, 0X0057,
  /* 58 */ 0X0058, 0X0059, 0X005A, 0X005B, 0X005C, 0X005D, 0X005E, 0X005F,
  /* 60 */ 0X0060, 0X0061, 0X0062, 0X0063, 0X0064, 0X0065, 0X0066, 0X0067,
  /* 68 */ 0X0068, 0X0069, 0X006A, 0X006B, 0X006C, 0X006D, 0X006E, 0X006F,
  /* 70 */ 0X0070, 0X0071, 0X0072, 0X0073, 0X0074, 0X0075, 0X0076, 0X0077,
  /* 78 */ 0X0078, 0X0079, 0X007A, 0X007B, 0X007C, 0X007D, 0X007E, 0X007F,
  /* 80 */ 0X00C7, 0X00FC, 0X00E9, 0X00E2, 0X00E4, 0X00E0, 0X00E5, 0X00E7,
  /* 88 */ 0X00EA, 0X00EB, 0X00E8, 0X00EF, 0X00EE, 0X00EC, 0X00C4, 0X00C5,
  /* 90 */ 0X00C9, 0X00E6, 0X00C6, 0X00F4, 0X00F6, 0X00F2, 0X00FB, 0X00F9,
  /* 98 */ 0X00FF, 0X00D6, 0X00DC, 0X00A2, 0X00A3, 0X00A5, 0X20A7, 0X0192,
  /* A0 */ 0X00E1, 0X00ED, 0X00F3, 0X00FA, 0X00F1, 0X00D1, 0X00AA, 0X00BA,
  /* A8 */ 0X00BF, 0X2310, 0X00AC, 0X00BD, 0X00BC, 0X00A1, 0X00AB, 0X00BB,
  /* B0 */ 0X2591, 0X2592, 0X2593, 0X2502, 0X2524, 0X2561, 0X2562, 0X2556,
  /* B8 */ 0X2555, 0X2563, 0X2551, 0X2557, 0X255D, 0X255C, 0X255B, 0X2510,
  /* C0 */ 0X2514, 0X2534, 0X252C, 0X251C, 0X2500, 0X253C, 0X255E, 0X255F,
  /* C8 */ 0X255A, 0X2554, 0X2569, 0X2566, 0X2560, 0X2550, 0X256C, 0X2567,
  /* D0 */ 0X2568, 0X2564, 0X2565, 0X2559, 0X2558, 0X2552, 0X2553, 0X256B,
  /* D8 */ 0X256A, 0X2518, 0X250C, 0X2588, 0X2584, 0X258C, 0X2590, 0X2580,
  /* E0 */ 0X03B1, 0X00DF, 0X0393, 0X03C0, 0X03A3, 0X03C3, 0X00B5, 0X03C4,
  /* E8 */ 0X03A6, 0X0398, 0X03A9, 0X03B4, 0X221E, 0X03C6, 0X03B5, 0X2229,
  /* F0 */ 0X2261, 0X00B1, 0X2265, 0X2264, 0X2320, 0X2321, 0X00F7, 0X2248,
  /* F8 */ 0X00B0, 0X2219, 0X00B7, 0X221A, 0X207F, 0X00B2, 0X25A0, 0X00A0
};

static const uint16_t toUnicode_CP850[CHARACTER_SET_SIZE] = {
  /* 00 */ 0X0000, 0X0001, 0X0002, 0X0003, 0X0004, 0X0005, 0X0006, 0X0007,
  /* 08 */ 0X0008, 0X0009, 0X000A, 0X000B, 0X000C, 0X000D, 0X000E, 0X000F,
  /* 10 */ 0X0010, 0X0011, 0X0012, 0X0013, 0X0014, 0X0015, 0X0016, 0X0017,
  /* 18 */ 0X0018, 0X0019, 0X001A, 0X001B, 0X001C, 0X001D, 0X001E, 0X001F,
  /* 20 */ 0X0020, 0X0021, 0X0022, 0X0023, 0X0024, 0X0025, 0X0026, 0X0027,
  /* 28 */ 0X0028, 0X0029, 0X002A, 0X002B, 0X002C, 0X002D, 0X002E, 0X002F,
  /* 30 */ 0X0030, 0X0031, 0X0032, 0X0033, 0X0034, 0X0035, 0X0036, 0X0037,
  /* 38 */ 0X0038, 0X0039, 0X003A, 0X003B, 0X003C, 0X003D, 0X003E, 0X003F,
  /* 40 */ 0X0040, 0X0041, 0X0042, 0X0043, 0X0044, 0X0045, 0X0046, 0X0047,
  /* 48 */ 0X0048, 0X0049, 0X004A, 0X004B, 0X004C, 0X004D, 0X004E, 0X004F,
  /* 50 */ 0X0050, 0X0051, 0X0052, 0X0053, 0X0054, 0X0055, 0X0056, 0X0057,
  /* 58 */ 0X0058, 0X0059, 0X005A, 0X005B, 0X005C, 0X005D, 0X005E, 0X005F,
  /* 60 */ 0X0060, 0X0061, 0X0062, 0X0063, 0X0064, 0X0065, 0X0066, 0X0067,
  /* 68 */ 0X0068, 0X0069, 0X006A, 0X006B, 0X006C, 0X006D, 0X006E, 0X006F,
  /* 70 */ 0X0070, 0X0071, 0X0072, 0X0073, 0X0074, 0X0075, 0X0076, 0X0077,
  /* 78 */ 0X0078, 0X0079, 0X007A, 0X007B, 0X007C, 0X007D, 0X007E, 0X007F,
  /* 80 */ 0X00C7, 0X00FC, 0X00E9, 0X00E2, 0X00E4, 0X00E0, 0X00E5, 0X00E7,
  /* 88 */ 0X00EA, 0X00EB, 0X00E8, 0X00EF, 0X00EE, 0X00EC, 0X00C4, 0X00C5,
  /* 90 */ 0X00C9, 0X00E6, 0X00C6, 0X00F4, 0X00F6, 0X00F2, 0X00FB, 0X00F9,
  /* 98 */ 0X00FF, 0X00D6, 0X00DC, 0X00F8, 0X00A3, 0X00D8, 0X00D7, 0X0192,
  /* A0 */ 0X00E1, 0X00ED, 0X00F3, 0X00FA, 0X00F1, 0X00D1, 0X00AA, 0X00BA,
  /* A8 */ 0X00BF, 0X00AE, 0X00AC, 0X00BD, 0X00BC, 0X00A1, 0X00AB, 0X00BB,
  /* B0 */ 0X2591, 0X2592, 0X2593, 0X2502, 0X2524, 0X00C1, 0X00C2, 0X00C0,
  /* B8 */ 0X00A9, 0X2563, 0X2551, 0X2557, 0X255D, 0X00A2, 0X00A5, 0X2510,
  /* C0 */ 0X2514, 0X2534, 0X252C, 0X251C, 0X2500, 0X253C, 0X00E3, 0X00C3,
  /* C8 */ 0X255A, 0X2554, 0X2569, 0X2566, 0X2560, 0X2550, 0X256C, 0X00A4,
  /* D0 */ 0X00F0, 0X00D0, 0X00CA, 0X00CB, 0X00C8, 0X0131, 0X00CD, 0X00CE,
  /* D8 */ 0X00CF, 0X2518, 0X250C, 0X2588, 0X2584, 0X00A6, 0X00CC, 0X2580,
  /* E0 */ 0X00D3, 0X00DF, 0X00D4, 0X00D2, 0X00F5, 0X00D5, 0X00B5, 0X00FE,
  /* E8 */ 0X00DE, 0X00DA, 0X00DB, 0X00D9, 0X00FD, 0X00DD, 0X00AF, 0X00B4,
  /* F0 */ 0X00AD, 0X00B1, 0X2017, 0X00BE, 0X00B6, 0X00A7, 0X00F7, 0X00B8,
  /* F8 */ 0X00B0, 0X00A8, 0X00B7, 0X00B9, 0X00B3, 0X00B2, 0X25A0, 0X00A0
};

typedef struct {
  const char *name;
  const uint16_t *toUnicode;
} CharacterSet;

static const CharacterSet characterSets[] = {
  {.name="cp437", .toUnicode=toUnicode_CP437},
  {.name="cp850", .toUnicode=toUnicode_CP850},
  {}
};
static const CharacterSet *characterSet;

#define UNICODE_ROWS_PER_PLANE 0X100
#define UNICODE_CELLS_PER_ROW 0X100

static char *unicodeRows[UNICODE_ROWS_PER_PLANE];

static void
clearUnicodeRow (char *row) {
  memset(row, 0, UNICODE_CELLS_PER_ROW);
}

static void
clearUnicodeRows (void) {
  int rowNumber;
  for (rowNumber=0; rowNumber<UNICODE_ROWS_PER_PLANE; ++rowNumber) {
    char *row = unicodeRows[rowNumber];
    if (row) clearUnicodeRow(row);
  }
}

static char *
getUnicodeCell (uint16_t unicode, int allocate) {
  unsigned char cellNumber = unicode & 0XFF;
  unsigned char rowNumber = unicode >> 8;
  char **rowAddress = &unicodeRows[rowNumber];

  if (!*rowAddress) {
    if (!allocate) return NULL;

    {
      char *row = malloc(UNICODE_CELLS_PER_ROW);
      if (!row) return NULL;
      clearUnicodeRow(row);
      *rowAddress = row;
    }
  }

  return &(*rowAddress)[cellNumber];
}

static int
setCharacterSet (const char *name) {
  const CharacterSet *set = characterSets;

  while (set->name) {
    if (strcasecmp(name, set->name) == 0) {
      clearUnicodeRows();
      characterSet = NULL;

      {
        int character;
        for (character=CHARACTER_SET_SIZE-1; character>=0; --character) {
          char *cell = getUnicodeCell(set->toUnicode[character], 1);
          if (!cell) return 0;
          *cell = character;
        }
      }

      characterSet = set;
      return 1;
    }

    ++set;
  }

  return 0;
}

wint_t
convertCharToWchar (char c) {
  return characterSet->toUnicode[c & 0XFF];
}

int
convertWcharToChar (wchar_t wc) {
  char *cell = getUnicodeCell(wc, 0);
  if (!cell) return EOF;
  if (!*cell && wc) return EOF;
  return *cell & 0XFF;
}
#endif /* conversions */

size_t
convertWcharToUtf8 (wchar_t wc, Utf8Buffer utf8) {
  size_t utfs;

  if (!(wc & ~0X7F)) {
    *utf8 = wc;
    utfs = 1;
  } else {
    char buffer[MB_LEN_MAX];
    char *end = buffer + sizeof(buffer);
    char *byte = end;

    do {
      *--byte = (wc & 0X3F) | 0X80;
    } while (wc >>= 6);

    utfs = end - byte;
    if ((*byte & 0X7F) >= (1 << (7 - utfs))) {
      *--byte = 0;
      utfs++;
    }

    *byte |= ~((1 << (8 - utfs)) - 1);
    memcpy(utf8, byte, utfs);
  }

  utf8[utfs] = 0;
  return utfs;
}

wint_t
convertUtf8ToWchar (const char **utf8, size_t *utfs) {
  wint_t wc = WEOF;
  int state = 0;

  while (*utfs) {
    unsigned char byte = *(*utf8)++;
    (*utfs)--;

    if (!(byte & 0X80)) {
      if (wc != WEOF) goto truncated;
      wc = byte;
      break;
    }

    if (!(byte & 0X40)) {
      if (wc == WEOF) break;
      wc = (wc << 6) | (byte & 0X3F);
      if (!--state) break;
    } else {
      if (!(byte & 0X20)) {
        state = 1;
      } else if (!(byte & 0X10)) {
        state = 2;
      } else if (!(byte & 0X08)) {
        state = 3;
      } else if (!(byte & 0X04)) {
        state = 4;
      } else if (!(byte & 0X02)) {
        state = 5;
      } else {
        state = 0;
      }

      if (wc != WEOF) goto truncated;

      if (!state) {
        wc = WEOF;
        break;
      }

      wc = byte & ((1 << (6 - state)) - 1);
    }
  }

  while (*utfs) {
    if ((**utf8 & 0XC0) != 0X80) break;
    (*utf8)++, (*utfs)--;
    wc = WEOF;
  }

  return wc;

truncated:
  (*utf8)--, (*utfs)++;
  return WEOF;
}

size_t
convertCharToUtf8 (char c, Utf8Buffer utf8) {
  wint_t wc = convertCharToWchar(c);
  if (wc == WEOF) return 0;
  return convertWcharToUtf8(wc, utf8);
}

int
convertUtf8ToChar (const char **utf8, size_t *utfs) {
  wint_t wc = convertUtf8ToWchar(utf8, utfs);
  if (wc == WEOF) return EOF;
  return convertWcharToChar(wc);
}

static const char *
getLocaleCharset (void) {
  const char *locale = setlocale(LC_ALL, "");

  if (locale && (MB_CUR_MAX == 1) &&
      (strcmp(locale, "C") != 0) &&
      (strcmp(locale, "POSIX") != 0)) {
    /* some 8-bit locale is set, assume its charset is correct */
#if defined(WINDOWS)
    static char codepage[8] = {'C', 'P'};
    GetLocaleInfo(GetThreadLocale(), LOCALE_IDEFAULTANSICODEPAGE, codepage+2, sizeof(codepage)-2);
    return codepage;
#elif defined(__MSDOS__)
    static char codepage[8];
    snprintf(codepage, sizeof(codepage), "CP%03u", getCodePage());
    return codepage;
#else /* Unix */
    return nl_langinfo(CODESET);
#endif /* locale character set */
  }
  return "ISO-8859-1";
}

const char *
getWcharCharset (void) {
  static const char *wcharCharset = NULL;

  if (!wcharCharset) {
    char charset[0X10];
    snprintf(charset, sizeof(charset), "UCS-%lu%cE",
             (unsigned long)sizeof(wchar_t),
#ifdef WORDS_BIGENDIAN
             'B'
#else /* WORDS_BIGENDIAN */
             'L'
#endif /* WORDS_BIGENDIAN */
            );

    wcharCharset = strdupWrapper(charset);
  }

  return wcharCharset;
}

const char *
setCharset (const char *name) {
  char *charset;

  if (name) {
    if (currentCharset && (strcmp(currentCharset, name) == 0)) return currentCharset;
  } else if (currentCharset) {
    return currentCharset;
  } else {
    name = getLocaleCharset();
  }
  if (!(charset = strdup(name))) return NULL;

#if defined(HAVE_ICONV_H)
  {
    const char *const wcharCharset = getWcharCharset();

    typedef struct {
      iconv_t *handle;
      const char *fromCharset;
      const char *toCharset;
      unsigned permanent:1;
      iconv_t newHandle;
    } ConvEntry;

    ConvEntry convTable[] = {
      {&iconvCharToWchar, charset, wcharCharset, 0},
      {&iconvWcharToChar, wcharCharset, charset, 0},
      {NULL, NULL, NULL, 1}
    };
    ConvEntry *conv = convTable;

    while (conv->handle) {
      if (conv->permanent && (*conv->handle != CHARSET_ICONV_NULL)) {
        conv->newHandle = CHARSET_ICONV_NULL;
      } else if ((conv->newHandle = iconv_open(conv->toCharset, conv->fromCharset)) == CHARSET_ICONV_NULL) {
        LogError("iconv_open");
        while (conv != convTable)
          if ((--conv)->newHandle != CHARSET_ICONV_NULL)
            iconv_close(conv->newHandle);
        free(charset);
        return NULL;
      } else if (conv->permanent) {
        *conv->handle = conv->newHandle;
        conv->newHandle = CHARSET_ICONV_NULL;
      }

      ++conv;
    }

    while (conv != convTable) {
      if ((--conv)->newHandle != CHARSET_ICONV_NULL) {
        if (*conv->handle != CHARSET_ICONV_NULL) iconv_close(*conv->handle);
        *conv->handle = conv->newHandle;
      }
    }
  }
#endif /* conversions */

  if (currentCharset) free(currentCharset);
  return currentCharset = charset;
}

const char *
getCharset (void) {
  return setCharset(NULL);
}

static LockDescriptor *
getCharsetLock (void) {
  static LockDescriptor *lock = NULL;
  return getLockDescriptor(&lock);
}

int
lockCharset (LockOptions options) {
  LockDescriptor *lock = getCharsetLock();
  if (!lock) return 0;
  return obtainLock(lock, options);
}

void
unlockCharset (void) {
  LockDescriptor *lock = getCharsetLock();
  if (lock) releaseLock(lock);
}
