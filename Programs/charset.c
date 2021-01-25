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

#include "prologue.h"

#include <string.h>

#include "log.h"
#include "charset_internal.h"
#include "lock.h"
#include "program.h"

int
isCharsetLatin1 (const char *name) {
  {
    const char *substring = "iso";
    size_t length = strlen(substring);
    if (strncasecmp(name, substring, length) != 0) return 0;
    name += length;
    if ((*name == '-') || (*name == '_')) name += 1;
  }

  {
    const char *substring = "8859";
    size_t length = strlen(substring);
    if (strncmp(name, substring, length) != 0) return 0;
    name += length;
    if (*name == '-') name += 1;
  }

  return strcmp(name, "1") == 0;
}

#if defined(__MINGW32__)
#include "system_windows.h"

#elif defined(__ANDROID__)
#include "system_java.h"

#else /* unix */
#include <locale.h>
#endif /* locale definitions */

const char defaultCharset[] = "ISO-8859-1";

static char *currentCharset = NULL;

char *
getLocaleName (void) {
#if defined(__MINGW32__)
  return getWindowsLocaleName();

#elif defined(__ANDROID__)
  return getJavaLocaleName();

#else /* unix */
  char *name = setlocale(LC_CTYPE, NULL);

  if (name) {
    if (!(name = strdup(name))) {
      logMallocError();
    }
  }

  return name;
#endif /* text table locale */
}

int
isPosixLocale (const char *locale) {
  if (strcmp(locale, "C") == 0) return 1;
  if (strcmp(locale, "POSIX") == 0) return 1;
  return 0;
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

    if ((wcharCharset = strdup(charset))) {
      registerProgramMemory("wchar-charset", &wcharCharset);
    } else {
      logMallocError();
    }
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

  if (!(charset = strdup(name))) {
    logMallocError();
    return NULL;
  }

  if (!registerCharacterSet(charset)) {
    free(charset);
    return NULL;
  }

  if (currentCharset) {
    free(currentCharset);
  } else {
    registerProgramMemory("current-charset", &currentCharset);
  }

  return currentCharset = charset;
}

const char *
getCharset (void) {
  return setCharset(NULL);
}

static LockDescriptor *
getCharsetLock (void) {
  static LockDescriptor *lock = NULL;

  return getLockDescriptor(&lock, "charset");
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
