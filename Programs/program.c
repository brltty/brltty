/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2007 by The BRLTTY Developers.
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

#include "prologue.h"

#include <limits.h>

#ifdef ENABLE_I18N_SUPPORT
#include <locale.h>
#endif /* ENABLE_I18N_SUPPORT */

#include "program.h"
#include "misc.h"
#include "system.h"

#ifdef WINDOWS
#include "sys_windows.h"
#endif /* WINDOWS */

const char *programPath;
const char *programName;

static char *
testProgram (const char *directory, const char *name) {
  char *path;

  if ((path = makePath(directory, name))) {
    if (access(path, X_OK) != -1) return path;

    free(path);
  }

  return NULL;
}

static char *
findProgram (const char *name) {
  char *path = NULL;
  const char *string;

  if ((string = getenv("PATH"))) {
    int count;
    char **array;

    if ((array = splitString(string, ':', &count))) {
      int index;

      for (index=0; index<count; ++index) {
        const char *directory = array[index];
        if (!*directory) directory = ".";
        if ((path = testProgram(directory, name))) break;
      }

      deallocateStrings(array);
    }
  }

  return path;
}

void
prepareProgram (int argumentCount, char **argumentVector) {
#ifdef WINDOWS
  sysInit();
#endif /* WINDOWS */

#ifdef ENABLE_I18N_SUPPORT
  setlocale(LC_ALL, "");
  textdomain(PACKAGE_NAME);
#endif /* ENABLE_I18N_SUPPORT */

  if (!(programPath = getProgramPath())) programPath = argumentVector[0];

  if (!isExplicitPath(programPath)) {
    char *path = findProgram(programPath);
    if (!path) path = testProgram(".", programPath);
    if (path) programPath = path;
  }

  if (isExplicitPath(programPath)) {
#if defined(HAVE_REALPATH) && defined(PATH_MAX)
    if (!isAbsolutePath(programPath)) {
      char buffer[PATH_MAX];
      char *path = realpath(programPath, buffer);

      if (path) {
        programPath = strdupWrapper(path);
      } else {
        LogError("realpath");
      }
    }
#endif /* defined(HAVE_REALPATH) && defined(PATH_MAX) */

    if (!isAbsolutePath(programPath)) {
      char *directory;

      if ((directory = getWorkingDirectory())) {
        char *path;
        if ((path = makePath(directory, programPath))) programPath = path;
        free(directory);
      }
    }
  }

  programName = locatePathName(programPath);
  setPrintPrefix(programName);
}

void
makeProgramBanner (char *buffer, size_t size) {
  const char *revision = PACKAGE_REVISION;
  snprintf(buffer, size, "%s %s%s%s [%s]",
           PACKAGE_TITLE, PACKAGE_VERSION,
           (*revision? " rev ": ""), revision,
           BRLTTY_URL);
}

void
fixInstallPaths (char **const *paths) {
  static const char *programDirectory = NULL;

  if (!programDirectory) {
    if (!(programDirectory = getPathDirectory(programPath))) {
      LogPrint(LOG_WARNING, gettext("cannot determine program directory"));
      programDirectory = ".";
    }

    LogPrint(LOG_DEBUG, "program directory: %s", programDirectory);
  }

  while (*paths) {
    char *newPath = makePath(programDirectory, **paths);

    if (!newPath) {
      LogPrint(LOG_WARNING, "%s: %s", gettext("cannot fix install path"), **paths);
    } else if (!isAbsolutePath(**paths=newPath)) {
      LogPrint(LOG_WARNING, "%s: %s", gettext("install path not absolute"), **paths);
    }

    ++paths;
  }
}

void
fixInstallPath (char **path) {
  char **const paths[] = {path, NULL};
  fixInstallPaths(paths);
}
