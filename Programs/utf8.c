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

#include <stdio.h>
#include <string.h>

#include "log.h"
#include "utf8.h"
#include "unicode.h"

wchar_t *
allocateCharacters (size_t count) {
  {
    wchar_t *characters = malloc(count * sizeof(*characters));
    if (characters) return characters;
  }

  logMallocError();
  return NULL;
}

size_t
convertWcharToUtf8 (wchar_t wc, Utf8Buffer utf8) {
  size_t utfs;

  if (!(wc & ~0X7F)) {
    *utf8 = wc;
    utfs = 1;
  } else {
    Utf8Buffer buffer;
    char *end = &buffer[0] + sizeof(buffer);
    char *byte = end;
    static const wchar_t mask = (1 << ((sizeof(wchar_t) * 8) - 6)) - 1;

    do {
      *--byte = (wc & 0X3F) | 0X80;
    } while ((wc = (wc >> 6) & mask));

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
  const uint32_t initial = UINT32_MAX;
  uint32_t character = initial;
  int state = 0;

  while (*utfs) {
    const int first = character == initial;

    unsigned char byte = *(*utf8)++;
    (*utfs) -= 1;

    if (!(byte & 0X80)) {
      if (!first) goto truncated;
      character = byte;
      break;
    }

    if (!(byte & 0X40)) {
      if (first) break;
      character = (character << 6) | (byte & 0X3F);
      if (!--state) break;
    } else if (!first) {
      goto truncated;
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
        character = initial;
        break;
      }

      character = byte & ((1 << (6 - state)) - 1);
    }
  }

  while (*utfs) {
    if ((**utf8 & 0XC0) != 0X80) break;
    (*utf8) += 1;
    (*utfs) -= 1;
    character = initial;
  }

  if (character == initial) goto error;
  if (character > WCHAR_MAX) character = UNICODE_REPLACEMENT_CHARACTER;
  return character;

truncated:
  (*utf8) -= 1;
  (*utfs) += 1;
error:
  return WEOF;
}

void
convertUtf8ToWchars (const char **utf8, wchar_t **characters, size_t count) {
  while (**utf8 && (count > 1)) {
    size_t utfs = UTF8_LEN_MAX;
    wint_t character = convertUtf8ToWchar(utf8, &utfs);

    if (character == WEOF) break;
    *(*characters)++ = character;
    count -= 1;
  }

  if (count) **characters = 0;
}

size_t
makeUtf8FromWchars (const wchar_t *characters, unsigned int count, char *buffer, size_t size) {
  char *byte = buffer;
  const char *end = byte + size;

  for (unsigned int i=0; i<count; i+=1) {
    Utf8Buffer utf8;
    size_t utfs = convertWcharToUtf8(characters[i], utf8);

    char *next = byte + utfs;
    if (next >= end) break;

    memcpy(byte, utf8, utfs);
    byte = next;
  }

  *byte = 0;
  return byte - buffer;
}

char *
getUtf8FromWchars (const wchar_t *characters, unsigned int count, size_t *length) {
  size_t size = (count * UTF8_LEN_MAX) + 1;
  char buffer[size];
  size_t len = makeUtf8FromWchars(characters, count, buffer, size);
  char *text = strdup(buffer);

  if (!text) {
    logMallocError();
  } else if (length) {
    *length = len;
  }

  return text;
}

size_t
makeWcharsFromUtf8 (const char *text, wchar_t *characters, size_t size) {
  size_t length = strlen(text);
  size_t count = 0;

  while (length > 0) {
    const char *utf8 = text;
    size_t utfs = length;
    wint_t character = convertUtf8ToWchar(&utf8, &utfs);

    if (character == WEOF) break;
    if (!character) break;

    if (characters) {
      if (count == size) break;
      characters[count] = character;
    }

    count += 1;
    text = utf8;
    length = utfs;
  }

  if (characters && (count < size)) characters[count] = 0;
  return count;
}

size_t
countUtf8Characters (const char *text) {
  return makeWcharsFromUtf8(text, NULL, 0);
}

int
writeUtf8Character (FILE *stream, wchar_t character) {
  Utf8Buffer utf8;
  size_t utfs = convertWcharToUtf8(character, utf8);

  if (utfs) {
    if (fwrite(utf8, 1, utfs, stream) == utfs) {
      return 1;
    } else {
      logSystemError("fwrite");
    }
  } else {
    logBytes(LOG_ERR, "invalid Unicode character", &character, sizeof(character));
  }

  return 0;
}

int
writeUtf8Characters (FILE *stream, const wchar_t *characters, size_t count) {
  const wchar_t *character = characters;
  const wchar_t *end = character + count;

  while (character < end) {
    if (!writeUtf8Character(stream, *character++)) {
      return 0;
    }
  }

  return 1;
}
