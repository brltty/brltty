/*
 * BRLTTY - A background process providing access to the Linux console (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2001 by The BRLTTY Team. All rights reserved.
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

/* txt2tbl.c - Simple Braille table creator
 * James Bowden
 * $Id: txt2tbl.c,v 1.2 1996/09/21 23:34:51 nn201 Exp $
 * March 1996
 * Reworked by Dave Mielke <dave@mielke.cc> (November 2001)
 * Version 1.1
 */

#include <stdlib.h>
#include <stdio.h>
#include <getopt.h>
#include <string.h>
#include <errno.h>

static const unsigned char dotBit[8] = {
  0X01, 0X04, 0X10, 0X02, 0X08, 0X20, 0X40, 0X80
};

static void
putDots (FILE *stream, unsigned char dots) {
  int dot;
  fputc('(', stream);
  for (dot=0; dot<8; ++dot) {
    fputc(((dots & dotBit[dot])? (dot+'1'): ' '), stream);
  }
  fputc(')', stream);
}

int
main (int argc, char *argv[]) {
  int status = 0;

  int duplicates = 0;
  int missing = 0;

  const char *const shortOptions = ":dm";
  const struct option longOptions[] = {
    {"duplicates", no_argument      , NULL, 'd'},
    {"missing"   , no_argument      , NULL, 'm'},
    {NULL        , 0                , NULL,  0 }
  };

  opterr = 0;
  while (1) {
    int option = getopt_long(argc, argv, shortOptions, longOptions, NULL);
    if (option == -1) break;
    switch (option) {
      default:
        fprintf(stderr, "txt2tbl: Unimplemented option: -%c\n", option);
        exit(2);
      case '?':
        fprintf(stderr, "txt2tbl: Invalid option: -%c\n", optopt);
        exit(2);
      case ':':
        fprintf(stderr, "txt2tbl: Missing operand: -%c\n", optopt);
        exit(2);
      case 'd':
        duplicates = 1;
        break;
      case 'm':
        missing = 1;
        break;
    }
  }
  argv += optind; argc -= optind;

  if (argc == 2) {
    const char *inputPath = argv[0];
    FILE *inputStream = fopen(inputPath, "r");
    if (inputStream) {
      const char *outputPath = argv[1];
      FILE *outputStream = fopen(outputPath, "wb");
      if (outputStream) {
        typedef struct {
          unsigned defined:1;
          unsigned char byte;
        } DotsEntry;
        DotsEntry dotsTable[0X100];
        const unsigned int byteCount = 0X100;
        typedef struct {
          unsigned char dots;
        } ByteEntry;
        ByteEntry byteTable[byteCount];
        unsigned int byte = 0;
        typedef enum {LEFT, MIDDLE, RIGHT} InputState;
        InputState inputState = LEFT;
        int inputCharacter;
        memset(dotsTable, 0, sizeof(dotsTable));
        memset(byteTable, 0, sizeof(byteTable));
        while ((inputCharacter = fgetc(inputStream)) != EOF) {
          if (inputCharacter == '\n') {
            if (inputState == MIDDLE) {
              fprintf(stderr, "txt2tbl: Incomplete dot combination for byte %2.2X.\n", byte);
              status = 10;
              goto done;
            }
            inputState = LEFT;
            continue;
          }
          switch (inputState) {
            case LEFT:
              if (inputCharacter == '(') {
                if (byte == byteCount) {
                  fprintf(stderr, "txt2tbl: Too many dot combinations.\n");
                  status = 10;
                  goto done;
                }
                inputState = MIDDLE;
              }
              break;
            case MIDDLE:
              if (inputCharacter != ' ') {
                if ((inputCharacter >= '1') && (inputCharacter <= '8')) {
                  ByteEntry *b = &byteTable[byte];
                  unsigned char dot = dotBit[inputCharacter - '1'];
                  if (b->dots & dot) {
                    fprintf(stderr, "txt2tbl: Dot %c specified more than once for byte %2.2X.\n",
                            inputCharacter, byte);
                    status = 10;
                    goto done;
                  }
                  b->dots |= dot;
                } else if (inputCharacter == ')') {
                  unsigned char dots = byteTable[byte].dots;
                  DotsEntry *d = &dotsTable[dots];
                  if (!d->defined) {
                    d->defined = 1;
                    d->byte = byte;
                  } else if (duplicates) {
                    fprintf(stderr, "Dot combination ");
                    putDots(stderr, dots);
                    fprintf(stderr, " represents both bytes %2.2X and %2.2X.\n",
                            d->byte, byte);
                  }
                  inputState = RIGHT;
                  byte++;
                } else {
                  fprintf(stderr, "txt2tbl: Invalid dot number for byte %2.2X: %c\n",
                          byte, inputCharacter);
                  status = 10;
                  goto done;
                }
              }
            case RIGHT:
              break;
          }
        }
      done:
        if (!status) {
          if (!ferror(inputStream)) {
            if (byte == byteCount) {
              if (missing) {
                unsigned char dots = 0;
                do {
                  DotsEntry *d = &dotsTable[dots];
                  if (!d->defined) {
                    fprintf(stderr, "Dot combination ");
                    putDots(stderr, dots);
                    fprintf(stderr, " has not been assigned.\n");
                  }
                } while (++dots);
              }
              byte = 0;
              do {
                fputc(byteTable[byte].dots, outputStream);
                if (ferror(outputStream)) {
                  fprintf(stderr, "txt2tbl: Write error on file '%s': %s\n",
                          outputPath, strerror(errno));
                  status = 6;
                  break;
                }
              } while (++byte < byteCount);
            } else {
              fprintf(stderr, "txt2tbl: Too few dot combinations.\n");
              status = 10;
            }
          } else {
            fprintf(stderr, "txt2tbl: Read error on file '%s': %s\n",
                    inputPath, strerror(errno));
            status = 5;
          }
        }
        fclose(outputStream);
      } else {
        fprintf(stderr, "txt2tbl: Cannot open output file '%s': %s\n",
                outputPath, strerror(errno));
        status = 4;
      }
      fclose(inputStream);
    } else {
      fprintf(stderr, "txt2tbl: Cannot open input file '%s': %s\n",
              inputPath, strerror(errno));
      status = 3;
    }
  } else {
    fprintf(stderr, "txt2tbl - Compile a braille dot-table.\n");
    fprintf(stderr, "Usage: txt2tbl -option ... input_file output_file\n");
    fprintf(stderr, "-d  --duplicates  List duplicate dot combinations.\n");
    fprintf(stderr, "-m  --missing     List missing dot combinations.\n");
    status = 2;
  }
  return status;
}
