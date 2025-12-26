/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2025 by The BRLTTY Developers.
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

#ifndef BRLTTY_INCLUDED_CMDPUT
#define BRLTTY_INCLUDED_CMDPUT

#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

extern size_t getConsoleWidth (void);
extern const char *getTranslatedText (const char *text);

extern void putFlush (void);
extern void putConsole (const char *bytes, size_t count);

extern void putString (const char *string);
extern void putBytes (const void *bytes, size_t count);
extern void putByte (char byte);
extern void putNewline (void);

extern void vputf (const char *format, va_list args);
extern void putf (const char *format, ...) PRINTF(1, 2);

extern void putWrappedText (
  const char *text, char *line,
  unsigned int lineIndent, unsigned int lineWidth
);

extern void putFormattedLines (
  const char *const *const *blocks,
  char *line, int lineWidth
);

extern void putUtf8Character (wchar_t character);
extern void putUtf8Characters (const wchar_t *characters, size_t count);
extern void putUtf8String (const wchar_t *string);

extern void putHexadecimalCharacter (wchar_t character);
extern void putHexadecimalCharacters (const wchar_t *characters, size_t count);
extern void putHexadecimalString (const wchar_t *string);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BRLTTY_INCLUDED_CMDPUT */
