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

/* tbl2tbl.c - convert braille tables between dot mappings
 * $Id: tbl2tbl.c,v 1.3 1996/09/24 01:04:25 nn201 Exp $
 */

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

typedef struct {
  const char *name;
  const unsigned char *table;
} MappingEntry;
static unsigned char standard[8] = {0, 2, 4, 1, 3, 5, 6, 7}; /* BRLTTY standard mapping */
static unsigned char Tieman[8]   = {0, 1, 2, 7, 6, 5, 3, 4}; /* Tieman standard */
static unsigned char Alva_TSI[8] = {0, 1, 2, 3, 4, 5, 6, 7}; /* Alva/TSI standard */
static MappingEntry mappingTable[] = {
  {"standard", standard},
  {"tieman"  , Tieman  },
  {"alva"    , Alva_TSI},
  {"tsi"     , Alva_TSI},
  {NULL      , NULL    }
};

static const unsigned char *
mappingArgument (const char *argument) {
  size_t length = strlen(argument);
  const MappingEntry *mapping = mappingTable;
  while (mappingTable->name) {
    if (strlen(mapping->name) >= length) {
      if (strncasecmp(mapping->name, argument, length) == 0) {
        return mapping->table;
      }
    }
    ++mapping;
  }
  fprintf(stderr, "tbl2tbl: Unknown dot mapping: %s\n", argument);
  exit(2);
}

int
main (int argc, char *argv[]) {
  int status = 2;
  ++argv, --argc;
  if (argc > 0) {
    const unsigned char *inputMapping = mappingArgument(*argv++);
    --argc;
    if (argc > 0) {
      const unsigned char *outputMapping = mappingArgument(*argv++);
      --argc;
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
        fprintf(stderr, "tbl2tbl: Too many parameters.\n");
      }
    } else {
      fprintf(stderr, "tbl2tbl: Missing output dot mapping.\n");
    }
  } else {
    fprintf(stderr, "tbl2tbl: Missing input dot mapping.\n");
  }
  return status;
}
