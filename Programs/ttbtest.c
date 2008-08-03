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
#include <ctype.h>

#ifdef HAVE_ICU
#include <unicode/uchar.h>
#endif /* HAVE_ICU */

#include "program.h"
#include "options.h"
#include "misc.h"
#include "brl.h"
#include "charset.h"
#include "datafile.h"
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

static const DotsTable dotsInternal = {
  BRL_DOT1, BRL_DOT2, BRL_DOT3, BRL_DOT4,
  BRL_DOT5, BRL_DOT6, BRL_DOT7, BRL_DOT8
};

static const DotsTable dots12345678 = {
  0X01, 0X02, 0X04, 0X08, 0X10, 0X20, 0X40, 0X80
};

static const DotsTable dots14253678 = {
  0X01, 0X04, 0X10, 0X02, 0X08, 0X20, 0X40, 0X80
};

static unsigned char
mapDots (unsigned char input, const DotsTable from, const DotsTable to) {
  unsigned char output = 0;
  {
    int dot;
    for (dot=0; dot<DOTS_TABLE_SIZE; ++dot) {
      if (input & from[dot]) output |= to[dot];
    }
  }
  return output;
}

typedef TextTable *TableReader (const char *path, FILE *file, void *data);
typedef int TableWriter (const char *path, FILE *file, TextTable *table, void *data);
typedef int ByteWriter (FILE *file, unsigned char byte, unsigned char dots, void *data);
typedef int CharacterWriter (FILE *file, wchar_t character, unsigned char dots, void *data);

static int
writeBytes (FILE *file, TextTable *table, ByteWriter writer, void *data) {
  const TextTableHeader *header = table->header.fields;
  int byte;

  for (byte=0; byte<BYTES_PER_CHARSET; byte+=1)
    if (BITMASK_TEST(header->byteDotsDefined, byte))
      if (!writer(file, byte, header->byteToDots[byte], data))
        return 0;

  return 1;
}

static int
writeCharacters (FILE *file, TextTable *table, CharacterWriter writer, void *data) {
  const TextTableHeader *header = table->header.fields;
  unsigned int groupNumber;

  for (groupNumber=0; groupNumber<UNICODE_GROUP_COUNT; groupNumber+=1) {
    TextTableOffset groupOffset = header->unicodeGroups[groupNumber];

    if (groupOffset) {
      const UnicodeGroupEntry *group = getTextTableItem(table, groupOffset);
      unsigned int plainNumber;

      for (plainNumber=0; plainNumber<UNICODE_PLAINS_PER_GROUP; plainNumber+=1) {
        TextTableOffset plainOffset = group->plains[plainNumber];

        if (plainOffset) {
          const UnicodePlainEntry *plain = getTextTableItem(table, plainOffset);
          unsigned int rowNumber;

          for (rowNumber=0; rowNumber<UNICODE_ROWS_PER_PLAIN; rowNumber+=1) {
            TextTableOffset rowOffset = plain->rows[rowNumber];

            if (rowOffset) {
              const UnicodeRowEntry *row = getTextTableItem(table, rowOffset);
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

static TextTable *
readTable_native (const char *path, FILE *file, void *data) {
  return compileTextTable_native(path, file);
}

static int
writeDots_native (FILE *file, unsigned char dots) {
  int dot;

  if (fprintf(file, "(") == EOF) return 0;
  for (dot=0; dot<BRL_DOT_COUNT; dot+=1) {
    wchar_t character = (dots & brlDotBits[dot])? brlDotNumbers[dot]: WC_C(' ');
    if (fprintf(file, "%.1" PRIws, &character) == EOF) return 0;
  }
  if (fprintf(file, ")") == EOF) return 0;

  return 1;
}

static int
writeByte_native (FILE *file, unsigned char byte, unsigned char dots, void *data) {
  if (fprintf(file, "byte \\x%02X ", byte) == EOF) return 0;
  if (!writeDots_native(file, dots)) return 0;
  if (fprintf(file, "\n") == EOF) return 0;
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

  if (fprintf(file, "\n") == EOF) goto error;
  return 1;

error:
  return 0;
}

static int
writeTable_native (const char *path, FILE *file, TextTable *table, void *data) {
  if (fprintf(file, "# generated by %s\n", programName) == EOF) goto error;

  {
    const char *charset = getCharset();
    if (charset)
      if (fprintf(file, "# charset: %s\n", charset) == EOF)
        goto error;
  }

  if (!writeBytes(file, table, writeByte_native, data)) return 0;
  if (!writeCharacters(file, table, writeCharacter_native, data)) return 0;
  return 1;

error:
  return 0;
}

static TextTable *
readTable_binary (const char *path, FILE *file, void *data) {
  TextTable *table = NULL;
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
      if (!setTextTableByte(byte, dots, ttd)) break;
    }

    if (byte == count) {
      table = newTextTable(ttd);
    }

    destroyTextTableData(ttd);
  }

  return table;
}

static int
writeTable_binary (const char *path, FILE *file, TextTable *table, void *data) {
  const TextTableHeader *header = table->header.fields;
  int byte;

  for (byte=0; byte<0X100; byte+=1) {
    unsigned char dots;

    if (BITMASK_TEST(header->byteDotsDefined, byte)) {
      dots = header->byteToDots[byte];
    } else {
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
//{"gnb", readTable_Gnome, writeTable_Gnome, NULL},
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
      TextTable *table;

      if ((table = inputFormat->read(inputPath, inputFile, inputFormat->data))) {
        if (outputPath) {
          FILE *outputFile = openTable(&outputPath, "w", NULL, stdout, "<standard-output>");

          if (outputFile) {
            if (outputFormat->write(outputPath, outputFile, table, outputFormat->data)) {
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

        destroyTextTable(table);
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

  if (opt_characterSet && !setCharset(opt_characterSet)) {
    LogPrint(LOG_ERR, "can't establish character set: %s", opt_characterSet);
    exit(9);
  }

#ifdef ENABLE_API
  if (opt_edit) {
//  status = editTable();
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
