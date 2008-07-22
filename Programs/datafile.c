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

#include <stdarg.h>
#include <string.h>
#include <errno.h>

#ifdef HAVE_ICU
#include <unicode/uchar.h>
#endif /* HAVE_ICU */

#include "misc.h"
#include "datafile.h"
#include "charset.h"

struct DataFileStruct {
  const char *name;
  int line;		/*line number in table */
  DataParser parser;
  void *data;

  const wchar_t *start;
  const wchar_t *end;
};

void
reportDataError (DataFile *file, char *format, ...) {
  char message[0X100];

  {
    const char *name = NULL;
    const int *line = NULL;
    va_list args;

    if (file) {
      name = file->name;
      if (file->line) line = &file->line;
    }

    va_start(args, format);
    formatInputError(message, sizeof(message), name, line, format, args);
    va_end(args);
  }

  LogPrint(LOG_WARNING, "%s", message);
}

int
includeDataFile (DataFile *file, const CharacterOperand *name) {
  const char *prefixAddress = file->name;
  int prefixLength = 0;
  const wchar_t *suffixAddress = name->characters;
  int suffixLength = name->length;

  if (*suffixAddress != WC_C('/')) {
    const char *prefixEnd = strrchr(prefixAddress, '/');
    if (prefixEnd) prefixLength = prefixEnd - prefixAddress + 1;
  }

  {
    char path[prefixLength + suffixLength + 1];
    snprintf(path, sizeof(path), "%.*s%.*" PRIws,
             prefixLength, prefixAddress, suffixLength, suffixAddress);
    return processDataFile(path, file->parser, file->data);
  }
}

static int
findDataOperand (DataFile *file) {
  while (iswspace(file->start[0])) file->start += 1;
  return *(file->end = file->start) != 0;
}

int
getDataOperand (DataFile *file, DataOperand *operand, const char *description) {
  if (!findDataOperand(file)) {
    if (description) reportDataError(file, "%s not specified", description);
    return 0;
  }

  do {
    file->end += 1;
  } while (file->end[0] && !iswspace(file->end[0]));

  operand->address = file->start;
  operand->length = file->end - file->start;
  return 1;
}

static int
isHexadecimalDigit (wchar_t digit, int *value, int *shift) {
  if (digit >= WC_C('0') && digit <= WC_C('9'))
    *value = digit - WC_C('0');
  else if (digit >= WC_C('a') && digit <= WC_C('f'))
    *value = digit - WC_C('a') + 10;
  else if (digit >= WC_C('A') && digit <= WC_C('F'))
    *value = digit - WC_C('A') + 10;
  else
    return 0;

  *shift = 4;
  return 1;
}

static int
isOctalDigit (wchar_t digit, int *value, int *shift) {
  if (digit < WC_C('0') || digit > WC_C('7')) return 0;
  *value = digit - WC_C('0');
  *shift = 3;
  return 1;
}

static int
parseCharacterOperand (DataFile *file, CharacterOperand *result, const DataOperand *operand) {
  int length = 0;		/*loop counters */
  int index = 0;

  while (index < operand->length) {
    wchar_t character = operand->address[index];

    if (character == WC_C('\\')) {
      int start = index;
      int ok = 0;

      if (++index < operand->length) {
        switch (character = operand->address[index]) {
          case WC_C('\\'):
            ok = 1;
            break;

          case WC_C('f'):
            character = WC_C('\f');
            ok = 1;
            break;

          case WC_C('n'):
            character = WC_C('\n');
            ok = 1;
            break;

          case WC_C('r'):
            character = WC_C('\r');
            ok = 1;
            break;

          case WC_C('s'):
            character = WC_C(' ');
            ok = 1;
            break;

          case WC_C('t'):
            character = WC_C('\t');
            ok = 1;
            break;

          case WC_C('v'):
            character = WC_C('\v');
            ok = 1;
            break;

          {
            int count;
            int (*isDigit) (wchar_t digit, int *value, int *shift);

          case WC_C('o'):
            count = 3;
            isDigit = isOctalDigit;
            goto doNumber;

          case WC_C('U'):
            count = 8;
            goto doHexadecimal;

          case WC_C('u'):
            count = 4;
            goto doHexadecimal;

          case WC_C('x'):
            count = 2;
            goto doHexadecimal;

          doHexadecimal:
            isDigit = isHexadecimalDigit;
            goto doNumber;

          doNumber:
            character = 0;

            while (++index < operand->length) {
              {
                int value;
                int shift;

                if (!isDigit(operand->address[index], &value, &shift)) break;
                character = (character << shift) | value;
              }

              if (!--count) {
                ok = 1;
                break;
              }
            }
            break;
          }

          case WC_C('<'): {
            const wchar_t *first = &operand->address[++index];
            const wchar_t *end = wmemchr(first, WC_C('>'), operand->length-index);

            if (end) {
              int count = end - first;
              index += count;

              {
                char name[count+1];

                {
                  int i;
                  for (i=0; i<count; i+=1) {
                    wchar_t wc = first[i];
                    if (wc == WC_C('_')) wc = WC_C(' ');
                    if (!iswLatin1(wc)) goto badName;
                    name[i] = wc;
                  }
                }
                name[count] = 0;

#ifdef HAVE_ICU
                {
                  UErrorCode error = U_ZERO_ERROR;
                  character = u_charFromName(U_EXTENDED_CHAR_NAME, name, &error);
                  if (U_SUCCESS(error)) ok = 1;
                }
#endif /* HAVE_ICU */
              }
            } else {
              index = operand->length - 1;
            }

          badName:
            break;
          }
        }
      }

      if (!ok) {
        if (index < operand->length) index += 1;
        reportDataError(file, "invalid escape sequence: %.*" PRIws,
                        index-start, &operand->address[start]);
        return 0;
      }
    }

    if (!character) character = WC_C(' ');
    result->characters[length++] = character;

    index += 1;
  }
  result->length = length;

  return 1;
}

int
getCharacterOperand (DataFile *file, CharacterOperand *characters, DataOperand *operand, const char *description) {
  if (getDataOperand(file, operand, description))
    if (parseCharacterOperand(file, characters, operand))
      return 1;
  return 0;
}

static int
processWcharLine (DataFile *file, const wchar_t *line) {
  file->end = file->start = line;
  if (!findDataOperand(file)) return 1;			/*blank line */
  if (file->start[0] == WC_C('#')) return 1;
  return file->parser(file, file->data);
}

static int
processUtf8Line (char *line, void *dataAddress) {
  DataFile *file = dataAddress;
  size_t length = strlen(line);
  const char *byte = line;
  wchar_t characters[length+1];
  wchar_t *character = characters;

  file->line += 1;

  while (length) {
    const char *start = byte;
    wint_t wc = convertUtf8ToWchar(&byte, &length);

    if (wc == WEOF) {
      reportDataError(file, "illegal UTF-8 character at offset %d", start-line);
      return 1;
    }

    *character++ = wc;
  }

  *character = 0;
  return processWcharLine(file, characters);
}

int
processDataFile (const char *name, DataParser parser, void *data) {
  int ok = 0;
  DataFile file;
  FILE *stream;

  file.name = name;
  file.line = 0;
  file.parser = parser;
  file.data = data;

  LogPrint(LOG_DEBUG, "including data file: %s", file.name);

  if ((stream = openDataFile(file.name, "r"))) {
    if (processLines(stream, processUtf8Line, &file)) ok = 1;
    fclose(stream);
  } else {
    LogPrint(LOG_ERR,
             "cannot open data file '%s': %s",
             file.name, strerror(errno));
  }

  return ok;
}				/*compilation completed */
