/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2018 by The BRLTTY Developers.
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

#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <expat.h>

#include "log.h"
#include "cldr.h"
#include "file.h"

const char cldrDefaultDirectory[] = "/usr/share/unicode/cldr/common/annotations";
const char cldrDefaultExtension[] = ".xml";

typedef struct {
  struct {
    CLDR_AnnotationHandler *handler;
    void *data;
  } caller;

  struct {
    XML_Parser parser;
    unsigned int depth;
  } document;

  struct {
    char *sequence;
    char *name;
    unsigned int depth;
  } annotation;
} DocumentParseData;

static void
abortParser (DocumentParseData *dpd) {
  XML_StopParser(dpd->document.parser, 0);
}

static void
appendAnnotationText (void *userData, const char *characters, int count) {
  DocumentParseData *dpd = userData;

  if (dpd->document.depth == dpd->annotation.depth) {
    if (count > 0) {
      char *name = dpd->annotation.name;

      if (name) {
        size_t oldLength = strlen(name);
        size_t newLength = oldLength + count;
        char *newName = realloc(name, newLength+1);

        if (!newName) {
          logMallocError();
          abortParser(dpd);
          return;
        }

        memcpy(&newName[oldLength], characters, count);
        newName[newLength] = 0;
        name = newName;
      } else {
        if (!(name = malloc(count+1))) {
          logMallocError();
          abortParser(dpd);
          return;
        }

        memcpy(name, characters, count);
        name[count] = 0;
      }

      dpd->annotation.name = name;
    }
  }
}

static void
handleElementStart (void *userData, const char *element, const char **attributes) {
  DocumentParseData *dpd = userData;
  dpd->document.depth += 1;

  if (dpd->annotation.depth) {
    logMessage(LOG_WARNING, "nested annotation");
    abortParser(dpd);
    return;
  }

  if (strcmp(element, "annotation") == 0) {
    const char *sequence = NULL;
    int tts = 0;

    while (*attributes) {
      const char *name = *attributes++;
      const char *value = *attributes++;

      if (strcmp(name, "type") == 0) {
        if (strcmp(value, "tts") == 0) tts = 1;
      } else if (strcmp(name, "cp") == 0) {
        sequence = value;
      }
    }

    if (tts) {
      if (sequence) {
        if ((dpd->annotation.sequence = strdup(sequence))) {
          dpd->annotation.depth = dpd->document.depth;
        } else {
          logMallocError();
          abortParser(dpd);
        }
      }
    }
  }
}

static void
handleElementEnd (void *userData, const char *name) {
  DocumentParseData *dpd = userData;

  if (dpd->document.depth == dpd->annotation.depth) {
    if (dpd->annotation.name) {
      CLDR_AnnotationHandlerParameters parameters = {
        .sequence = dpd->annotation.sequence,
        .name = dpd->annotation.name,
        .data = dpd->caller.data
      };

      if (!dpd->caller.handler(&parameters)) {
        abortParser(dpd);
      }
      free(dpd->annotation.sequence);
      dpd->annotation.sequence = NULL;
      free(dpd->annotation.name);
      dpd->annotation.name = NULL;
    }

    dpd->annotation.depth = 0;
  }

  dpd->document.depth -= 1;
}

int
cldrParseDocument (
  const char *document, size_t size,
  CLDR_AnnotationHandler *handler, void *data
) {
  int ok = 0;
  XML_Parser parser = XML_ParserCreate(NULL);

  if (parser) {
    DocumentParseData dpd = {
      .caller = {
        .handler = handler,
        .data = data
      },

      .document = {
        .parser = parser,
        .depth = 0
      },

      .annotation = {
        .sequence = NULL,
        .name = NULL,
        .depth = 0
      }
    };

    XML_SetUserData(parser, &dpd);
    XML_SetElementHandler(parser, handleElementStart, handleElementEnd);
    XML_SetCharacterDataHandler(parser, appendAnnotationText);
    enum XML_Status status = XML_Parse(parser, document, size, 0);

    switch (status) {
      case XML_STATUS_OK:
        ok = 1;
        break;

      case XML_STATUS_ERROR:
        logMessage(LOG_WARNING, "CLDR parse error: %s", XML_ErrorString(XML_GetErrorCode(parser)));
        break;

      default:
        logMessage(LOG_WARNING, "unrecognized CLDR parse status: %d", status);
        break;
    }

    if (dpd.annotation.sequence) {
      free(dpd.annotation.sequence);
      dpd.annotation.sequence = NULL;
    }

    if (dpd.annotation.name) {
      free(dpd.annotation.name);
      dpd.annotation.name = NULL;
    }

    XML_ParserFree(parser);
    parser = NULL;
  } else {
    logMallocError();
  }

  return ok;
}

int
cldrParseFile (
  const char *name,
  CLDR_AnnotationHandler *handler, void *data
) {
  int ok = 0;
  char *path = makeFilePath(cldrDefaultDirectory, name, cldrDefaultExtension);

  if (path) {
    int fd = open(path, O_RDONLY);

    if (fd != -1) {
      struct stat status;

      if (fstat(fd, &status) != -1) {
        size_t size = status.st_size;
        char buffer[size];
        ssize_t count = read(fd, buffer, size);

        if (count != -1) {
          if (cldrParseDocument(buffer, count, handler, data)) ok = 1;
        } else {
          logMessage(LOG_WARNING, "CLDR read error: %s: %s", strerror(errno), path);
        }
      } else {
        logMessage(LOG_WARNING, "CLDR fstat error: %s: %s", strerror(errno), path);
      }

      close(fd);
      fd = -1;
    } else {
      logMessage(LOG_WARNING, "CLDR open error: %s: %s", strerror(errno), path);
    }

    free(path);
  }

  return ok;
}
