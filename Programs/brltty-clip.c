/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2019 by The BRLTTY Developers.
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

#include "log.h"
#include "options.h"
#include "datafile.h"
#include "utf8.h"
#include "brlapi.h"

static char *opt_host;
static char *opt_auth;

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
END_OPTION_TABLE

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
    brlapi_param_t parameter = BRLAPI_PARAM_CLIPBOARD_CONTENT;
    brlapi_param_subparam_t subparam = 0;
    brlapi_param_flags_t flags = BRLAPI_PARAMF_GLOBAL;

    if (argc > 0) {
      LineProcessingData lpd = {
        .content = {
          .characters = NULL,
          .size = 0,
          .count = 0
        }
      };

      const InputFilesProcessingParameters parameters = {
        .dataFileParameters = {
          .options = DFO_NO_COMMENTS,
          .processOperands = processInputLine,
          .data = &lpd
        }
      };

      if ((exitStatus = processInputFiles(argv, argc, &parameters)) == PROG_EXIT_SUCCESS) {
        exitStatus = PROG_EXIT_FATAL;

        size_t length;
        char *content = getUtf8FromWchars(lpd.content.characters, lpd.content.count, &length);

        if (content) {
          if (brlapi_setParameter(parameter, subparam, flags, content, length)) {
            exitStatus = PROG_EXIT_SUCCESS;
          }

          free(content);
        }
      }

      if (lpd.content.characters) free(lpd.content.characters);
    } else {
      char *content = brlapi_getParameterAlloc(parameter, subparam, flags, NULL);

      if (content) {
        printf("%s", content);
        free(content);
      }

      exitStatus = PROG_EXIT_SUCCESS;
    }

    brlapi_closeConnection();
  } else {
    logMessage(LOG_ERR, "failed to connect to %s using auth %s: %s",
               settings.host, settings.auth, brlapi_strerror(&brlapi_error));
  }

  return exitStatus;
}
