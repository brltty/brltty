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
#include <ctype.h>
#include <errno.h>

#ifdef HAVE_ICU
#include <unicode/uchar.h>
#endif /* HAVE_ICU */

#include "program.h"
#include "options.h"
#include "misc.h"
#include "brldots.h"
#include "charset.h"
#include "ttb.h"
#include "ttb_internal.h"
#include "ttb_compile.h"

static char *opt_charset;
static char *opt_inputFormat;
static char *opt_outputFormat;
static char *opt_tablesDirectory;

#ifdef ENABLE_API
static int opt_edit;
#endif /* ENABLE_API */

BEGIN_OPTION_TABLE(programOptions)
  { .letter = 'T',
    .word = "tables-directory",
    .flags = OPT_Hidden,
    .argument = "directory",
    .setting.string = &opt_tablesDirectory,
    .defaultSetting = DATA_DIRECTORY,
    .description = "Path to directory containing text tables."
  },

#ifdef ENABLE_API
  { .letter = 'e',
    .word = "edit",
    .setting.flag = &opt_edit,
    .description = "Edit table."
  },
#endif /* ENABLE_API */

  { .letter = 'i',
    .word = "input-format",
    .argument = "format",
    .setting.string = &opt_inputFormat,
    .description = "Format of input file."
  },

  { .letter = 'o',
    .word = "output-format",
    .argument = "format",
    .setting.string = &opt_outputFormat,
    .description = "Format of output file."
  },

  { .letter = 'c',
    .word = "charset",
    .argument = "charset",
    .setting.string = &opt_charset,
    .description = "8-bit character set to use."
  },
END_OPTION_TABLE

static const BrlDotTable dotsInternal = {
  BRL_DOT1, BRL_DOT2, BRL_DOT3, BRL_DOT4,
  BRL_DOT5, BRL_DOT6, BRL_DOT7, BRL_DOT8
};

static const BrlDotTable dots12345678 = {
  0X01, 0X02, 0X04, 0X08, 0X10, 0X20, 0X40, 0X80
};

static const BrlDotTable dots14253678 = {
  0X01, 0X04, 0X10, 0X02, 0X08, 0X20, 0X40, 0X80
};

static unsigned char
mapDots (unsigned char input, const BrlDotTable from, const BrlDotTable to) {
  unsigned char output = 0;
  {
    int dot;
    for (dot=0; dot<BRL_DOT_COUNT; ++dot) {
      if (input & from[dot]) output |= to[dot];
    }
  }
  return output;
}

typedef TextTableData *TableReader (const char *path, FILE *file, void *data);
typedef int TableWriter (const char *path, FILE *file, TextTableData *ttd, void *data);
typedef int CharacterWriter (FILE *file, wchar_t character, unsigned char dots, const unsigned char *byte, void *data);

static int
getDots (TextTableData *ttd, wchar_t character, unsigned char *dots) {
  const UnicodeCellEntry *cell = getUnicodeCellEntry(ttd, character);
  if (!cell) return 0;
  *dots = cell->dots;
  return 1;
}

static int
writeCharacters (FILE *file, TextTableData *ttd, CharacterWriter writer, void *data) {
  if (*opt_charset) {
    unsigned char byte = 0;

    do {
      wint_t character = convertCharToWchar(byte);

      if (character != WEOF) {
        const UnicodeCellEntry *cell = getUnicodeCellEntry(ttd, character);

        if (cell) {
          if (!writer(file, character, cell->dots, &byte, data)) return 0;
        }
      }
    } while ((byte += 1));
  }

  {
    const TextTableHeader *header = getTextTableHeader(ttd);
    unsigned int groupNumber;

    for (groupNumber=0; groupNumber<UNICODE_GROUP_COUNT; groupNumber+=1) {
      TextTableOffset groupOffset = header->unicodeGroups[groupNumber];

      if (groupOffset) {
        const UnicodeGroupEntry *group = getTextTableItem(ttd, groupOffset);
        unsigned int plainNumber;

        for (plainNumber=0; plainNumber<UNICODE_PLAINS_PER_GROUP; plainNumber+=1) {
          TextTableOffset plainOffset = group->plains[plainNumber];

          if (plainOffset) {
            const UnicodePlainEntry *plain = getTextTableItem(ttd, plainOffset);
            unsigned int rowNumber;

            for (rowNumber=0; rowNumber<UNICODE_ROWS_PER_PLAIN; rowNumber+=1) {
              TextTableOffset rowOffset = plain->rows[rowNumber];

              if (rowOffset) {
                const UnicodeRowEntry *row = getTextTableItem(ttd, rowOffset);
                unsigned int cellNumber;

                for (cellNumber=0; cellNumber<UNICODE_CELLS_PER_ROW; cellNumber+=1) {
                  if (BITMASK_TEST(row->defined, cellNumber)) {
                    wchar_t character = UNICODE_CHARACTER(groupNumber, plainNumber, rowNumber, cellNumber);

                    if (*opt_charset)
                      if (convertWcharToChar(character) != EOF)
                        continue;

                    {
                      const UnicodeCellEntry *cell = &row->cells[cellNumber];
                      if (!writer(file, character, cell->dots, NULL, data)) return 0;
                    }
                  }
                }
              }
            }
          }
        }
      }
    }
  }

  return 1;
}

static int
writeUtf8 (FILE *file, wchar_t character) {
  Utf8Buffer utf8;
  size_t utfs = convertWcharToUtf8(character, utf8);
  return fprintf(file, "%.*s", utfs, utf8) != EOF;
}

static TextTableData *
readTable_native (const char *path, FILE *file, void *data) {
  return processTextTableStream(file, path);
}

static int
writeDots_native (FILE *file, unsigned char dots) {
  unsigned char dot;

  if (fprintf(file, "(") == EOF) return 0;
  for (dot=0X01; dot; dot<<=1) {
    char number = (dots & dot)? brlDotToNumber(dot): ' ';
    if (fprintf(file, "%c", number) == EOF) return 0;
  }
  if (fprintf(file, ")") == EOF) return 0;

  return 1;
}

static int
writeCharacter_native (FILE *file, wchar_t character, unsigned char dots, const unsigned char *byte, void *data) {
  uint32_t value = character;

  if (fprintf(file, "char ") == EOF) goto error;

  if (value < 0X100) {
    if (fprintf(file, "\\x%02X", value) == EOF) goto error;
  } else if (value < 0X10000) {
    if (fprintf(file, "\\u%04X", value) == EOF) goto error;
  } else {
    if (fprintf(file, "\\U%08X", value) == EOF) goto error;
  }

  if (fprintf(file, "\t") == EOF) goto error;
  if (!writeDots_native(file, dots)) goto error;
  if (fprintf(file, "  #") == EOF) goto error;

  if (*opt_charset) {
    const int width = 2;
    char buffer[width + 1];

    if (byte) {
      snprintf(buffer, sizeof(buffer), "%0*X", width, *byte);
    } else {
      snprintf(buffer, sizeof(buffer), "%*s", width, "");
    }

    if (fprintf(file, " %s", buffer) == EOF) goto error;
  }

  if (fprintf(file, " ") == EOF) goto error;
  if (!writeUtf8(file, UNICODE_BRAILLE_ROW | dots)) goto error;

  if (fprintf(file, " ") == EOF) goto error;
  if (!writeUtf8(file, ((iswprint(character) && !iswspace(character))? character: WC_C(' ')))) goto error;

#ifdef HAVE_ICU
  {
    char name[0X40];
    UErrorCode error = U_ZERO_ERROR;

    u_charName(character, U_EXTENDED_CHAR_NAME, name, sizeof(name), &error);
    if (U_SUCCESS(error)) {
      if (fprintf(file, " [%s]", name) == EOF) return 0;
    }
  }
#endif /* HAVE_ICU */

  if (fprintf(file, "\n") == EOF) goto error;
  return 1;

error:
  return 0;
}

static int
writeTable_native (const char *path, FILE *file, TextTableData *ttd, void *data) {
  if (fprintf(file, "# generated by %s:", programName) == EOF) goto error;
  if (*opt_charset)
    if (fprintf(file, " charset=%s", opt_charset) == EOF)
      goto error;
  if (fprintf(file, "\n") == EOF) goto error;

  if (!writeCharacters(file, ttd, writeCharacter_native, data)) goto error;
  return 1;

error:
  return 0;
}

static TextTableData *
readTable_binary (const char *path, FILE *file, void *data) {
  TextTableData *ttd;

  if ((ttd = newTextTableData())) {
    int count = 0X100;
    int byte;

    for (byte=0; byte<count; byte+=1) {
      int dots = fgetc(file);

      if (dots == EOF) {
        if (ferror(file)) {
          LogPrint(LOG_ERR, "input error: %s: %s", path, strerror(errno));
        } else {
          LogPrint(LOG_ERR, "table too short: %s", path);
        }

        break;
      }

      if (data) dots = mapDots(dots, data, dotsInternal);
      if (!setTextTableByte(ttd, byte, dots)) break;
    }
    if (byte == count) return ttd;

    destroyTextTableData(ttd);
  }

  return NULL;
}

static int
writeTable_binary (const char *path, FILE *file, TextTableData *ttd, void *data) {
  int byte;

  for (byte=0; byte<0X100; byte+=1) {
    unsigned char dots;

    {
      wint_t character = convertCharToWchar(byte);
      if (character == WEOF) character = UNICODE_REPLACEMENT_CHARACTER;

      if (!getDots(ttd, character, &dots))
        dots = 0;
    }

    if (data) dots = mapDots(dots, dotsInternal, data);
    if (fputc(dots, file) == EOF) {
      LogPrint(LOG_ERR, "output error: %s: %s", path, strerror(errno));
      return 0;
    }
  }

  return 1;
}

#ifdef HAVE_ICONV_H
static TextTableData *
readTable_gnome (const char *path, FILE *file, void *data) {
  return processGnomeBrailleStream(file, path);
}

static int
writeCharacter_gnome (FILE *file, wchar_t character, unsigned char dots, const unsigned char *byte, void *data) {
  wchar_t pattern = UNICODE_BRAILLE_ROW | dots;

  if (iswprint(character) && !iswspace(character)) {
    Utf8Buffer utf8Character;
    Utf8Buffer utf8Pattern;

    if (!convertWcharToUtf8(character, utf8Character)) return 0;
    if (!convertWcharToUtf8(pattern, utf8Pattern)) return 0;
    if (fprintf(file, "UCS-CHAR %s %s\n", utf8Character, utf8Pattern) == EOF) return 0;
  } else {
    uint32_t c = character;
    uint32_t p = pattern;
    if (fprintf(file, "UNICODE-CHAR U+%02" PRIx32 " U+%04" PRIx32" \n", c, p) == EOF) return 0;
  }

  return 1;
}

static int
writeTable_gnome (const char *path, FILE *file, TextTableData *ttd, void *data) {
  /* TODO UNKNOWN-CHAR %wc all */
  if (fprintf(file, "ENCODING UTF-8\n") == EOF) goto error;
  if (fprintf(file, "# generated by %s\n", programName) == EOF) goto error;
  if (!writeCharacters(file, ttd, writeCharacter_gnome, data)) goto error;
  return 1;

error:
  return 0;
}
#endif /* HAVE_ICONV_H */

static TextTableData *
readTable_libLouis (const char *path, FILE *file, void *data) {
  return processLibLouisStream(file, path);
}

static int
writeCharacter_libLouis (FILE *file, wchar_t character, unsigned char dots, const unsigned char *byte, void *data) {
  int i;

  if (iswspace(character)) {
    fprintf(file, "space");
  } else if (iswdigit(character)) {
    fprintf(file, "digit");
  } else if (iswpunct(character)) {
    fprintf(file, "punctuation");
  } else if (iswlower(character)) {
    fprintf(file, "lowercase");
  } else if (iswupper(character)) {
    fprintf(file, "uppercase");
  } else {
    fprintf(file, "letter");
  }
  fprintf(file, " ");

  if (character == '\\') {
    fprintf(file, "\\\\");
  } else if (character == '\f') {
    fprintf(file, "\\f");
  } else if (character == '\n') {
    fprintf(file, "\\n");
  } else if (character == '\r') {
    fprintf(file, "\\r");
  } else if (character == ' ') {
    fprintf(file, "\\s");
  } else if (character == '\t') {
    fprintf(file, "\\t");
  } else if (character == '\v') {
    fprintf(file, "\\v");
  } else if (character > 0X20 && character < 0X7F) {
    fprintf(file, "%c", character);
  } else if (character < 0X1000) {
    fprintf(file, "\\x%04x", character);
  } else if (character < 0X10000) {
    fprintf(file, "\\x%05x", character);
  } else {
    fprintf(file, "\\x%08x", character);
  }
  fprintf(file, " ");

  if (!dots)
    fprintf(file, "0");
  else
    for (i = 1; i <= 8; i++) {
      if (dots & (1 << (i - 1)))
	fprintf(file, "%c", '0' + i);
    }
#ifdef HAVE_ICU
  {
    char name[0X40];
    UErrorCode error = U_ZERO_ERROR;

    u_charName(character, U_EXTENDED_CHAR_NAME, name, sizeof(name), &error);
    if (U_SUCCESS(error)) {
      if (fprintf(file, "\t\t%s", name) == EOF) return 0;
    }
  }
#endif /* HAVE_ICU */

  fprintf(file, "\n");

  return 1;
}

static int
writeTable_libLouis (const char *path, FILE *file, TextTableData *ttd, void *data) {
  if (fprintf(file, "# generated by %s\n", programName) == EOF) goto error;
  if (!writeCharacters(file, ttd, writeCharacter_libLouis, data)) goto error;
  return 1;

error:
  return 0;
}
typedef struct {
  const char *name;
  TableReader *read;
  TableWriter *write;
  void *data;
} FormatEntry;

static const FormatEntry formatEntries[] = {
  {"ttb", readTable_native, writeTable_native, NULL},
  {"a2b", readTable_binary, writeTable_binary, &dots12345678},
  {"sbl", readTable_binary, writeTable_binary, &dots14253678},

#ifdef HAVE_ICONV_H
  {"gnb", readTable_gnome, writeTable_gnome, NULL},
#endif /* HAVE_ICONV_H */

  {"ctb", readTable_libLouis, writeTable_libLouis, NULL},

  {NULL}
};

static const FormatEntry *
findFormatEntry (const char *name) {
  const FormatEntry *format = formatEntries;
  while (format->name) {
    if (strcmp(name, format->name) == 0) return format;
    format += 1;
  }
  return NULL;
}

static const FormatEntry *
getFormatEntry (const char *name, const char *path, const char *description) {
  if (!(name && *name)) {
    name = locatePathExtension(path);

    if (!(name && *++name)) {
      LogPrint(LOG_ERR, "unspecified %s format.", description);
      exit(2);
    }
  }

  {
    const FormatEntry *format = findFormatEntry(name);
    if (format) return format;
  }

  LogPrint(LOG_ERR, "unknown %s format: %s", description, name);
  exit(2);
}

static const char *inputPath;
static const char *outputPath;
static const FormatEntry *inputFormat;
static const FormatEntry *outputFormat;

static FILE *
openTable (const char **file, const char *mode, const char *directory, FILE *stdStream, const char *stdName) {
  if (stdStream) {
    if (strcmp(*file, "-") == 0) {
      *file = stdName;
      return stdStream;
    }
  }

  if (directory) {
    const char *path = makePath(directory, *file);
    if (!path) return NULL;
    *file = path;
  }

  {
    FILE *stream = fopen(*file, mode);
    if (!stream) LogPrint(LOG_ERR, "table open error: %s: %s", *file, strerror(errno));
    return stream;
  }
}

static int
convertTable (void) {
  int status;
  FILE *inputFile = openTable(&inputPath, "r", opt_tablesDirectory, stdin, "<standard-input>");

  if (inputFile) {
    TextTableData *ttd;

    if ((ttd = inputFormat->read(inputPath, inputFile, inputFormat->data))) {
      if (outputPath) {
        FILE *outputFile = openTable(&outputPath, "w", NULL, stdout, "<standard-output>");

        if (outputFile) {
          if (outputFormat->write(outputPath, outputFile, ttd, outputFormat->data)) {
            status = 0;
          } else {
            status = 6;
          }

          fclose(outputFile);
        } else {
          status = 5;
        }
      } else {
        status = 0;
      }

      destroyTextTableData(ttd);
    } else {
      status = 4;
    }

    fclose(inputFile);
  } else {
    status = 3;
  }

  return status;
}

#ifdef ENABLE_API
#define BRLAPI_NO_DEPRECATED
#include "brlapi.h"

#undef USE_CURSES
#undef USE_FUNC_GET_WCH
#if defined(HAVE_PKG_CURSES)
#define USE_CURSES
#include <curses.h>

#elif defined(HAVE_PKG_NCURSES)
#define USE_CURSES
#include <ncurses.h>

#elif defined(HAVE_PKG_NCURSESW)
#define USE_CURSES
#define USE_FUNC_GET_WCH
#include <ncursesw/ncurses.h>

#else /* standard input/output */
#warning curses package either unspecified or unsupported
#define printw printf
#define erase() printf("\r\n\v")
#define refresh() fflush(stdout)
#define beep() printf("\a")
#endif /* curses package */

typedef struct {
  TextTableData *ttd;
  unsigned updated:1;

  const char *charset;
  union {
    wchar_t unicode;
    unsigned char byte;
  } character;

  brlapi_fileDescriptor brlapiFileDescriptor;
  unsigned int displayWidth;
  unsigned int displayHeight;
  const char *brlapiErrorFunction;
  const char *brlapiErrorMessage;
} EditTableData;

static int
haveBrailleDisplay (EditTableData *etd) {
  return etd->brlapiFileDescriptor != (brlapi_fileDescriptor)-1;
}

static void
setBrlapiError (EditTableData *etd, const char *function) {
  if ((etd->brlapiErrorFunction = function)) {
    etd->brlapiErrorMessage = brlapi_strerror(&brlapi_error);
  } else {
    etd->brlapiErrorMessage = NULL;
  }
}

static void
releaseBrailleDisplay (EditTableData *etd) {
  brlapi_closeConnection();
  etd->brlapiFileDescriptor = (brlapi_fileDescriptor)-1;
}

static int
claimBrailleDisplay (EditTableData *etd) {
  etd->brlapiFileDescriptor = brlapi_openConnection(NULL, NULL);
  if (haveBrailleDisplay(etd)) {
    if (brlapi_getDisplaySize(&etd->displayWidth, &etd->displayHeight) != -1) {
      if (brlapi_enterTtyMode(BRLAPI_TTY_DEFAULT, NULL) != -1) {
        setBrlapiError(etd, NULL);
        return 1;
      } else {
        setBrlapiError(etd, "brlapi_enterTtyMode");
      }
    } else {
      setBrlapiError(etd, "brlapi_getDisplaySize");
    }

    releaseBrailleDisplay(etd);
  } else {
    setBrlapiError(etd, "brlapi_openConnection");
  }

  return 0;
}

static int
getCharacter (EditTableData *etd, wchar_t *character) {
  if (etd->charset) {
    wint_t wc = convertCharToWchar(etd->character.byte);
    if (wc == WEOF) return 0;;
    *character = wc;
  } else {
    *character = etd->character.unicode;
  }

  return 1;
}

static wchar_t *
makeCharacterDescription (TextTableData *ttd, wchar_t character, size_t *length, int *defined, unsigned char *braille) {
  char buffer[0X100];
  int characterIndex;
  int brailleIndex;
  int descriptionLength;

  unsigned char dots;
  int gotDots = getDots(ttd, character, &dots);

  wchar_t printableCharacter;
  char printablePrefix;

  printableCharacter = character;
  if (iswLatin1(printableCharacter)) {
    printablePrefix = '^';
    if (!(printableCharacter & 0X60)) {
      printableCharacter |= 0X40;
      if (printableCharacter & 0X80) printablePrefix = '~';
    } else if (printableCharacter == 0X7F) {
      printableCharacter ^= 0X40;       
    } else if (printableCharacter != printablePrefix) {
      printablePrefix = ' ';
    }
  } else {
    printablePrefix = ' ';
  }
  if (!gotDots) dots = 0;

#define DOT(n) ((dots & BRLAPI_DOT##n)? ((n) + '0'): ' ')
  {
    uint32_t value = character;

    snprintf(buffer, sizeof(buffer),
             "%04" PRIX32 " %c%nx %nx %c%c%c%c%c%c%c%c%c%c%n",
             value, printablePrefix, &characterIndex, &brailleIndex,
             (gotDots? '[': ' '),
             DOT(1), DOT(2), DOT(3), DOT(4), DOT(5), DOT(6), DOT(7), DOT(8),
             (gotDots? ']': ' '),
             &descriptionLength);
  }
#undef DOT

#ifdef HAVE_ICU
  {
    char *name = buffer + descriptionLength + 1;
    int size = buffer + sizeof(buffer) - name;
    UErrorCode error = U_ZERO_ERROR;

    u_charName(character, U_EXTENDED_CHAR_NAME, name, size, &error);
    if (U_SUCCESS(error)) {
      descriptionLength += strlen(name) + 1;
      *--name = ' ';
    }
  }
#endif /* HAVE_ICU */

  {
    wchar_t *description = calloc(descriptionLength+1, sizeof(*description));
    if (description) {
      int i;
      for (i=0; i<descriptionLength; i+=1) {
        wint_t wc = convertCharToWchar(buffer[i]);
        if (wc == WEOF) wc = WC_C(' ');
        description[i] = wc;
      }

      description[characterIndex] = printableCharacter;
      description[brailleIndex] = gotDots? (UNICODE_BRAILLE_ROW | dots): WC_C(' ');
      description[descriptionLength] = 0;

      if (length) *length = descriptionLength;
      if (defined) *defined = gotDots;
      if (braille) *braille = dots;
      return description;
    }
  }

  return NULL;
}

static void
printCharacterString (const wchar_t *wcs) {
  while (*wcs) {
    wint_t wc = *wcs++;
    printw("%" PRIwc, wc);
  }
}

static int
updateCharacterDescription (EditTableData *etd) {
  int ok = 0;

  wchar_t character;
  int gotCharacter = getCharacter(etd, &character);

  size_t length;
  int gotDots;
  unsigned char dots;
  wchar_t *description = makeCharacterDescription(etd->ttd, character, &length, &gotDots, &dots);

  if (description) {
    ok = 1;
    erase();

#if defined(USE_CURSES)
    printw("Left/Right: previous/next %s character\n",
           etd->charset? etd->charset: "unicode");
    printw("Up/Down: previous/next defined character\n");
    printw("Home/End: first/last defined character\n");

#define DOT(n) printw("F%u: %s dot %u  ", n, ((dots & BRLAPI_DOT##n)? "lower": "raise"), n)
    DOT(1);
    DOT(5);
    printw("F9: %s",
           !gotCharacter? "":
           !gotDots? "define (as empty cell)":
           dots? "clear all dots":
           "undefine character");
    printw("\n");

    DOT(2);
    DOT(6);
    printw("F10:");
    {
      static const char *label_SwitchCase = "switch case";
      const char *label = NULL;
      if (etd->charset) {
        if (isalpha(etd->character.byte)) label = label_SwitchCase;
      } else {
        if (iswalpha(etd->character.unicode)) label = label_SwitchCase;
      }
      if (label) printw(" %s", label);
    }
    printw("\n");

    DOT(3);
    DOT(7);
    printw("F11: %s", etd->updated? "save table": "");
    printw("\n");

    DOT(4);
    DOT(8);
    printw("F12: exit table editor");
    if (etd->updated) printw(" (unsaved changes)");
    printw("\n");
#undef DOT

    printw("\n");
#else /* standard input/output */
#endif /* write header */

    if (etd->charset) {
      printw("%02X: %s\n", etd->character.byte, etd->charset);
    }

    printCharacterString(description);
    printw("\n");

#define DOT(n) ((dots & BRLAPI_DOT##n)? '#': ' ')
    printw(" +---+ \n");
    printw("1|%c %c|4\n", DOT(1), DOT(4));
    printw("2|%c %c|5\n", DOT(2), DOT(5));
    printw("3|%c %c|6\n", DOT(3), DOT(6));
    printw("7|%c %c|8\n", DOT(7), DOT(8));
    printw(" +---+ \n");
#undef DOT

    if (etd->brlapiErrorFunction) {
      printw("BrlAPI error: %s: %s\n",
             etd->brlapiErrorFunction, etd->brlapiErrorMessage);
      setBrlapiError(etd, NULL);
    }

    refresh();

    if (haveBrailleDisplay(etd)) {
      brlapi_writeArguments_t args = BRLAPI_WRITEARGUMENTS_INITIALIZER;
      wchar_t text[etd->displayWidth];

      {
        size_t count = MIN(etd->displayWidth, length);
        wmemcpy(text, description, count);
        wmemset(&text[count], WC_C(' '), etd->displayWidth-count);
      }

      args.regionBegin = 1;
      args.regionSize = etd->displayWidth;
      args.text = (char *)text;
      args.textSize = etd->displayWidth * sizeof(text[0]);
      args.charset = "WCHAR_T";

      if (brlapi_write(&args) == -1) {
        setBrlapiError(etd, "brlapi_write");
        releaseBrailleDisplay(etd);
      }
    }

    free(description);
  }

  return ok;
}

static void
setPreviousActualCharacter (EditTableData *etd) {
  if (etd->charset) {
    etd->character.byte = (etd->character.byte - 1) & CHARSET_BYTE_MAXIMUM;
  } else {
    etd->character.unicode = (etd->character.unicode - 1) & UNICODE_CHARACTER_MASK;
  }
}

static void
setNextActualCharacter (EditTableData *etd) {
  if (etd->charset) {
    etd->character.byte = (etd->character.byte + 1) & CHARSET_BYTE_MAXIMUM;
  } else {
    etd->character.unicode = (etd->character.unicode + 1) & UNICODE_CHARACTER_MASK;
  }
}

static void
setFirstActualCharacter (EditTableData *etd) {
  if (etd->charset) {
    etd->character.byte = 0;
  } else {
    etd->character.unicode = 0;
  }
}

static void
setLastActualCharacter (EditTableData *etd) {
  if (etd->charset) {
    etd->character.byte = CHARSET_BYTE_MAXIMUM;
  } else {
    etd->character.unicode = UNICODE_CHARACTER_MASK;
  }
}

static int
findCharacter (EditTableData *etd, int backward) {
  const int increment = backward? -1: 1;

  if (etd->charset) {
    const int byteLimit = backward? 0: CHARSET_BYTE_MAXIMUM;
    const int byteReset = CHARSET_BYTE_MAXIMUM - byteLimit - increment;

    unsigned char byte = etd->character.byte;
    int counter = CHARSET_BYTE_COUNT;

    do {
      if (byte == byteLimit) byte = byteReset;
      byte += increment;

      {
        wint_t wc = convertCharToWchar(byte);

        if (wc != WEOF) {
          if (getUnicodeCellEntry(etd->ttd, wc)) {
            etd->character.byte = byte;
            return 1;
          }
        }       
      }
    } while ((counter -= 1) >= 0);
  } else {
    const int groupLimit = backward? 0: UNICODE_GROUP_MAXIMUM;
    const int plainLimit = backward? 0: UNICODE_PLAIN_MAXIMUM;
    const int rowLimit = backward? 0: UNICODE_ROW_MAXIMUM;
    const int cellLimit = backward? 0: UNICODE_CELL_MAXIMUM;

    const int groupReset = UNICODE_GROUP_MAXIMUM - groupLimit;
    const int plainReset = UNICODE_PLAIN_MAXIMUM - plainLimit;
    const int rowReset = UNICODE_ROW_MAXIMUM - rowLimit;
    const int cellReset = UNICODE_CELL_MAXIMUM - cellLimit - increment;

    int groupNumber = UNICODE_GROUP_NUMBER(etd->character.unicode);
    int plainNumber = UNICODE_PLAIN_NUMBER(etd->character.unicode);
    int rowNumber = UNICODE_ROW_NUMBER(etd->character.unicode);
    int cellNumber = UNICODE_CELL_NUMBER(etd->character.unicode);

    const TextTableHeader *header = getTextTableHeader(etd->ttd);
    int groupCounter = UNICODE_GROUP_COUNT;

    do {
      TextTableOffset groupOffset = header->unicodeGroups[groupNumber];

      if (groupOffset) {
        const UnicodeGroupEntry *group = getTextTableItem(etd->ttd, groupOffset);

        while (1) {
          TextTableOffset plainOffset = group->plains[plainNumber];

          if (plainOffset) {
            const UnicodePlainEntry *plain = getTextTableItem(etd->ttd, plainOffset);

            while (1) {
              TextTableOffset rowOffset = plain->rows[rowNumber];

              if (rowOffset) {
                const UnicodeRowEntry *row = getTextTableItem(etd->ttd, rowOffset);

                while (cellNumber != cellLimit) {
                  cellNumber += increment;

                  if (BITMASK_TEST(row->defined, cellNumber)) {
                    etd->character.unicode = UNICODE_CHARACTER(groupNumber, plainNumber, rowNumber, cellNumber);
                    return 1;
                  }
                }
              }

              cellNumber = cellReset;

              if (rowNumber == rowLimit) break;
              rowNumber += increment;
            }
          }

          rowNumber = rowReset;
          cellNumber = cellReset;

          if (plainNumber == plainLimit) break;
          plainNumber += increment;
        }
      }

      plainNumber = plainReset;
      rowNumber = rowReset;
      cellNumber = cellReset;

      if (groupNumber == groupLimit) {
        groupNumber = groupReset;
      } else {
        groupNumber += increment;
      }
    } while ((groupCounter -= 1) >= 0);
  }

  return 0;
}

static int
setPreviousDefinedCharacter (EditTableData *etd) {
  return findCharacter(etd, 1);
}

static int
setNextDefinedCharacter (EditTableData *etd) {
  return findCharacter(etd, 0);
}

static int
setFirstDefinedCharacter (EditTableData *etd) {
  setLastActualCharacter(etd);
  if (setNextDefinedCharacter(etd)) return 1;

  setFirstActualCharacter(etd);
  return 0;
}

static int
setLastDefinedCharacter (EditTableData *etd) {
  setFirstActualCharacter(etd);
  if (setPreviousDefinedCharacter(etd)) return 1;

  setLastActualCharacter(etd);
  return 0;
}

static int
setAlternateCharacter (EditTableData *etd) {
  if (etd->charset) {
    if (isalpha(etd->character.byte)) {
      if (islower(etd->character.byte)) {
        etd->character.byte = toupper(etd->character.byte);
        return 1;
      }

      if (isupper(etd->character.byte)) {
        etd->character.byte = tolower(etd->character.byte);
        return 1;
      }
    }
  } else {
    if (iswalpha(etd->character.unicode)) {
      if (iswlower(etd->character.unicode)) {
        etd->character.unicode = towupper(etd->character.unicode);
        return 1;
      }

      if (iswupper(etd->character.unicode)) {
        etd->character.unicode = towlower(etd->character.unicode);
        return 1;
      }
    }
  }

  return 0;
}

static int
toggleCharacter (EditTableData *etd) {
  wchar_t character;
  if (!getCharacter(etd, &character)) return 0;

  {
    const UnicodeCellEntry *cell = getUnicodeCellEntry(etd->ttd, character);
    if (cell && !cell->dots) {
      unsetTextTableCharacter(etd->ttd, character);
    } else if (!setTextTableCharacter(etd->ttd, character, 0)) {
      return 0;
    }
  }

  etd->updated = 1;
  return 1;
}

static int
toggleDot (EditTableData *etd, unsigned char dot) {
  wchar_t character;

  if (getCharacter(etd, &character)) {
    const UnicodeCellEntry *cell = getUnicodeCellEntry(etd->ttd, character);
    unsigned char dots = cell? cell->dots: 0;

    if (setTextTableCharacter(etd->ttd, character, dots^dot)) {
      etd->updated = 1;
      return 1;
    }
  }

  return 0;
}

static int
setDots (EditTableData *etd, unsigned char dots) {
  wchar_t character;

  if (getCharacter(etd, &character)) {
    if (setTextTableCharacter(etd->ttd, character, dots)) {
      etd->updated = 1;
      return 1;
    }
  }

  return 0;
}

static int
saveTable (EditTableData *etd) {
  int ok = 0;
  FILE *outputFile;

  if (!outputPath) outputPath = inputPath;
  if (!outputFormat) outputFormat = inputFormat;

  if ((outputFile = openTable(&outputPath, "w", NULL, stdout, "<standard-output>"))) {
    if (outputFormat->write(outputPath, outputFile, etd->ttd, outputFormat->data)) {
      ok = 1;
      etd->updated = 0;
    }

    fclose(outputFile);
  }

  return ok;
}

static int
doKeyboardCommand (EditTableData *etd) {
#undef IS_UNICODE_CHARACTER
#if defined(USE_CURSES)
#ifdef USE_FUNC_GET_WCH
#define IS_UNICODE_CHARACTER
  wint_t ch;
  int ret = get_wch(&ch);

  if (ret == KEY_CODE_YES)
#else /* USE_FUNC_GET_WCH */
  int ch = getch();

  if (ch >= 0X100)
#endif /* USE_FUNC_GET_WCH */
  {
    switch (ch) {
      case KEY_LEFT:
        setPreviousActualCharacter(etd);
        break;

      case KEY_RIGHT:
        setNextActualCharacter(etd);
        break;

      case KEY_UP:
        if (!setPreviousDefinedCharacter(etd)) beep();
        break;

      case KEY_DOWN:
        if (!setNextDefinedCharacter(etd)) beep();;
        break;

      case KEY_HOME:
        if (!setFirstDefinedCharacter(etd)) beep();
        break;

      case KEY_END:
        if (!setLastDefinedCharacter(etd)) beep();
        break;

      case KEY_F(1):
        if (!toggleDot(etd, BRLAPI_DOT1)) beep();
        break;

      case KEY_F(2):
        if (!toggleDot(etd, BRLAPI_DOT2)) beep();
        break;

      case KEY_F(3):
        if (!toggleDot(etd, BRLAPI_DOT3)) beep();
        break;

      case KEY_F(4):
        if (!toggleDot(etd, BRLAPI_DOT4)) beep();
        break;

      case KEY_F(5):
        if (!toggleDot(etd, BRLAPI_DOT5)) beep();
        break;

      case KEY_F(6):
        if (!toggleDot(etd, BRLAPI_DOT6)) beep();
        break;

      case KEY_F(7):
        if (!toggleDot(etd, BRLAPI_DOT7)) beep();
        break;

      case KEY_F(8):
        if (!toggleDot(etd, BRLAPI_DOT8)) beep();
        break;

      case KEY_F(9):
        if (!toggleCharacter(etd)) beep();
        break;

      case KEY_F(10):
        if (!setAlternateCharacter(etd)) beep();
        break;

      case KEY_F(11):
        if (!(etd->updated && saveTable(etd))) beep();
        break;

      case KEY_F(12):
        return 0;

      default:
        beep();
        break;
    }
  } else

#ifdef USE_FUNC_GET_WCH
  if (ret == OK)
#endif /* USE_FUNC_GET_WCH */
#else /* standard input/output */
#define IS_UNICODE_CHARACTER
  wint_t ch = fgetwc(stdin);
  if (ch == WEOF) return 0;
#endif /* read character */

  {
    wint_t character;

#ifdef IS_UNICODE_CHARACTER
    character = ch;
#else /* IS_UNICODE_CHARACTER */
    character = convertCharToWchar(ch);
#endif /* IS_UNICODE_CHARACTER */

    if ((character >= UNICODE_BRAILLE_ROW) &&
        (character <= (UNICODE_BRAILLE_ROW | UNICODE_CELL_MASK))) {
      if (!setDots(etd, character & UNICODE_CELL_MASK)) beep();
    } else {
      if (etd->charset) {
        int c;

#ifdef IS_UNICODE_CHARACTER
        c = convertWcharToChar(ch);
#else /* IS_UNICODE_CHARACTER */
        c = ch;
#endif /* IS_UNICODE_CHARACTER */

        if (c != EOF) {
          etd->character.byte = c;
        } else {
          beep();
        }
      } else if (character != WEOF) {
        etd->character.unicode = character;
      } else {
        beep();
      }
    }
  }

  return 1;
}

static int
doBrailleCommand (EditTableData *etd) {
  if (haveBrailleDisplay(etd)) {
    brlapi_keyCode_t key;
    int ret = brlapi_readKey(0, &key);

    if (ret == 1) {
      unsigned long code = key & BRLAPI_KEY_CODE_MASK;

      switch (key & BRLAPI_KEY_TYPE_MASK) {
        case BRLAPI_KEY_TYPE_CMD:
          switch (code & BRLAPI_KEY_CMD_BLK_MASK) {
            case 0:
              switch (code) {
                case BRLAPI_KEY_CMD_FWINLT:
                  setPreviousActualCharacter(etd);
                  break;

                case BRLAPI_KEY_CMD_FWINRT:
                  setNextActualCharacter(etd);
                  break;

                case BRLAPI_KEY_CMD_LNUP:
                  if (!setPreviousDefinedCharacter(etd)) beep();
                  break;

                case BRLAPI_KEY_CMD_LNDN:
                  if (!setNextDefinedCharacter(etd)) beep();
                  break;

                case BRLAPI_KEY_CMD_TOP_LEFT:
                case BRLAPI_KEY_CMD_TOP:
                  if (!setFirstDefinedCharacter(etd)) beep();
                  break;

                case BRLAPI_KEY_CMD_BOT_LEFT:
                case BRLAPI_KEY_CMD_BOT:
                  if (!setLastDefinedCharacter(etd)) beep();
                  break;

                default:
                  beep();
                  break;
              }
              break;

            case BRLAPI_KEY_CMD_PASSDOTS:
              if (!setDots(etd, code & BRLAPI_KEY_CMD_ARG_MASK)) beep();
              break;

            default:
              beep();
              break;
          }
          break;

        case BRLAPI_KEY_TYPE_SYM: {
          /* latin1 */
          if (code < 0X100) code |= BRLAPI_KEY_SYM_UNICODE;

          if ((code & 0X1f000000) == BRLAPI_KEY_SYM_UNICODE) {
            /* unicode */
            if ((code & 0Xffff00) == UNICODE_BRAILLE_ROW) {
              /* Set braille pattern */
              if (!setDots(etd, code & UNICODE_CELL_MASK)) beep();
            } else {
              wchar_t character = code & 0XFFFFFF;

              if (etd->charset) {
                int c = convertWcharToChar(character);

                if (c != EOF) {
                  etd->character.byte = c;
                } else {
                  beep();
                }
              } else {
                etd->character.unicode = character;
              }
            }
          } else {
            switch (code) {
              case BRLAPI_KEY_SYM_LEFT:
                setPreviousActualCharacter(etd);
                break;

              case BRLAPI_KEY_SYM_RIGHT:
                setNextActualCharacter(etd);
                break;

              case BRLAPI_KEY_SYM_UP:
                if (!setPreviousDefinedCharacter(etd)) beep();
                break;

              case BRLAPI_KEY_SYM_DOWN:
                if (!setNextDefinedCharacter(etd)) beep();
                break;

              case BRLAPI_KEY_SYM_HOME:
                if (!setFirstDefinedCharacter(etd)) beep();
                break;

              case BRLAPI_KEY_SYM_END:
                if (!setLastDefinedCharacter(etd)) beep();
                break;

              default:
                beep();
                break;
            }
          }
          break;
        }

        default:
          beep();
          break;
      }
    } else if (ret == -1) {
      setBrlapiError(etd, "brlapi_readKey");
      releaseBrailleDisplay(etd);
    }
  }

  return 1;
}

static int
editTable (void) {
  int status;
  EditTableData etd;

  etd.ttd = NULL;
  etd.updated = 0;

  {
    FILE *inputFile = openTable(&inputPath, "r", opt_tablesDirectory, NULL, NULL);

    if (inputFile) {
      if ((etd.ttd = inputFormat->read(inputPath, inputFile, inputFormat->data))) {
        status = 0;
      } else {
        status = 4;
      }

      fclose(inputFile);
    } else {
      status = 3;
    }
  }

  if (!status) {
    claimBrailleDisplay(&etd);

#if defined(USE_CURSES)
    initscr();
    cbreak();
    keypad(stdscr, TRUE);
    noecho();
    nonl();
    intrflush(stdscr, FALSE);
#else /* standard input/output */
#endif /* initialize keyboard and screen */

    etd.charset = *opt_charset? opt_charset: NULL;
    setFirstDefinedCharacter(&etd);

    while (updateCharacterDescription(&etd)) {
      fd_set set;
      FD_ZERO(&set);

      {
        int maximumFileDescriptor = STDIN_FILENO;
        FD_SET(STDIN_FILENO, &set);

        if (haveBrailleDisplay(&etd)) {
          FD_SET(etd.brlapiFileDescriptor, &set);
          if (etd.brlapiFileDescriptor > maximumFileDescriptor)
            maximumFileDescriptor = etd.brlapiFileDescriptor;
        }

        select(maximumFileDescriptor+1, &set, NULL, NULL, NULL);
      }

      if (FD_ISSET(STDIN_FILENO, &set))
        if (!doKeyboardCommand(&etd))
          break;

      if (haveBrailleDisplay(&etd) && FD_ISSET(etd.brlapiFileDescriptor, &set))
        if (!doBrailleCommand(&etd))
          break;
    }

    erase();
    refresh();

#if defined(USE_CURSES)
    endwin();
#else /* standard input/output */
#endif /* restore keyboard and screen */

    if (haveBrailleDisplay(&etd)) releaseBrailleDisplay(&etd);
    if (etd.ttd) destroyTextTableData(etd.ttd);
  }

  return status;
}
#endif /* ENABLE_API */

int
main (int argc, char *argv[]) {
  int status;

  {
    static const OptionsDescriptor descriptor = {
      OPTION_TABLE(programOptions),
      .applicationName = "ttbtest",
      .argumentsSummary = "input-table [output-table]"
    };
    processOptions(&descriptor, &argc, &argv);
  }

  {
    char **const paths[] = {
      &opt_tablesDirectory,
      NULL
    };
    fixInstallPaths(paths);
  }

  if (argc == 0) {
    LogPrint(LOG_ERR, "missing input table.");
    exit(2);
  }
  inputPath = *argv++, argc--;

  if (argc > 0) {
    outputPath = *argv++, argc--;
  } else if (opt_outputFormat && *opt_outputFormat) {
    const char *extension = locatePathExtension(inputPath);
    int prefix = extension? (extension - inputPath): strlen(inputPath);
    char buffer[prefix + 1 + strlen(opt_outputFormat) + 1];
    snprintf(buffer, sizeof(buffer), "%.*s.%s", prefix, inputPath, opt_outputFormat);
    outputPath = strdupWrapper(buffer);
  } else {
    outputPath = NULL;
  }

  if (argc > 0) {
    LogPrint(LOG_ERR, "too many parameters.");
    exit(2);
  }

  inputFormat = getFormatEntry(opt_inputFormat, inputPath, "input");
  if (outputPath) {
    outputFormat = getFormatEntry(opt_outputFormat, outputPath, "output");
  } else {
    outputFormat = NULL;
  }

  if (*opt_charset && !setCharset(opt_charset)) {
    LogPrint(LOG_ERR, "can't establish character set: %s", opt_charset);
    exit(9);
  }

#ifdef ENABLE_API
  if (opt_edit) {
    status = editTable();
  } else
#endif /* ENABLE_API */

  {
    status = convertTable();
  }

  return status;
}
