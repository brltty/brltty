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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "options.h"
#include "misc.h"
#include "help.h"

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

int
main (int argc, char *argv[]) {
  int status = 2;

  processOptions(optionTable, optionCount, handleOption,
                 &argc, &argv, "output-file input-file ...");

  if (argc > 0) {
    const char *outputPath = *argv++; argc--;
    if (argc > 0) {
      HelpFileHeader fileHeader;
      HelpPageEntry *pageTable;

      memset(&fileHeader, 0, sizeof(fileHeader));
      fileHeader.pages = argc;

      if ((pageTable = (HelpPageEntry *)malloc(fileHeader.pages * sizeof(HelpPageEntry)))) {
        FILE *outputStream = fopen(outputPath, "w+");
        if (outputStream) {
          unsigned char pageNumber;
          setvbuf(outputStream, NULL, _IOFBF, 0X7FF0);

          /* Leave space for header: */
          fseek(outputStream, sizeof(fileHeader) + (fileHeader.pages * sizeof(*pageTable)), SEEK_SET);

          for (pageNumber=0; pageNumber<fileHeader.pages; pageNumber++) {
            HelpPageEntry *page = &pageTable[pageNumber];
            const char *inputPath = argv[pageNumber];
            FILE *inputStream = fopen(inputPath, "r");
            if (!inputStream) {
              fprintf(stderr, "%s: Cannot open input file %s: %s\n",
                      programName, inputPath, strerror(errno));
              status = pageNumber + 11;
              break;
            }
            setvbuf(inputStream, NULL, _IOFBF, 0X7FF0);

            memset(page, 0, sizeof(*page));
            while (1) {
              char buffer[242];
              int length = sizeof(buffer);
              if (fgets(buffer, length, inputStream)) {
                const char *end = memchr(buffer, '\n', length);
                if (end) length = end - buffer;
                fputc(length, outputStream);
                fwrite(buffer, 1, length, outputStream);
                page->columns = MAX(page->columns, length);
                page->rows++;
              } else {
                break;
              }
            }
            page->columns = (((int) ((page->columns - 1) / 40)) + 1) * 40;
            fclose(inputStream);
          }

          fseek(outputStream, 0, SEEK_SET);
          fwrite(&fileHeader, sizeof(fileHeader), 1, outputStream);
          fwrite(pageTable, sizeof(*pageTable), fileHeader.pages, outputStream);

          fclose(outputStream);
        } else {
          fprintf(stderr, "%s: Cannot open output file %s: %s\n",
                  programName, outputPath, strerror(errno));
          status = 10;
        }
        free(pageTable);
      } else {
        fprintf (stderr, "%s: Out of memory.\n", programName);
        status = 3;
      }
    } else {
      fprintf(stderr, "%s: Missing input file.\n", programName);
    }
  } else {
    fprintf(stderr, "%s: Missing output file.\n", programName);
  }

  return 0;
}
