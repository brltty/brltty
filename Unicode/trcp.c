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

#include <stdlib.h>
#include <stdio.h>
#include <getopt.h>
#include <string.h>
#include <errno.h>
#include "../Unicode/unicode.h"

int
main (int argc, char *argv[])
{
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

  int help = 0;
  const char *inputCodePageName = "iso-8859-1";
  int listCodePages = 0;
  int noOutput = 0;
  const char *outputCodePageName = inputCodePageName;
  int displaySummary = 0;
  unsigned char unmappedCharacter = '?';

  const char *const shortOptions = ":hi:lno:su:";
  const struct option longOptions[] = {
    {"help"                  , no_argument      , NULL, 'h'},
    {"input-code-page"       , required_argument, NULL, 'i'},
    {"list-code-pages"       , no_argument      , NULL, 'l'},
    {"no-output"             , no_argument      , NULL, 'n'},
    {"output-code-page"      , required_argument, NULL, 'o'},
    {"display-summary"       , no_argument      , NULL, 's'},
    {"unmapped-character"    , required_argument, NULL, 'u'},
    {NULL                    , 0                , NULL,  0 }
  };

  opterr = 0;
  while (1) {
    int option = getopt_long(argc, argv, shortOptions, longOptions, NULL);
    if (option == -1) break;
    switch (option) {
      default:
        fprintf(stderr, "trcp: Unimplemented option: -%c\n", option);
        exit(2);
      case '?':
        fprintf(stderr, "trcp: Invalid option: -%c\n", optopt);
        exit(2);
      case ':':
        fprintf(stderr, "trcp: Missing operand: -%c\n", optopt);
        exit(2);
      case 'h':
        help = 1;
        break;
      case 'i':
        inputCodePageName = optarg;
        break;
      case 'l':
        listCodePages = 1;
        break;
      case 'n':
        noOutput = 1;
        break;
      case 'o':
        outputCodePageName = optarg;
        break;
      case 's':
        displaySummary = 1;
        break;
      case 'u':
        unmappedCharacter = optarg[0];
        break;
    }
  }
  argv += optind; argc -= optind;

  if (!(inputCodePage = getCodePage(inputCodePageName))) {
    fprintf(stderr, "trcp: Unknown input code page: %s\n", inputCodePageName);
    exit(2);
  }

  if (!(outputCodePage = getCodePage(outputCodePageName))) {
    fprintf(stderr, "trcp: Unknown output code page: %s\n", outputCodePageName);
    exit(2);
  }

  if (help) {
    printf("Translate from one code page to another while\n");
    printf("copying from standard input to standard output.\n");
    printf("Usage: trcp -option ...\n");
    printf("-h       --help                 Display command usage summary (and exit).\n");
    printf("-i page  --input-code-page=     Input code page (default iso-8859-1).\n");
    printf("-l       --list-code-pages      List all code pages (and exit).\n");
    printf("-n       --no-output            Don't write the translated characters.\n");
    printf("-o page  --output-code-page=    Output code page (default iso-8859-1).\n");
    printf("-s       --display-summary      Display character translation summary.\n");
    printf("-u char  --unmapped-character=  Character to use when no mapping exists.\n");
    exit(0);
  }

  if (listCodePages) {
    int i;
    for (i=0; i<codePageCount; i++) {
      const CodePage *cp = codePageTable[i];
      printf("%s - %s\n", cp->name, cp->desc);
    }
    exit(0);
  }

  /* build translation table */
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
        translation = unmappedCharacter;
        ch->mapped = 0;
      }
      ch->translation = translation;
    }
  }

  /* here is what the program really does */
  while ((character = getchar()) != EOF) {
    CharacterEntry *ch = &characterTable[character];
    ch->count++;
    if (!noOutput) {
      putchar(ch->translation);
      if (ferror(stdout)) {
        fprintf(stderr, "trcp: Output error: %s\n", strerror(errno));
        exit(4);
      }
    }
  }
  if (ferror(stdin)) {
    fprintf(stderr, "trcp: Input error: %s\n", strerror(errno));
    exit(3);
  }

  if (displaySummary) {
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

  return 0;
}
