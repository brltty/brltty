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

#include <stdio.h>
#include <string.h>
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

static char *opt_characterSet;
static char *opt_inputFormat;
static char *opt_outputFormat;
static int opt_translate;
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

  { .letter = 't',
    .word = "translate",
    .setting.flag = &opt_translate,
    .description = "Translate."
  },

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
    .word = "character-set",
    .argument = "charset",
    .setting.string = &opt_characterSet,
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
typedef int CharacterWriter (FILE *file, wchar_t character, unsigned char dots, void *data);

static unsigned char
getDots (TextTableData *ttd, wchar_t character) {
  const UnicodeCellEntry *cell;
  if ((cell = getUnicodeCellEntry(ttd, character))) return cell->dots;
  if ((cell = getUnicodeCellEntry(ttd, UNICODE_REPLACEMENT_CHARACTER))) return cell->dots;
  if ((cell = getUnicodeCellEntry(ttd, WC_C('?')))) return cell->dots;
  return 0;
}

static int
writeCharacters (FILE *file, TextTableData *ttd, CharacterWriter writer, void *data) {
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
                  wchar_t character = (groupNumber << UNICODE_GROUP_SHIFT) |
                                      (plainNumber << UNICODE_PLAIN_SHIFT) |
                                      (rowNumber   << UNICODE_ROW_SHIFT) |
                                      (cellNumber  << UNICODE_CELL_SHIFT);

                  if (!writer(file, character, row->cells[cellNumber].dots, data)) return 0;
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

static TextTableData *
readTable_native (const char *path, FILE *file, void *data) {
  return processTextTableStream(file, path, processTextTableLine);
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
writeCharacter_native (FILE *file, wchar_t character, unsigned char dots, void *data) {
  uint32_t value = character;

  if (fprintf(file, "char ") == EOF) goto error;

  if (value < 0X100) {
    if (fprintf(file, "\\x%02X", value) == EOF) goto error;
  } else if (value < 0X10000) {
    if (fprintf(file, "\\u%04X", value) == EOF) goto error;
  } else {
    if (fprintf(file, "\\U%08X", value) == EOF) goto error;
  }

  if (fprintf(file, " ") == EOF) goto error;
  if (!writeDots_native(file, dots)) goto error;

#ifdef HAVE_ICU
  {
    char name[0X40];
    UErrorCode error = U_ZERO_ERROR;

    u_charName(character, U_EXTENDED_CHAR_NAME, name, sizeof(name), &error);
    if (U_SUCCESS(error)) {
      if (fprintf(file, " %s", name) == EOF) return 0;
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
  if (fprintf(file, "# generated by %s\n", programName) == EOF) goto error;

  {
    const char *charset = getCharset();
    if (charset)
      if (fprintf(file, "# charset: %s\n", charset) == EOF)
        goto error;
  }

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
      dots = getDots(ttd, character);
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
  return processTextTableStream(file, path, processGnomeBrailleLine);
}

static int
writeCharacter_gnome (FILE *file, wchar_t character, unsigned char dots, void *data) {
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
    if (fprintf(file, "UNICODE-CHAR U+%04" PRIx32 " U+%04" PRIx32" \n", c, p) == EOF) return 0;
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

  if (outputFormat != inputFormat) {
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
  } else {
    LogPrint(LOG_ERR, "same input and output formats: %s", outputFormat->name);
    status = 2;
  }

  return status;
}

#ifdef ENABLE_API
#define BRLAPI_NO_DEPRECATED
#include "brlapi.h"

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
#else
#warning curses package either unspecified or unsupported
#define printw printf
#define clear() printf("\r\n\v")
#define refresh() fflush(stdout)
#define beep() printf("\a")
#endif /* HAVE_PKG_CURSES */

static wchar_t *
makeCharacterDescription (TextTableData *ttd, wchar_t character, size_t *length, unsigned char *braille) {
  char buffer[0X100];
  int characterIndex;
  int brailleIndex;
  int descriptionLength;

  unsigned char dots = getDots(ttd, character);
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

#define DOT(n) ((dots & BRLAPI_DOT##n)? ((n) + '0'): ' ')
  {
    uint32_t value = character;

    snprintf(buffer, sizeof(buffer),
             "%04" PRIx32 " %c%nx %nx [%c%c%c%c%c%c%c%c]%n",
             value, printablePrefix, &characterIndex, &brailleIndex,
             DOT(1), DOT(2), DOT(3), DOT(4), DOT(5), DOT(6), DOT(7), DOT(8),
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
        description[i] = (wc != WEOF)? wc: WC_C(' ');
      }

      description[characterIndex] = printableCharacter;
      description[brailleIndex] = UNICODE_BRAILLE_ROW | dots;
      description[descriptionLength] = 0;

      if (length) *length = descriptionLength;
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

typedef struct {
  TextTableData *ttd;
  wchar_t currentCharacter;

  brlapi_fileDescriptor brlapiFileDescriptor;
  unsigned int displayWidth;
  unsigned int displayHeight;
  const char *brlapiErrorFunction;
  const char *brlapiErrorMessage;
} EditTableData;

static int
haveBrailleDisplay (EditTableData *data) {
  return data->brlapiFileDescriptor != (brlapi_fileDescriptor)-1;
}

static void
setBrlapiError (EditTableData *data, const char *function) {
  if ((data->brlapiErrorFunction = function)) {
    data->brlapiErrorMessage = brlapi_strerror(&brlapi_error);
  } else {
    data->brlapiErrorMessage = NULL;
  }
}

static void
releaseBrailleDisplay (EditTableData *data) {
  brlapi_closeConnection();
  data->brlapiFileDescriptor = (brlapi_fileDescriptor)-1;
}

static int
claimBrailleDisplay (EditTableData *data) {
  data->brlapiFileDescriptor = brlapi_openConnection(NULL, NULL);
  if (haveBrailleDisplay(data)) {
    if (brlapi_getDisplaySize(&data->displayWidth, &data->displayHeight) != -1) {
      if (brlapi_enterTtyMode(BRLAPI_TTY_DEFAULT, NULL) != -1) {
        setBrlapiError(data, NULL);
        return 1;
      } else {
        setBrlapiError(data, "brlapi_enterTtyMode");
      }
    } else {
      setBrlapiError(data, "brlapi_getDisplaySize");
    }

    releaseBrailleDisplay(data);
  } else {
    setBrlapiError(data, "brlapi_openConnection");
  }

  return 0;
}

static int
updateCharacterDescription (EditTableData *data) {
  int ok = 0;

  size_t descriptionLength;
  unsigned char descriptionDots;
  wchar_t *descriptionText = makeCharacterDescription(
    data->ttd,
    data->currentCharacter,
    &descriptionLength,
    &descriptionDots
  );

  if (descriptionText) {
    ok = 1;
    clear();

#if defined(USE_CURSES)
    printw("F2:Save F8:Exit\n");
    printw("Up:Up1 Dn:Down1 PgUp:Up16 PgDn:Down16 Home:First End:Last\n");
    printw("\n");
#else /* standard input/output */
#endif /* clear screen */

    printCharacterString(descriptionText);
    printw("\n");

#define DOT(n) ((descriptionDots & BRLAPI_DOT##n)? '#': ' ')
    printw(" +---+ \n");
    printw("1|%c %c|4\n", DOT(1), DOT(4));
    printw("2|%c %c|5\n", DOT(2), DOT(5));
    printw("3|%c %c|6\n", DOT(3), DOT(6));
    printw("7|%c %c|8\n", DOT(7), DOT(8));
    printw(" +---+ \n");
#undef DOT

    if (data->brlapiErrorFunction) {
      printw("BrlAPI error: %s: %s\n",
             data->brlapiErrorFunction, data->brlapiErrorMessage);
      setBrlapiError(data, NULL);
    }

    refresh();

    if (haveBrailleDisplay(data)) {
      brlapi_writeArguments_t args = BRLAPI_WRITEARGUMENTS_INITIALIZER;
      wchar_t text[data->displayWidth];
      size_t length = MIN(data->displayWidth, descriptionLength);

      wmemcpy(text, descriptionText, length);
      wmemset(&text[length], WC_C(' '), data->displayWidth-length);

      args.regionBegin = 1;
      args.regionSize = data->displayWidth;
      args.text = (char *)text;
      args.textSize = data->displayWidth * sizeof(text[0]);
      args.charset = "WCHAR_T";

      if (brlapi_write(&args) == -1) {
        setBrlapiError(data, "brlapi_write");
        releaseBrailleDisplay(data);
      }
    }

    free(descriptionText);
  }

  return ok;
}

static int
doKeyboardCommand (EditTableData *data) {
#if defined(USE_CURSES)
#ifdef USE_FUNC_GET_WCH
  wint_t ch;
  int ret = get_wch(&ch);

  if (ret == KEY_CODE_YES)
#else /* USE_FUNC_GET_WCH */
  int ch = getch();

  if (ch >= 0X100)
#endif /* USE_FUNC_GET_WCH */
  {
    switch (ch) {
      case KEY_UP:
        data->currentCharacter -= 1;
        break;

      case KEY_DOWN:
        data->currentCharacter += 1;
        break;

      case KEY_PPAGE:
        data->currentCharacter -= 0X10;
        break;

      case KEY_NPAGE:
        data->currentCharacter += 0X10;
        break;

      case KEY_HOME:
        data->currentCharacter = 0;
        break;

      case KEY_END:
        data->currentCharacter = 0XFF;
        break;

      case KEY_F(2): {
        FILE *outputFile;

        if (!outputPath) outputPath = inputPath;
        if (!outputFormat) outputFormat = inputFormat;
        outputFile = openTable(&outputPath, "w", NULL, stdout, "<standard-output>");

        if (outputFile) {
          outputFormat->write(outputPath, outputFile, data->ttd, outputFormat->data);
          fclose(outputFile);
        }

        break;
      }

      case KEY_F(8):
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
  wint_t ch = fgetwc(stdin);
  if (ch == WEOF) return 0;
#endif /* read character */
  {
    if ((ch >= UNICODE_BRAILLE_ROW) &&
        (ch <= (UNICODE_BRAILLE_ROW | UNICODE_CELL_MASK))) {
      /* Set braille pattern */
      setTextTableCharacter(data->ttd, data->currentCharacter, ch & 0XFF);
    } else {
      /* Switch to char */
      int c = convertWcharToChar(ch);
      if (c != EOF) {
        data->currentCharacter = c;
      } else {
        beep();
      }
    }
  }

  return 1;
}

static int
doBrailleCommand (EditTableData *data) {
  if (haveBrailleDisplay(data)) {
    brlapi_keyCode_t key;
    int ret = brlapi_readKey(0, &key);

    if (ret == 1) {
      unsigned long code = key & BRLAPI_KEY_CODE_MASK;

      switch (key & BRLAPI_KEY_TYPE_MASK) {
        case BRLAPI_KEY_TYPE_CMD:
          switch (code & BRLAPI_KEY_CMD_BLK_MASK) {
            case 0:
              switch (code) {
                case BRLAPI_KEY_CMD_LNUP:
                  data->currentCharacter -= 1;
                  break;

                case BRLAPI_KEY_CMD_LNDN:
                  data->currentCharacter += 1;
                  break;

                case BRLAPI_KEY_CMD_PRPGRPH:
                  data->currentCharacter -= 0X10;
                  break;

                case BRLAPI_KEY_CMD_NXPGRPH:
                  data->currentCharacter += 0X10;
                  break;

                case BRLAPI_KEY_CMD_TOP_LEFT:
                case BRLAPI_KEY_CMD_TOP:
                  data->currentCharacter = 0;
                  break;

                case BRLAPI_KEY_CMD_BOT_LEFT:
                case BRLAPI_KEY_CMD_BOT:
                  data->currentCharacter = 0XFF;
                  break;

                default:
                  beep();
                  break;
              }
              break;

            case BRLAPI_KEY_CMD_PASSDOTS:
              setTextTableCharacter(data->ttd, data->currentCharacter, code & BRLAPI_KEY_CMD_ARG_MASK);
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
              setTextTableCharacter(data->ttd, data->currentCharacter, code & 0XFF);
            } else {
              /* Switch to char */
              int c = convertWcharToChar(code & 0XFFFFFF);
              if (c != EOF) {
                data->currentCharacter = c;
              } else {
                beep();
              }
            }
          } else {
            switch (code) {
              case BRLAPI_KEY_SYM_UP:
                data->currentCharacter -= 1;
                break;

              case BRLAPI_KEY_SYM_DOWN:
                data->currentCharacter += 1;
                break;

              case BRLAPI_KEY_SYM_PAGE_UP:
                data->currentCharacter -= 0X10;
                break;

              case BRLAPI_KEY_SYM_PAGE_DOWN:
                data->currentCharacter += 0X10;
                break;

              case BRLAPI_KEY_SYM_HOME:
                data->currentCharacter = 0;
                break;

              case BRLAPI_KEY_SYM_END:
                data->currentCharacter = 0XFF;
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
      setBrlapiError(data, "brlapi_readKey");
      releaseBrailleDisplay(data);
    }
  }

  return 1;
}

static int
editTable (void) {
  int status;
  EditTableData data;

  data.ttd = NULL;
  {
    FILE *inputFile = openTable(&inputPath, "r", opt_tablesDirectory, NULL, NULL);

    if (inputFile) {
      if ((data.ttd = inputFormat->read(inputPath, inputFile, inputFormat->data))) {
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
    claimBrailleDisplay(&data);

#if defined(USE_CURSES)
    initscr();
    cbreak();
    noecho();
    nonl();
    intrflush(stdscr, FALSE);
    keypad(stdscr, TRUE);
#else /* standard input/output */
#endif /* initialize keyboard and screen */

    data.currentCharacter = 0;
    while (updateCharacterDescription(&data)) {
      fd_set set;
      FD_ZERO(&set);

      {
        int maximumFileDescriptor = STDIN_FILENO;
        FD_SET(STDIN_FILENO, &set);

        if (haveBrailleDisplay(&data)) {
          FD_SET(data.brlapiFileDescriptor, &set);
          if (data.brlapiFileDescriptor > maximumFileDescriptor)
            maximumFileDescriptor = data.brlapiFileDescriptor;
        }

        select(maximumFileDescriptor+1, &set, NULL, NULL, NULL);
      }

      if (FD_ISSET(STDIN_FILENO, &set))
        if (!doKeyboardCommand(&data))
          break;

      if (haveBrailleDisplay(&data) && FD_ISSET(data.brlapiFileDescriptor, &set))
        if (!doBrailleCommand(&data))
          break;
    }

    clear();
    refresh();

#if defined(USE_CURSES)
    endwin();
#else /* standard input/output */
#endif /* restore keyboard and screen */

    if (haveBrailleDisplay(&data)) releaseBrailleDisplay(&data);
    if (data.ttd) destroyTextTableData(data.ttd);
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

  if (*opt_characterSet && !setCharset(opt_characterSet)) {
    LogPrint(LOG_ERR, "can't establish character set: %s", opt_characterSet);
    exit(9);
  }

#ifdef ENABLE_API
  if (opt_edit) {
    status = editTable();
  } else
#endif /* ENABLE_API */

  if (opt_translate) {
//  status = translateText();
  } else

  {
    status = convertTable();
  }

  return status;
}
