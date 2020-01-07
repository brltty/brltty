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

#include <string.h>

#include "log.h"
#include "options.h"
#include "datafile.h"
#include "utf8.h"
#include "brlapi.h"

static char *opt_host;
static char *opt_auth;
static int opt_get;
static char *opt_set;

BEGIN_OPTION_TABLE(programOptions)
  { .letter = 'b',
    .word = "brlapi",
    .argument = "[host][:port]",
    .setting.string = &opt_host,
    .description = "BrlAPIa host and/or port to connect to."
  },

  { .letter = 'a',
    .word = "auth",
    .argument = "scheme+...",
    .setting.string = &opt_auth,
    .description = "BrlAPI authorization/authentication schemes."
  },

  { .letter = 'g',
    .word = "get",
    .setting.flag = &opt_get,
    .description = "Write the content of the clipboard to standard output."
  },

  { .letter = 's',
    .word = "set",
    .argument = strtext("content"),
    .setting.string = &opt_set,
    .description = "Set the content of the clipboard."
  },
END_OPTION_TABLE

static const brlapi_param_t apiParameter = BRLAPI_PARAM_CLIPBOARD_CONTENT;
static const brlapi_param_subparam_t apiSubparam = 0;
static const brlapi_param_flags_t apiFlags = BRLAPI_PARAMF_GLOBAL;

static char *
getClipboardContent (void) {
  return brlapi_getParameterAlloc(apiParameter, apiSubparam, apiFlags, NULL);
}

static int
setClipboardContent (const char *content, size_t length) {
  return brlapi_setParameter(apiParameter, apiSubparam, apiFlags, content, length) >= 0;
}

typedef struct {
  struct {
    wchar_t *characters;
    size_t size;
    size_t count;
  } content;
} LineProcessingData;

static int
addContent (LineProcessingData *lpd, const wchar_t *characters, size_t count) {
  {
    size_t newSize = lpd->content.count + count;

    if (newSize > lpd->content.size) {
      newSize |= 0XFFF;
      newSize += 1;
      wchar_t *newCharacters = allocateCharacters(newSize);

      if (!newCharacters) {
        logMallocError();
        return 0;
      }

      if (lpd->content.characters) {
        wmemcpy(newCharacters, lpd->content.characters, lpd->content.count);
      }

      lpd->content.characters = newCharacters;
      lpd->content.size = newSize;
    }
  }

  wmemcpy(&lpd->content.characters[lpd->content.count], characters, count);
  lpd->content.count += count;
  return 1;
}

static DATA_OPERANDS_PROCESSOR(processInputLine) {
  LineProcessingData *lpd = data;

  DataOperand line;
  getTextRemaining(file, &line);

  if (lpd->content.count > 0) {
    static const wchar_t delimiter[] = {WC_C('\n')};
    if (!addContent(lpd, delimiter, ARRAY_COUNT(delimiter))) return 0;
  }

  return addContent(lpd, line.characters, line.length);
}

int
main (int argc, char *argv[]) {
  ProgramExitStatus exitStatus = PROG_EXIT_FATAL;

  {
    static const OptionsDescriptor descriptor = {
      OPTION_TABLE(programOptions),
      .applicationName = "brltty-clip",
      .argumentsSummary = "[{input-file | -} ...]"
    };

    PROCESS_OPTIONS(descriptor, argc, argv);
  }

  brlapi_connectionSettings_t settings = {
    .host = opt_host,
    .auth = opt_auth
  };

  brlapi_fileDescriptor fileDescriptor = brlapi_openConnection(&settings, &settings);

  if (fileDescriptor != (brlapi_fileDescriptor)(-1)) {
    char *oldContent = NULL;

    LineProcessingData lpd = {
      .content = {
        .characters = NULL,
        .size = 0,
        .count = 0
      }
    };

    int getContent = !!opt_get;
    int setContent = !!*opt_set;

    if (!(getContent || setContent)) {
      const InputFilesProcessingParameters parameters = {
        .dataFileParameters = {
          .options = DFO_NO_COMMENTS,
          .processOperands = processInputLine,
          .data = &lpd
        }
      };

      exitStatus = processInputFiles(argv, argc, &parameters);
    } else if (argc > 0) {
      logMessage(LOG_ERR, "too many arguments");
      exitStatus = PROG_EXIT_SYNTAX;
    } else {
      exitStatus = PROG_EXIT_SUCCESS;
    }

    if (exitStatus == PROG_EXIT_SUCCESS) {
      if (getContent) {
        oldContent = getClipboardContent();
        if (!oldContent) exitStatus = PROG_EXIT_FATAL;
      }
    }

    if (exitStatus == PROG_EXIT_SUCCESS) {
      if (setContent) {
        if (!setClipboardContent(opt_set, strlen(opt_set))) {
          exitStatus = PROG_EXIT_FATAL;
        }
      }
    }

    if (lpd.content.characters) {
      if (exitStatus == PROG_EXIT_SUCCESS) {
        exitStatus = PROG_EXIT_FATAL;

        size_t length;
        char *content = getUtf8FromWchars(lpd.content.characters, lpd.content.count, &length);

        if (content) {
          if (setClipboardContent(content, length)) {
            exitStatus = PROG_EXIT_SUCCESS;
          }

          free(content);
        }
      }

      free(lpd.content.characters);
    }

    if (oldContent) {
      if (exitStatus == PROG_EXIT_SUCCESS) {
        printf("%s", oldContent);
        if (ferror(stdout)) exitStatus = PROG_EXIT_FATAL;
      }

      free(oldContent);
    }

    brlapi_closeConnection();
  } else {
    logMessage(LOG_ERR, "failed to connect to %s using auth %s: %s",
               settings.host, settings.auth, brlapi_strerror(&brlapi_error));
  }

  return exitStatus;
}
