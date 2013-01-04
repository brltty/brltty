/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2013 by The BRLTTY Developers.
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
#include <iconv.h>
#include <locale.h>

#ifdef HAVE_LANGINFO_H
#include <langinfo.h>
#endif /* HAVE_LANGINFO_H */

#include "log.h"
#include "charset_internal.h"

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
    if (iconv(iconv##name, (void *)&fp, &fs, (void *)&tp, &ts) != (size_t)-1) return t; \
    logMessage(LOG_DEBUG, "iconv (" #from " -> " #to ") error: %s", strerror(errno)); \
  } \
  return eof; \
}
CHARSET_CONVERT_TYPE_TO_TYPE(CharToWchar, char, wchar_t, wint_t, WEOF)
CHARSET_CONVERT_TYPE_TO_TYPE(WcharToChar, wchar_t, unsigned char, int, EOF)
#undef CHARSET_CONVERT_TYPE_TO_TYPE

const char *
getLocaleCharset (void) {
  const char *locale = setlocale(LC_ALL, "");

  if (locale && !isPosixLocale(locale)) {
    /* some 8-bit locale is set, assume its charset is correct */
    return nl_langinfo(CODESET);
  }

  return defaultCharset;
}

int
registerCharacterSet (const char *charset) {
  const char *const wcharCharset = getWcharCharset();

  typedef struct {
    iconv_t *handle;
    const char *fromCharset;
    const char *toCharset;
    unsigned permanent:1;
    iconv_t newHandle;
  } ConvEntry;

  ConvEntry convTable[] = {
    {&iconvCharToWchar, charset, wcharCharset, 0, CHARSET_ICONV_NULL},
    {&iconvWcharToChar, wcharCharset, charset, 0, CHARSET_ICONV_NULL},
    {NULL, NULL, NULL, 1, CHARSET_ICONV_NULL}
  };
  ConvEntry *conv = convTable;

  while (conv->handle) {
    if (conv->permanent && (*conv->handle != CHARSET_ICONV_NULL)) {
      conv->newHandle = CHARSET_ICONV_NULL;
    } else if ((conv->newHandle = iconv_open(conv->toCharset, conv->fromCharset)) == CHARSET_ICONV_NULL) {
      logSystemError("iconv_open");

      while (conv != convTable)
        if ((--conv)->newHandle != CHARSET_ICONV_NULL)
          iconv_close(conv->newHandle);

      return 0;
    } else if (conv->permanent) {
      *conv->handle = conv->newHandle;
      conv->newHandle = CHARSET_ICONV_NULL;
    }

    conv += 1;
  }

  while (conv != convTable) {
    if ((--conv)->newHandle != CHARSET_ICONV_NULL) {
      if (*conv->handle != CHARSET_ICONV_NULL) iconv_close(*conv->handle);
      *conv->handle = conv->newHandle;
    }
  }

  return 1;
}
