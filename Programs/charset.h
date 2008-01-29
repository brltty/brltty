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

#ifndef BRLTTY_INCLUDED_CHARSET
#define BRLTTY_INCLUDED_CHARSET

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <limits.h>

#include "lock.h"

extern const char *setCharset (const char *name);
extern const char *getCharset (void);

extern const char *getWcharCharset (void);

typedef char Utf8Buffer[MB_LEN_MAX+1];

extern int convertCharToUtf8 (char c, Utf8Buffer utf8);
extern int convertUtf8ToChar (char **utf8, size_t *utfs);

extern int convertWcharToUtf8 (wchar_t wc, Utf8Buffer utf8);
extern wint_t convertUtf8ToWchar (char **utf8, size_t *utfs);

extern wint_t convertCharToWchar (char c);
extern int convertWcharToChar (wchar_t wc);

extern int lockCharset (LockOptions options);
extern void unlockCharset (void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BRLTTY_INCLUDED_CHARSET */
