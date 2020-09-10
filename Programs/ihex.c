/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2020 by The BRLTTY Developers.
 *
 * BRLTTY comes with ABSOLUTELY NO WARRANTY.
 *
 * This is free software, placed under the terms of the
 * GNU Lesser General Public License, as published by the Free Software
 * Foundation; either version 2.1 of the License, or (at your option) any
 * later version. Please see the file LICENSE-LGPL for details.
 *
 * Web Page: http://brltty.app/
 *
 * This software is maintained by Dave Mielke <dave@mielke.cc>.
 */

#include "prologue.h"

#include <stdio.h>
#include <string.h>

#include "log.h"
#include "strfmt.h"
#include "ihex.h"
#include "file.h"
#include "datafile.h"

#define IHEX_RECORD_PREFIX  ':'
#define IHEX_COMMENT_PREFIX '#'

#define IHEX_BYTE_WIDTH 8
#define IHEX_BYTE_MASK ((1 << IHEX_BYTE_WIDTH) - 1)

static size_t
ihexByteCount (size_t count) {
  return 1 // the number of data bytes
       + 2 // the starting address
       + 1 // the record type
       + count // the data
       + 1 // the checksum
       ;
}

size_t
ihexRecordLength (size_t count) {
  return 1 // the colon prefix
       + (ihexByteCount(count) * 2) // hexadecimal digit pairs
       ;
}

int
ihexMakeRecord (char *buffer, size_t size, uint8_t type, uint16_t address, const unsigned char *data, uint8_t count) {
  unsigned char bytes[ihexByteCount(count)];
  unsigned char *end = bytes;

  *end++ = count;
  *end++ = (address >> IHEX_BYTE_WIDTH) & IHEX_BYTE_MASK;
  *end++ = address & IHEX_BYTE_MASK;
  *end++ = type;
  if (count > 0) end = mempcpy(end, data, count);

  {
    uint32_t checksum = 0;

    {
      const unsigned char *byte = bytes;
      while (byte < end) checksum += *byte++;
    }

    checksum ^= IHEX_BYTE_MASK;
    checksum += 1;
    *end++ = checksum & IHEX_BYTE_MASK;
  }

  if ((1 + (end - bytes) + 1) > size) return 0;
  STR_BEGIN(buffer, size);
  STR_PRINTF("%c", IHEX_RECORD_PREFIX);

  {
    const unsigned char *byte = bytes;
    while (byte < end) STR_PRINTF("%02X", *byte++);
  }

  STR_END;
  return 1;
}

int
ihexMakeDataRecord (char *buffer, size_t size, uint16_t address, const unsigned char *data, uint8_t count) {
  return ihexMakeRecord(buffer, size, IHEX_TYPE_DATA, address, data, count);
}

int
ihexMakeEndRecord (char *buffer, size_t size) {
  return ihexMakeRecord(buffer, size, IHEX_TYPE_END, 0, NULL, 0);
}

typedef struct {
  struct {
    const char *path;
    unsigned int line;
  } file;

  struct {
    char *text;
  } record;

  struct {
    IhexRecordHandler *function;
    void *data;
  } handler;

  unsigned char problemReported:1;
} IhexData;

static void
ihexReportProblem (IhexData *ihex, const char *message) {
  ihex->problemReported = 1;

  logMessage(LOG_ERR,
    "ihex error: %s: %s[%u]: %s",
    message, ihex->file.path, ihex->file.line, ihex->record.text
  );
}

static int
ihexCheckDigit (IhexData *ihex, unsigned char *value, char digit) {
  typedef struct {
    char first;
    char last;
    char offset;
  } Range;

  static const Range ranges[] = {
    { .first='0', .last='9', .offset= 0 },
    { .first='A', .last='F', .offset=10 },
    { .first='a', .last='f', .offset=10 },
  };

  const Range *range = ranges;
  const Range *end = range + ARRAY_COUNT(ranges);

  while (range < end) {
    if ((digit >= range->first) && (digit <= range->last)) {
      *value = (digit - range->first) + range->offset;
      return 1;
    }

    range += 1;
  }

  ihexReportProblem(ihex, "invalid hexadecimal digit");
  return 0;
}

static int
ihexProcessRecord (IhexData *ihex) {
  const char *character = ihex->record.text;

  if (!*character || (*character != IHEX_RECORD_PREFIX)) {
    ihexReportProblem(ihex, "not a record");
    return 0;
  }

  size_t length = strlen(++character);
  unsigned char bytes[length + 1];
  unsigned char *end = bytes;
  int first = 1;

  while (*character) {
    unsigned char value;
    if (!ihexCheckDigit(ihex, &value, *character)) return 0;

    if (first) {
      *end = value << 4;
    } else {
      *end++ |= value;
    }

    first = !first;
    character += 1;
  }

  if (!first) {
    ihexReportProblem(ihex, "missing hexadecimal digit");
    return 0;
  }

  {
    uint32_t checksum = 0;
    const unsigned char *byte = bytes;
    while (byte < end) checksum += *byte++;
    checksum &= IHEX_BYTE_MASK;

    if (checksum) {
      ihexReportProblem(ihex, "checksum mismatch");
      return 0;
    }
  }

  const unsigned char *byte = bytes;
  size_t actual = end - byte;

  {
    static const char *const messages[] = {
      [0] = "missing data byte count",
      [1] = "missing address",
      [2] = "incomplete address",
      [3] = "missing record type",
    };

    if (actual < ARRAY_COUNT(messages)) {
      const char *message = messages[actual];
      if (!message) message = "unknown error";

      ihexReportProblem(ihex, message);
      return 0;
    }
  }

  uint8_t count = *byte++;
  uint16_t address = *byte++ << IHEX_BYTE_WIDTH;
  address |= *byte++;
  uint8_t type = *byte++;
  size_t expected = ihexByteCount(count);

  if (actual < expected) {
    ihexReportProblem(ihex, "truncated data");
    return 0;
  }

  if (actual > expected) {
    ihexReportProblem(ihex, "excessive data");
    return 0;
  }

  switch (type) {
    case IHEX_TYPE_DATA:
      if (!count) return 0;
      break;

    case IHEX_TYPE_END:
      return 0;

    default:
      ihexReportProblem(ihex, "unsupported record type");
      return 0;
  }

  const IhexRecordData record = {
    .address = address,
    .data = byte,
    .count = count
  };

  if (!ihex->handler.function(&record, ihex->handler.data)) {
    ihexReportProblem(ihex, "record handler failed");
    return 0;
  }

  return 1;
}

static int
ihexProcessLine (char *line, void *data) {
  IhexData *ihex = data;
  ihex->file.line += 1;

  while (*line == ' ') line += 1;
  if (!*line) return 1;
  if (*line == IHEX_COMMENT_PREFIX) return 1;

  ihex->record.text = line;
  return ihexProcessRecord(ihex);
}

int
ihexProcessFile (const char *path, IhexRecordHandler *handler, void *data) {
  IhexData ihex = {
    .file = {
      .path = path,
      .line = 0
    },

    .handler = {
      .function = handler,
      .data = data
    }
  };

  int ok = 0;
  FILE *file = openDataFile(path, "r", 0);

  if (file) {
    if (processLines(file, ihexProcessLine, &ihex)) {
      if (!ihex.problemReported) {
        ok = 1;
      }
    }
    ;
    fclose(file);
  }

  return ok;
}

char *
ihexEnsureExtension (const char *path) {
  return ensureFileExtension(path, IHEX_FILE_EXTENSION);
}

char *
ihexMakePath (const char *directory, const char *name) {
  char *subdirectory = makePath(directory, IHEX_FILES_SUBDIRECTORY);

  if (subdirectory) {
    char *file = makeFilePath(subdirectory, name, IHEX_FILE_EXTENSION);

    free(subdirectory);
    if (file) return file;
  }

  return NULL;
}
