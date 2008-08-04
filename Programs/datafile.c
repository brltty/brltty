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
#include "brldots.h"

struct DataFileStruct {
  const char *name;
  int line;		/*line number in table */
  DataProcessor *processor;
  void *data;

  const wchar_t *start;
  const wchar_t *end;
};

const wchar_t brlDotNumbers[BRL_DOT_COUNT] = WS_C("12345678");
const unsigned char brlDotBits[BRL_DOT_COUNT] = {
  BRL_DOT1, BRL_DOT2, BRL_DOT3, BRL_DOT4,
  BRL_DOT5, BRL_DOT6, BRL_DOT7, BRL_DOT8
};

int
brlDotNumberToIndex (wchar_t number, int *index) {
  const wchar_t *character = wmemchr(brlDotNumbers, number, ARRAY_COUNT(brlDotNumbers));
  if (!character) return 0;
  *index = character - brlDotNumbers;
  return 1;
}

int
brlDotBitToIndex (unsigned char bit, int *index) {
  const unsigned char *cell = memchr(brlDotBits, bit, ARRAY_COUNT(brlDotBits));
  if (!cell) return 0;
  *index = cell - brlDotBits;
  return 1;
}

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
findDataOperand (DataFile *file, const char *description) {
  file->start = file->end;

  while (iswspace(file->start[0])) file->start += 1;
  if (*(file->end = file->start)) return 1;
  if (description) reportDataError(file, "%s not specified", description);
  return 0;
}

int
getDataCharacter (DataFile *file, wchar_t *character) {
  if (!*file->end) return 0;
  *character = *file->end++;
  return 1;
}

int
ungetDataCharacters (DataFile *file, unsigned int count) {
  unsigned int maximum = file->end - file->start;

  if (count > maximum) {
    reportDataError(file, "unget character count out of range: %u > %u",
                    count, maximum);
    return 0;
  }

  file->end -= count;
  return 1;
}

int
getDataOperand (DataFile *file, DataOperand *operand, const char *description) {
  if (!findDataOperand(file, description)) return 0;

  do {
    file->end += 1;
  } while (file->end[0] && !iswspace(file->end[0]));

  operand->characters = file->start;
  operand->length = file->end - file->start;
  return 1;
}

static int
isHexadecimalDigit (wchar_t character, int *value, int *shift) {
  if ((character >= WC_C('0')) && (character <= WC_C('9'))) {
    *value = character - WC_C('0');
  } else if ((character >= WC_C('a')) && (character <= WC_C('f'))) {
    *value = character - WC_C('a') + 10;
  } else if ((character >= WC_C('A')) && (character <= WC_C('F'))) {
    *value = character - WC_C('A') + 10;
  } else {
    return 0;
  }

  *shift = 4;
  return 1;
}

static int
isOctalDigit (wchar_t character, int *value, int *shift) {
  if ((character < WC_C('0')) || (character > WC_C('7'))) return 0;
  *value = character - WC_C('0');
  *shift = 3;
  return 1;
}

static int
parseDataString (DataFile *file, DataString *string, const wchar_t *characters, int length) {
  int index = 0;

  string->length = 0;

  while (index < length) {
    wchar_t character = characters[index];

    if (character == WC_C('\\')) {
      int start = index;
      int ok = 0;

      if (++index < length) {
        switch ((character = characters[index])) {
          case WC_C('#'):
          case WC_C('\\'):
            ok = 1;
            break;

          case WC_C('b'):
            character = WC_C('\b');
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
            int (*isDigit) (wchar_t character, int *value, int *shift);

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

          case WC_C('X'):
          case WC_C('x'):
            count = 2;
            goto doHexadecimal;

          doHexadecimal:
            isDigit = isHexadecimalDigit;
            goto doNumber;

          doNumber:
            character = 0;

            while (++index < length) {
              {
                int value;
                int shift;

                if (!isDigit(characters[index], &value, &shift)) break;
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
            const wchar_t *first = &characters[++index];
            const wchar_t *end = wmemchr(first, WC_C('>'), length-index);

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
              index = length - 1;
            }

          badName:
            break;
          }
        }
      }

      if (!ok) {
        if (index < length) index += 1;
        reportDataError(file, "invalid escape sequence: %.*" PRIws,
                        index-start, &characters[start]);
        return 0;
      }
    }

    if (string->length == ARRAY_COUNT(string->characters)) {
      reportDataError(file, "string operand too long");
      return 0;
    }

    string->characters[string->length++] = character;
    index += 1;
  }

  return 1;
}

int
getDataString (DataFile *file, DataString *string, const char *description) {
  DataOperand operand;

  if (getDataOperand(file, &operand, description))
    if (parseDataString(file, string, operand.characters, operand.length))
      return 1;

  return 0;
}

static int
parseDotOperand (DataFile *file, int *index, const wchar_t *characters, int length) {
  if (length == 1)
    if (brlDotNumberToIndex(characters[0], index))
      return 1;

  reportDataError(file, "invalid braille dot number: %.*" PRIws, length, characters);
  return 0;
}

int
getDotOperand (DataFile *file, int *index) {
  DataOperand number;

  if (getDataOperand(file, &number, "dot number"))
    if (parseDotOperand(file, index, number.characters, number.length))
      return 1;

  return 0;
}

int
includeDataFile (DataFile *file, const wchar_t *name, unsigned int length) {
  const char *prefixAddress = file->name;
  unsigned int prefixLength = 0;

  if (*name != WC_C('/')) {
    const char *prefixEnd = strrchr(prefixAddress, '/');
    if (prefixEnd) prefixLength = prefixEnd - prefixAddress + 1;
  }

  {
    char path[prefixLength + length + 1];
    snprintf(path, sizeof(path), "%.*s%.*" PRIws,
             prefixLength, prefixAddress, length, name);
    return processDataFile(path, file->processor, file->data);
  }
}

int
processIncludeOperands (DataFile *file, void *data) {
  DataString path;

  if (getDataString(file, &path, "include file path"))
    if (!includeDataFile(file, path.characters, path.length))
      return 0;

  return 1;
}

int
processPropertyOperand (DataFile *file, const DataProperty *properties, const char *description, void *data) {
  DataOperand name;

  if (getDataOperand(file, &name, description)) {
    {
      const DataProperty *property = properties;

      while (property->name) {
        if (name.length == wcslen(property->name))
          if (wmemcmp(name.characters, property->name, name.length) == 0)
            return property->processor(file, data);

        property += 1;
      }

      if (property->processor) {
        ungetDataCharacters(file, name.length);
        return property->processor(file, data);
      }
    }

    reportDataError(file, "unknown %s: %.*" PRIws,
                    description, name.length, name.characters);
  }

  return 1;
}

static int
processWcharLine (DataFile *file, const wchar_t *line) {
  file->end = file->start = line;
  if (!findDataOperand(file, NULL)) return 1;			/*blank line */
  if (file->start[0] == WC_C('#')) return 1;
  return file->processor(file, file->data);
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
      unsigned int offset = start - line;
      reportDataError(file, "illegal UTF-8 character at offset %u", offset);
      return 1;
    }

    *character++ = wc;
  }

  *character = 0;
  return processWcharLine(file, characters);
}

int
processDataStream (FILE *stream, const char *name, DataProcessor processor, void *data) {
  DataFile file;

  file.name = name;
  file.line = 0;
  file.processor = processor;
  file.data = data;

  LogPrint(LOG_DEBUG, "including data file: %s", file.name);
  return processLines(stream, processUtf8Line, &file);
}

int
processDataFile (const char *name, DataProcessor processor, void *data) {
  int ok = 0;
  FILE *stream;

  if ((stream = openDataFile(name, "r"))) {
    if (processDataStream(stream, name, processor, data)) ok = 1;
    fclose(stream);
  } else {
    LogPrint(LOG_ERR, "cannot open data file '%s': %s", name, strerror(errno));
  }

  return ok;
}
