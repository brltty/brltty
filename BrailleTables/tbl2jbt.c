/*
 * BRLTTY - A background process providing access to the Linux console (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2003 by The BRLTTY Team. All rights reserved.
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

/* tbl2jbt - translate Braille dot table file to the JBT (JAWS for Windows
 * Braille Table) format
 * Warning: JFW expects tables for IBM437 charset, while BRLTTY uses
 * tables for ISO-8859-1. Use isotoibm437 before passing through tbl2jbt.
 * Adapted from James Bowden's tbl2txt
 * Stéphane Doyon
 * September 1999
 * Version 1.0
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "Programs/options.h"
#include "Programs/brl.h"

BEGIN_OPTION_TABLE
END_OPTION_TABLE

static int
handleOption (const int option) {
  switch (option) {
    default:
      return 0;
  }
  return 1;
}

unsigned char dotTable[] = {B1, B2, B3, B4, B5, B6, B7, B8};

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
              int character;
              fputs("[OEM]\r\n", outputStream);
              for (character=0; character<0X100; character++) {
                unsigned char dots = fgetc(inputStream);
                unsigned char dot;
                fprintf(outputStream, "\\%u=", character);
                for (dot=0; dot<8; dot++)
                  if (dots & dotTable[dot])
                    fputc(dot+'1', outputStream);
                fputs("\r\n", outputStream);
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
