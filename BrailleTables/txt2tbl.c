/*
 * BRLTTY - A background process providing access to the Linux console (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2004 by The BRLTTY Team. All rights reserved.
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

BEGIN_OPTION_TABLE
  {'d', "duplicates", NULL, NULL, 0,
   "List duplicate dot combinations."},
  {'m', "missing", NULL, NULL, 0,
   "List missing dot combinations."},
END_OPTION_TABLE

static int opt_duplicates = 0;
static int opt_missing = 0;

static int
handleOption (const int option) {
  switch (option) {
    default:
      return 0;
    case 'd':
      opt_duplicates = 1;
      break;
    case 'm':
      opt_missing = 1;
      break;
  }
  return 1;
}

static const unsigned char dotTable[] = {B1, B2, B3, B4, B5, B6, B7, B8};

static void
putDots (FILE *stream, unsigned char dots) {
  int dot;
  fputc('(', stream);
  for (dot=0; dot<8; ++dot) {
    fputc(((dots & dotTable[dot])? (dot+'1'): ' '), stream);
  }
  fputc(')', stream);
}

int
main (int argc, char *argv[]) {
  int status = 2;

  if (processOptions(optionTable, optionCount, handleOption,
                     &argc, &argv, "input-file output-file")) {
    if (argc > 0) {
      const char *inputPath = *argv++; --argc;
      if (argc > 0) {
        const char *outputPath = *argv++; --argc;
        if (argc == 0) {
          FILE *inputStream = fopen(inputPath, "r");
          if (inputStream) {
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
              unsigned char *buffer = NULL;
              size_t bufferSize = 0;
              size_t bufferUsed = 0;
              typedef enum {LEFT, MIDDLE, RIGHT} InputState;
              InputState inputState = LEFT;
              int inputCharacter;
              memset(dotsTable, 0, sizeof(dotsTable));
              memset(byteTable, 0, sizeof(byteTable));
              status = 0;
              while ((inputCharacter = fgetc(inputStream)) != EOF) {
                if (inputCharacter == '\n') {
                  if (inputState == MIDDLE) {
                    fprintf(stderr, "%s: Incomplete dot combination for byte %02X.\n",
                            programName, byte);
                    status = 10;
                    goto done;
                  }
                  inputState = LEFT;
                  continue;
                }
                switch (inputState) {
                  case LEFT:
                    if (inputCharacter == '(') {
                      inputState = MIDDLE;
                    }
                    break;
                  case MIDDLE:
                    if (inputCharacter == ')') {
                      unsigned char dots = 0;
                      off_t bufferIndex;
                      for (bufferIndex=0; bufferIndex<bufferUsed; ++bufferIndex) {
                        inputCharacter = buffer[bufferIndex];
                        if (inputCharacter != ' ') {
                          unsigned char dot;
                          if ((inputCharacter < '1') || (inputCharacter > '8')) {
                            fprintf(stderr, "%s: Invalid dot number specified for byte %02X: %c\n",
                                    programName, byte, inputCharacter);
                            status = 10;
                            goto done;
                          }
                          dot = dotTable[inputCharacter - '1'];
                          if (dots & dot) {
                            fprintf(stderr, "%s: Dot %c specified more than once for byte %02X.\n",
                                    programName, inputCharacter, byte);
                            status = 10;
                            goto done;
                          }
                          dots |= dot;
                        }
                      }
                      if (byte == byteCount) {
                        fprintf(stderr, "%s: Too many dot combinations.\n",
                                programName);
                        status = 10;
                        goto done;
                      } else {
                        DotsEntry *d = &dotsTable[dots];
                        ByteEntry *b = &byteTable[byte];
                        b->dots = dots;
                        if (!d->defined) {
                          d->defined = 1;
                          d->byte = byte;
                        } else if (opt_duplicates) {
                          fprintf(stderr, "Dot combination ");
                          putDots(stderr, dots);
                          fprintf(stderr, " represents both bytes %02X and %02X.\n",
                                  d->byte, byte);
                        }
                      }
                      byte++;
                      inputState = RIGHT;
                      bufferUsed = 0;
                    } else if (inputCharacter == '(') {
                      bufferUsed = 0;
                    } else {
                      if (bufferUsed == bufferSize) {
                        size_t newSize = (bufferSize << 4) | 0XF;
                        unsigned char *newBuffer = malloc(newSize);
                        if (!newBuffer) {
                          fprintf(stderr, "%s: Insufficient memory: %s\n",
                                  programName, strerror(errno));
                          status = 10;
                          goto done;
                        }
                        memcpy(newBuffer, buffer, bufferUsed);
                        if (buffer) free(buffer);
                        buffer = newBuffer;
                        bufferSize = newSize;
                      }
                      buffer[bufferUsed++] = inputCharacter;
                    }
                  case RIGHT:
                    break;
                }
              }
            done:
              if (buffer) free(buffer);
              if (!status) {
                if (!ferror(inputStream)) {
                  if (byte == byteCount) {
                    if (opt_missing) {
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
                        fprintf(stderr, "%s: Write error on file '%s': %s\n",
                                programName, outputPath, strerror(errno));
                        status = 6;
                        break;
                      }
                    } while (++byte < byteCount);
                  } else {
                    fprintf(stderr, "%s: Too few dot combinations.\n",
                            programName);
                    status = 10;
                  }
                } else {
                  fprintf(stderr, "%s: Read error on file '%s': %s\n",
                          programName, inputPath, strerror(errno));
                  status = 5;
                }
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
