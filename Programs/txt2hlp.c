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
#include <errno.h>

#include "options.h"
#include "misc.h"
#include "help.h"

BEGIN_OPTION_TABLE(programOptions)
END_OPTION_TABLE

int
main (int argc, char *argv[]) {
  int status = 2;

  {
    static const OptionsDescriptor descriptor = {
      OPTION_TABLE(programOptions),
      .applicationName = "txt2hlp",
      .argumentsSummary = "output-file input-file ..."
    };
    processOptions(&descriptor, &argc, &argv);
  }

  if (argc > 0) {
    const char *outputPath = *argv++; argc--;
    if (argc > 0) {
      HelpFileHeader fileHeader;
      HelpPageEntry *pageTable;

      memset(&fileHeader, 0, sizeof(fileHeader));
      fileHeader.pages = argc;

      if ((pageTable = malloc(fileHeader.pages * sizeof(HelpPageEntry)))) {
        FILE *outputStream = fopen(outputPath, "w+b");
        if (outputStream) {
          unsigned char pageNumber;
          setvbuf(outputStream, NULL, _IOFBF, 0X7FF0);

          /* Leave space for header: */
          fseek(outputStream, sizeof(fileHeader) + (fileHeader.pages * sizeof(*pageTable)), SEEK_SET);

          for (pageNumber=0; pageNumber<fileHeader.pages; pageNumber++) {
            int rows = 0;
            int columns = 0;
            const char *inputPath = argv[pageNumber];
            FILE *inputStream = fopen(inputPath, "r");
            if (!inputStream) {
              LogPrint(LOG_ERR, "cannot open input file %s: %s",
                       inputPath, strerror(errno));
              status = pageNumber + 11;
              break;
            }
            setvbuf(inputStream, NULL, _IOFBF, 0X7FF0);

            while (1) {
              char buffer[242];
              int length = sizeof(buffer);
              if (fgets(buffer, length, inputStream)) {
                const char *end = memchr(buffer, '\n', length);
                if (end) length = end - buffer;
                fputc(length, outputStream);
                fwrite(buffer, 1, length, outputStream);
                columns = MAX(columns, length);
                rows++;
              } else {
                break;
              }
            }
            fclose(inputStream);

            {
              HelpPageEntry *page = &pageTable[pageNumber];
              memset(page, 0, sizeof(*page));
              putBigEndian(&page->height, rows);
              putBigEndian(&page->width, columns);
            }
          }

          fseek(outputStream, 0, SEEK_SET);
          fwrite(&fileHeader, sizeof(fileHeader), 1, outputStream);
          fwrite(pageTable, sizeof(*pageTable), fileHeader.pages, outputStream);

          fclose(outputStream);
        } else {
          LogPrint(LOG_ERR, "cannot open output file %s: %s",
                   outputPath, strerror(errno));
          status = 10;
        }
        free(pageTable);
      } else {
        LogPrint(LOG_ERR, "out of memory.");
        status = 3;
      }
    } else {
      LogPrint(LOG_ERR, "missing input file.");
    }
  } else {
    LogPrint(LOG_ERR, "missing output file.");
  }

  return 0;
}
