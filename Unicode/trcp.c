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
 * Maybe This software is maintained by Dave Mielke <dave@mielke.cc>.
 */

/*
 * by Hans Schou
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "unicode.h"
#include "Programs/options.h"

BEGIN_OPTION_TABLE
  {'i', "input-code-page", "name", NULL, 0,
   "Input code page (default iso-8859-1)."},
  {'l', "list-code-pages", NULL, NULL, 0,
   "List all code pages and exit."},
  {'n', "no-output", NULL, NULL, 0,
   "Don't write the translated characters."},
  {'o', "output-code-page", "name", NULL, 0,
   "Output code page (default iso-8859-1)."},
  {'s', "display-summary", NULL, NULL, 0,
   "Display character translation summary."},
  {'u', "unmapped-character", "character", NULL, 0,
   "Character to use when no mapping exists."},
END_OPTION_TABLE

static const char *opt_inputCodePageName = "iso-8859-1";
static int opt_listCodePages = 0;
static int opt_noOutput = 0;
static const char *opt_outputCodePageName = NULL;
static int opt_displaySummary = 0;
static unsigned char opt_unmappedCharacter = '?';

static int
handleOption (const int option) {
  switch (option) {
    default:
      return 0;
    case 'i':
      opt_inputCodePageName = optarg;
      break;
    case 'l':
      opt_listCodePages = 1;
      break;
    case 'n':
      opt_noOutput = 1;
      break;
    case 'o':
      opt_outputCodePageName = optarg;
      break;
    case 's':
      opt_displaySummary = 1;
      break;
    case 'u':
      opt_unmappedCharacter = optarg[0];
      break;
  }
  return 1;
}

int
main (int argc, char *argv[]) {
  int status = 2;

  typedef struct {
    unsigned long count;
    unsigned int unicode;
    unsigned char translation;
    unsigned mapped:1;
  } CharacterEntry;
  CharacterEntry characterTable[0X100];

  int character;
  const CodePage *inputCodePage;
  const CodePage *outputCodePage;

  opt_outputCodePageName = opt_inputCodePageName;
  if (processOptions(optionTable, optionCount, handleOption,
                     &argc, &argv, "")) {
    if (argc == 0) {
      if (!(inputCodePage = getCodePage(opt_inputCodePageName))) {
        fprintf(stderr, "%s: Unknown input code page: %s\n",
                programName, opt_inputCodePageName);
        exit(status);
      }

      if (!(outputCodePage = getCodePage(opt_outputCodePageName))) {
        fprintf(stderr, "%s: Unknown output code page: %s\n",
                programName, opt_outputCodePageName);
        exit(status);
      }

      if (opt_listCodePages) {
        int i;
        for (i=0; i<codePageCount; i++) {
          const CodePage *cp = codePageTable[i];
          printf("%s - %s\n", cp->name, cp->desc);
        }
        exit(0);
      }

      /* build translation table */
      status = 0;
      for (character=0; character<0X100; character++) {
        CharacterEntry *ch = &characterTable[character];
        ch->count = 0;
        ch->unicode = inputCodePage->table[character];
        ch->translation = character;
        ch->mapped = 1;
        if (ch->unicode != outputCodePage->table[ch->translation]) {
          int translation;
          for (translation=0; translation<0X100; translation++) {
            if (ch->unicode == outputCodePage->table[translation]) break;
          }
          if (translation == 0X100) {
            translation = opt_unmappedCharacter;
            ch->mapped = 0;
          }
          ch->translation = translation;
        }
      }

      /* here is what the program really does */
      while ((character = getchar()) != EOF) {
        CharacterEntry *ch = &characterTable[character];
        ch->count++;
        if (!opt_noOutput) {
          putchar(ch->translation);
          if (ferror(stdout)) {
            fprintf(stderr, "%s: Output error: %s\n",
                    programName, strerror(errno));
            exit(4);
          }
        }
      }
      if (ferror(stdin)) {
        fprintf(stderr, "%s: Input error: %s\n",
                programName, strerror(errno));
        exit(3);
      }

      if (opt_displaySummary) {
        fprintf(stderr, "In:  %s - %s\n", inputCodePage->name, inputCodePage->desc);
        fprintf(stderr, "Out: %s - %s\n", outputCodePage->name, outputCodePage->desc);
        fprintf(stderr, "The following characters were processed:\n");
        fprintf(stderr, "In      Count  Out  Unum  Description\n");
        for (character=0; character<0X100; character++) {
          CharacterEntry *ch = &characterTable[character];
          if (ch->count) {
            const UnicodeEntry *uc = getUnicodeEntry(ch->unicode);
            const char *name = uc? uc->name: "?";
            fprintf(stderr, "%02X %10lu  %c%02X  %04X  %s\n",
                    character, ch->count, (ch->mapped? '+': '-'),
                    ch->translation, ch->unicode, name);
          }
        }
      }
    } else {
      fprintf(stderr, "%s: Too many parameters.\n", programName);
    }
  }

  return status;
}
