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

/* tbl2tbl.c - convert braille tables between dot mappings
 * $Id: tbl2tbl.c,v 1.3 1996/09/24 01:04:25 nn201 Exp $
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

#include "Programs/options.h"

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

typedef struct {
  const char *name;
  const unsigned char *table;
} MappingEntry;
static unsigned char standard[8] = {0, 2, 4, 1, 3, 5, 6, 7}; /* BRLTTY standard mapping */
static unsigned char Tieman[8]   = {0, 1, 2, 7, 6, 5, 3, 4}; /* Tieman standard */
static unsigned char MDV[8]      = {3, 2, 1, 7, 6, 5, 0, 4}; /* MDV standard */
static unsigned char Eco[8]      = {4, 5, 6, 0, 1, 2, 7, 3}; /* EcoBraille standard */
static unsigned char Alva_TSI[8] = {0, 1, 2, 3, 4, 5, 6, 7}; /* Alva/TSI standard */
static MappingEntry mappingTable[] = {
  {"standard", standard},
  {"eco"     , Eco     },
  {"mdv"     , MDV     },
  {"tieman"  , Tieman  },
  {"alva"    , Alva_TSI},
  {"tsi"     , Alva_TSI},
  {NULL      , NULL    }
};

static const unsigned char *
mappingArgument (const char *argument) {
  size_t length = strlen(argument);
  const MappingEntry *mapping = mappingTable;
  while (mapping->name) {
    if (strlen(mapping->name) >= length) {
      if (strncasecmp(mapping->name, argument, length) == 0) {
        return mapping->table;
      }
    }
    ++mapping;
  }
  fprintf(stderr, "%s: Unknown dot mapping: %s\n", programName, argument);
  exit(2);
}

int
main (int argc, char *argv[]) {
  int status = 2;

  if (processOptions(optionTable, optionCount, handleOption,
                     &argc, &argv, "input-mapping output-mapping")) {
    if (argc > 0) {
      const unsigned char *inputMapping = mappingArgument(*argv++); --argc;
      if (argc > 0) {
        const unsigned char *outputMapping = mappingArgument(*argv++); --argc;
        if (argc == 0) {
          status = 0;
          while (1) {
            int inputCharacter = getchar();
            unsigned char outputCharacter = 0;
            int i;
            if (inputCharacter == EOF) break;
            for (i=0; i<8; i++)
              if (inputCharacter & (1 << inputMapping[i]))
                outputCharacter |= 1 << outputMapping[i];
            putchar(outputCharacter);
          }
        } else {
          fprintf(stderr, "%s: Too many parameters.\n", programName);
        }
      } else {
        fprintf(stderr, "%s: Missing output dot mapping.\n", programName);
      }
    } else {
      fprintf(stderr, "%s: Missing input dot mapping.\n", programName);
    }
  }

  return status;
}
