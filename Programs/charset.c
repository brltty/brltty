/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2006 by The BRLTTY Developers.
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
#include <wchar.h>
#include <locale.h>

#ifndef WINDOWS
#include <langinfo.h>
#endif /* WINDOWS */

#include "misc.h"
#include "lock.h"
#include "charset.h"

static char *currentCharset = NULL;

#if defined(HAVE_ICONV_H)
#include <iconv.h>

#define CHARSET_ICONV_NULL ((iconv_t)-1)
#define CHARSET_ICONV_HANDLE(name) iconv_t iconv##name = CHARSET_ICONV_NULL

static CHARSET_ICONV_HANDLE(CharToUtf8);
static CHARSET_ICONV_HANDLE(Utf8ToChar);
static CHARSET_ICONV_HANDLE(WcharToUtf8);
static CHARSET_ICONV_HANDLE(Utf8ToWchar);
static CHARSET_ICONV_HANDLE(CharToWchar);
static CHARSET_ICONV_HANDLE(WcharToChar);

#define CHARSET_CONVERT_UTF8_TO_TYPE(name, type, ret, eof) \
ret convert##name (char **utf8, size_t *utfs) { \
  if (getCharset()) { \
    type c; \
    type *cp = &c; \
    size_t cs = sizeof(c); \
    if ((iconv(iconv##name, utf8, utfs, (void *)&cp, &cs) != -1) || (errno == E2BIG)) return c; \
    LogError("iconv (UTF-8 -> " #type ")"); \
  } \
  return eof; \
}
CHARSET_CONVERT_UTF8_TO_TYPE(Utf8ToWchar, wchar_t, wint_t, WEOF)
CHARSET_CONVERT_UTF8_TO_TYPE(Utf8ToChar, unsigned char, int, EOF)
#undef CHARSET_CONVERT_UTF8_TO_TYPE

#define CHARSET_CONVERT_TYPE_TO_UTF8(name, type) \
int convert##name (type c, Utf8Buffer utf8) { \
  if (getCharset()) { \
    type *cp = &c; \
    size_t cs = sizeof(c); \
    size_t utfm = MB_LEN_MAX; \
    size_t utfs = utfm; \
    if (iconv(iconv##name, (void *)&cp, &cs, &utf8, &utfs) != -1) { \
      *utf8 = 0; \
      return utfm - utfs; \
    } \
    LogError("iconv (" #type " -> UTF-8)"); \
  } \
  return 0; \
}
CHARSET_CONVERT_TYPE_TO_UTF8(WcharToUtf8, wchar_t)
CHARSET_CONVERT_TYPE_TO_UTF8(CharToUtf8, char)
#undef CHARSET_CONVERT_TYPE_TO_UTF8

#define CHARSET_CONVERT_TYPE_TO_TYPE(name, from, to, ret, eof) \
ret convert##name (from f) { \
  if (getCharset()) { \
    from *fp = &f; \
    size_t fs = sizeof(f); \
    to t; \
    to *tp = &t; \
    size_t ts = sizeof(t); \
    if (iconv(iconv##name, (void *)&fp, &fs, (void *)&tp, &ts) != -1) return t; \
    LogError("iconv (" #from " -> " #to ")"); \
  } \
  return eof; \
}
CHARSET_CONVERT_TYPE_TO_TYPE(CharToWchar, char, wchar_t, wint_t, WEOF)
CHARSET_CONVERT_TYPE_TO_TYPE(WcharToChar, wchar_t, unsigned char, int, EOF)
#undef CHARSET_CONVERT_TYPE_TO_TYPE

#elif defined(WINDOWS)

int
convertWcharToUtf8 (wchar_t wc, Utf8Buffer utf8) {
  int result = WideCharToMultiByte(CP_UTF8, 0,
                                   &wc, 1, utf8, sizeof(Utf8Buffer),
                                   NULL, NULL);
  if (result) {
    utf8[result] = 0;
    return result;
  }
  LogWindowsError("WideCharToMultiByte[CP_UTF8]");
  return 0;
}

wint_t
convertUtf8ToWchar (char **utf8, size_t *utfs) {
  if (*utfs) {
    wchar_t wc;
    int result = MultiByteToWideChar(CP_UTF8, 0,
                                     *utf8, *utfs, &wc, 1);
    if (result || (GetLastError() == ERROR_INSUFFICIENT_BUFFER)) {
      if (**utf8 & 0X80) {
        do {
          ++*utf8, --*utfs;
        } while (*utfs && ((**utf8 & 0XC0) == 0X80));
      } else {
        ++*utf8, --*utfs;
      }
      return wc;
    }
    LogWindowsError("MultiByteToWideChar[CP_UTF8]");
  }
  return WEOF;
}

wint_t
convertCharToWchar (char c) {
  wchar_t wc;
  int result = MultiByteToWideChar(CP_THREAD_ACP, MB_ERR_INVALID_CHARS,
                                   &c, 1,  &wc, 1);
  if (result) return wc;
  LogWindowsError("MultiByteToWideChar[CP_THREAD_ACP]");
  return WEOF;
}

int
convertWcharToChar (wchar_t wc) {
  unsigned char c;
  int result = WideCharToMultiByte(CP_THREAD_ACP, WC_NO_BEST_FIT_CHARS /* WC_ERR_INVALID_CHARS */,
                                   &wc, 1, &c, 1,
                                   NULL, NULL);
  if (result) return c;
  LogWindowsError("WideCharToMultiByte[CP_THREAD_ACP]");
  return EOF;
}

int
convertCharToUtf8 (char c, Utf8Buffer utf8) {
  wchar_t wc = convertCharToWchar(c);
  if (wc == WEOF) return 0;
  return convertWcharToUtf8(wc, utf8);
}

int
convertUtf8ToChar (char **utf8, size_t *utfs) {
  wchar_t wc = convertUtf8ToWchar(utf8, utfs);
  if (wc == WEOF) return EOF;
  return convertWcharToChar(wc);
}
#endif /* conversions */

static const char *
getLocaleCharset (void) {
  const char *locale = setlocale(LC_ALL, "");

  if (locale && (MB_CUR_MAX == 1) &&
      (strcmp(locale, "C") != 0) &&
      (strcmp(locale, "POSIX") != 0)) {
    /* some 8-bit locale is set, assume its charset is correct */
#ifdef WINDOWS
    static char codepage[8] = {'C', 'P'};
    GetLocaleInfo(GetThreadLocale(), LOCALE_IDEFAULTANSICODEPAGE, codepage+2, sizeof(codepage)-2);
    return codepage;
#else /* WINDOWS */
    return nl_langinfo(CODESET);
#endif /* WINDOWS */
  }
  return "ISO-8859-1";
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
    static const char *const utf8Charset = "UTF-8";
    static const char *const wcharCharset = "WCHAR_T";

    typedef struct {
      iconv_t *handle;
      const char *fromCharset;
      const char *toCharset;
      unsigned permanent:1;
      iconv_t newHandle;
    } ConvEntry;

    ConvEntry convTable[] = {
      {&iconvCharToUtf8, charset, utf8Charset, 0},
      {&iconvUtf8ToChar, utf8Charset, charset, 0},
      {&iconvCharToWchar, charset, wcharCharset, 0},
      {&iconvWcharToChar, wcharCharset, charset, 0},
      {&iconvWcharToUtf8, wcharCharset, utf8Charset, 1},
      {&iconvUtf8ToWchar, utf8Charset, wcharCharset, 1},
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
