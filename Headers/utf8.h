/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2026 by The BRLTTY Developers.
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

#ifndef BRLTTY_INCLUDED_UTF8
#define BRLTTY_INCLUDED_UTF8

#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

wchar_t *allocateCharacters (size_t count);

#define UTF8_SIZE(bits) (((bits) < 8)? 1: (((bits) + 3) / 5))
#define UTF8_LEN_MAX UTF8_SIZE(32)
typedef char Utf8Buffer[UTF8_LEN_MAX + 1];

extern size_t convertCodepointToUtf8 (uint32_t codepoint, Utf8Buffer utf8);
extern int convertUtf8ToCodepoint (uint32_t *codepoint, const char **utf8, size_t *utfs);

extern size_t convertWcharToUtf8 (wchar_t wc, Utf8Buffer utf8);
extern wint_t convertUtf8ToWchar (const char **utf8, size_t *utfs);

/* Incremental (streaming) UTF-8 decoder. Bytes are supplied one at a time,
 * which suits a terminal emulator that consumes its input a byte at a time.
 * Zero-initialize the decoder before first use (and after each completed or
 * invalid character it is ready for the next one).
 */
typedef struct {
  uint32_t codepoint; /* accumulated value so far */
  uint32_t minimum; /* smallest value legal for this length (overlong guard) */
  unsigned char pending; /* continuation bytes still expected */
} Utf8StreamDecoder;

typedef enum {
  UTF8_DECODE_PENDING, /* byte consumed; more bytes are needed */
  UTF8_DECODE_DONE,    /* *character is a complete code point */
  UTF8_DECODE_INVALID  /* malformed input; *character is U+FFFD */
} Utf8DecodeResult;

/* Feed one byte to the decoder.
 *
 * On UTF8_DECODE_DONE the decoded code point is stored in *character.
 *
 * On UTF8_DECODE_INVALID the replacement character (U+FFFD) is stored in
 * *character and, if the offending byte may itself start a new character,
 * *reprocess is set non-zero so the caller can feed that same byte again.
 * *reprocess is always written (0 unless a restart is required).
 */
extern Utf8DecodeResult putUtf8StreamByte (
  Utf8StreamDecoder *decoder, unsigned char byte,
  wchar_t *character, int *reprocess
);

extern void convertUtf8ToWchars (const char **utf8, wchar_t **characters, size_t count);

extern size_t makeUtf8FromWchars (const wchar_t *characters, unsigned int count, char *buffer, size_t size);
extern char *getUtf8FromWchars (const wchar_t *characters, unsigned int count, size_t *length);

extern size_t makeWcharsFromUtf8 (const char *text, wchar_t *characters, size_t size);
extern size_t countUtf8Characters (const char *text);

extern int writeUtf8Character (FILE *stream, wchar_t character);
extern int writeUtf8Characters (FILE *stream, const wchar_t *characters, size_t count);
extern int writeUtf8ByteOrderMark (FILE *stream);

extern int isCharsetUTF8 (const char *name);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BRLTTY_INCLUDED_UTF8 */
