/*
 * BRLTTY - A background process providing access to the Linux console (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2002 by The BRLTTY Team. All rights reserved.
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

/* tbl2txt - translate Braille dot-table file to a readable form
 * James Bowden
 * $Id: tbl2txt.c,v 1.2 1996/09/21 23:34:51 nn201 Exp $
 * March 1996
 * Reworked by Dave Mielke <dave@mielke.cc> (November 2001)
 * Version 1.1
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>

#include "Programs/options.h"
#include "Programs/brl.h"
#include "Unicode/unicode.h"

BEGIN_OPTION_TABLE
  {'c', "code-page", "name", NULL, 0,
   "Code page to use for identifying characters."},
END_OPTION_TABLE

static const char *opt_codePageName = NULL;

static int
handleOption (const int option) {
  switch (option) {
    default:
      return 0;
    case 'c':
      opt_codePageName = optarg;
      break;
  }
  return 1;
}

/* Dot values for each byte in the table. */
static unsigned char dotTable[] = {B1, B2, B3, B4, B5, B6, B7, B8};
static unsigned char dotOrder[8] = {6, 2, 1, 0, 3, 4, 5, 7};

static char *characterNames[0X100] = {
  "NUL",
  "SOH",
  "STX",
  "ETX",
  "EOT",
  "ENQ",
  "ACK",
  "BEL",
  "BS",
  "HT",
  "LF",
  "VT",
  "FF",
  "CR",
  "SO",
  "SI",
  "DLE",
  "DC1",
  "DC2",
  "DC3",
  "DC4",
  "NAK",
  "SYN",
  "ETB",
  "CAN",
  "EM",
  "SUB",
  "ESC",
  "FS",
  "GS",
  "RS",
  "US",
  "space",
  "exclamation",
  "quotedouble",
  "number",
  "dollar",
  "percent",
  "ampersand",
  "quoteright",
  "parenleft",
  "parenright",
  "asterisk",
  "plus",
  "comma",
  "minus",
  "period",
  "slash",
  "zero",
  "one",
  "two",
  "three",
  "four",
  "five",
  "six",
  "seven",
  "eight",
  "nine",
  "colon",
  "semicolon",
  "less",
  "equal",
  "greater",
  "question",
  "at",
  "A",
  "B",
  "C",
  "D",
  "E",
  "F",
  "G",
  "H",
  "I",
  "J",
  "K",
  "L",
  "M",
  "N",
  "O",
  "P",
  "Q",
  "R",
  "S",
  "T",
  "U",
  "V",
  "W",
  "X",
  "Y",
  "Z",
  "bracketleft",
  "backslash",
  "bracketright",
  "asciicircumflex",
  "underscore",
  "quoteleft",
  "a",
  "b",
  "c",
  "d",
  "e",
  "f",
  "g",
  "h",
  "i",
  "j",
  "k",
  "l",
  "m",
  "n",
  "o",
  "p",
  "q",
  "r",
  "s",
  "t",
  "u",
  "v",
  "w",
  "x",
  "y",
  "z",
  "braceleft",
  "barsolid",
  "braceright",
  "asciitilde",
  "DEL",
  "<80>",
  "<81>",
  "BPH",
  "NBH",
  "<84>",
  "NL",
  "SSA",
  "ESA",
  "CTS",
  "CTJ",
  "LTS",
  "PLD",
  "PLU",
  "RLF",
  "SS2",
  "SS3",
  "DCS",
  "PU1",
  "PU2",
  "STS",
  "CC",
  "MW",
  "SGA",
  "EGA",
  "SS",
  "<99>",
  "SCI",
  "CSI",
  "ST",
  "OSC",
  "PM",
  "APC",
  "space",
  "exclamdown",
  "cent",
  "sterling",
  "currency",
  "yen",
  "brokenbar",
  "section",
  "dieresis",
  "copyright",
  "ordfeminine",
  "guillemotleft",
  "logicalnot",
  "hyphen",
  "registered",
  "macron",
  "degree",
  "plusminus",
  "twosuperior",
  "threesuperior",
  "acute",
  "mu",
  "paragraph",
  "periodcentered",
  "cedilla",
  "onesuperior",
  "ordmasculine",
  "guillemotright",
  "onequarter",
  "onehalf",
  "threequarters",
  "questiondown",
  "Agrave",
  "Aacute",
  "Acircumflex",
  "Atilde",
  "Adieresis",
  "Aring",
  "AE",
  "Ccedilla",
  "Egrave",
  "Eacute",
  "Ecircumflex",
  "Edieresis",
  "Igrave",
  "Iacute",
  "Icircumflex",
  "Idieresis",
  "Eth",
  "Ntilde",
  "Ograve",
  "Oacute",
  "Ocircumflex",
  "Otilde",
  "Odieresis",
  "multiply",
  "Oslash",
  "Ugrave",
  "Uacute",
  "Ucircumflex",
  "Udieresis",
  "Yacute",
  "Thorn",
  "germandbls",
  "agrave",
  "aacute",
  "acircumflex",
  "atilde",
  "adieresis",
  "aring",
  "ae",
  "ccedilla",
  "egrave",
  "eacute",
  "ecircumflex",
  "edieresis",
  "igrave",
  "iacute",
  "icircumflex",
  "idieresis",
  "eth",
  "ntilde",
  "ograve",
  "oacute",
  "ocircumflex",
  "otilde",
  "odieresis",
  "divide",
  "oslash",
  "ugrave",
  "uacute",
  "ucircumflex",
  "udieresis",
  "yacute",
  "thorn",
  "ydieresis"
};

int
main (int argc, char *argv[]) {
  int status = 2;			/* Return result, 0=success */

  if (processOptions(optionTable, optionCount, handleOption,
                     &argc, &argv, "input-file output-file")) {
    const CodePage *codePage = NULL;
    if (opt_codePageName) {
      if (!(codePage = getCodePage(opt_codePageName))) {
        fprintf(stderr, "%s: Unknown code page: %s\n",
                programName, opt_codePageName);
        exit(status);
      }
    }

    if (argc > 0) {
      const char *inputPath = *argv++; --argc;
      if (argc > 0) {
        const char *outputPath = *argv++; --argc;
        if (argc == 0) {
          FILE *inputStream = fopen(inputPath, "rb");
          if (inputStream) {
            FILE *outputStream = fopen(outputPath, "w");
            if (outputStream) {
              int byte;			/* Current byte being processed */
              status = 0;
              for (byte=0; byte<0X100; ++byte) {
                int dots = fgetc(inputStream);
                const char *name = NULL;
                int dotIndex;
                unsigned short ubrl = 0X2800;
                if (ferror(inputStream)) {
                  fprintf(stderr, "%s: Cannot read input file '%s': %s",
                          programName, inputPath, strerror(errno));
                  status = 5;
                  break;
                }
                {
                  unsigned char character = byte;
                  unsigned char prefix;
                  if (!(character & 0X60) || (character == 0X7F) || (character == 0XA0)) {
                    if (character & 0X80) {
                      prefix = '~';
                      character &= 0X7F;
                    } else {
                      prefix = '^';
                    }
                    if (character != ' ') character ^= 0X40;
                  } else {
                    prefix = ' ';
                  }
                  fprintf(outputStream, "%c%c", prefix, character);
                }
                fprintf(outputStream, " %02X %3d (", byte, byte);
                for (dotIndex=0; dotIndex<sizeof(dotOrder); ++dotIndex) {
                  unsigned char dot = dotOrder[dotIndex];
                  unsigned char bit = dots & dotTable[dot];
                  fputc((bit? dot+'1': ' '), outputStream);
                  if (bit) ubrl |= 1 << dot;
                }
                fprintf(outputStream, ")%02X B+%04X", dots, ubrl);
                if (codePage) {
                  unsigned short unicode = codePage->table[byte];
                  const UnicodeEntry *uc = getUnicodeEntry(unicode);
                  if (uc) name = uc->name;
                  fprintf(outputStream, " U+%4.4X", unicode);
                }
                if (!name) name = characterNames[byte];
                fprintf(outputStream, " %s", name);
                fprintf(outputStream, "\n");
              }

              fclose(outputStream);
            } else {
              fprintf(stderr, "%s: Cannot open output file '%s': %s\n",
                      programName, outputPath, strerror(errno));
              status = 4;
            }

            fclose(inputStream);
          } else {
            fprintf(stderr, "%s: Cannot open input file '%s': %s\n",
                    programName, inputPath, strerror(errno));
            status = 3;
          }
        } else {
          fprintf(stderr, "%s: Too many parameters.\n", programName);
        }
      } else {
        fprintf(stderr, "%s: Missing output file name.\n", programName);
      }
    } else {
      fprintf(stderr, "%s: Missing input file name.\n", programName);
    }
  }

  return status;
}
