/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2010 by The BRLTTY Developers.
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
#include <fcntl.h>
#include <sys/stat.h>
#include <limits.h>

#ifdef ENABLE_I18N_SUPPORT
#include <locale.h>
#endif /* ENABLE_I18N_SUPPORT */

#include "program.h"
#include "pid.h"
#include "log.h"
#include "file.h"
#include "parse.h"
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
  snprintf(buffer, size, "%s %s%s%s",
           PACKAGE_TITLE, PACKAGE_VERSION,
           (*revision? " rev ": ""), revision);
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
    if (**paths && ***paths) {
      char *newPath = makePath(programDirectory, **paths);

      if (!newPath) {
        LogPrint(LOG_WARNING, "%s: %s", gettext("cannot fix install path"), **paths);
      } else if (!isAbsolutePath(**paths=newPath)) {
        LogPrint(LOG_WARNING, "%s: %s", gettext("install path not absolute"), **paths);
      }
    }

    paths += 1;
  }
}

void
fixInstallPath (char **path) {
  char **const paths[] = {path, NULL};
  fixInstallPaths(paths);
}

int
createPidFile (const char *path, ProcessIdentifier pid) {
  if (!pid) pid = getProcessIdentifier();

  if (path && *path) {
    typedef enum {PFS_ready, PFS_stale, PFS_clash, PFS_error} PidFileState;
    PidFileState state = PFS_error;
    int file = open(path,
                    O_RDWR | O_CREAT,
                    S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);

    if (file != -1) {
      int locked = acquireFileLock(file, 1);

      if (locked || (errno == ENOSYS)) {
        char buffer[0X20];
        int length;

        if ((length = read(file, buffer, sizeof(buffer))) != -1) {
          ProcessIdentifier oldPid;
          char terminator;
          int count;

          if (length == sizeof(buffer)) length -= 1;
          buffer[length] = 0;
          count = sscanf(buffer, "%" SCNpid "%c", &oldPid, &terminator);
          state = PFS_stale;

          if ((count == 1) ||
              ((count == 2) && ((terminator == '\n') || (terminator == '\r')))) {
            if (oldPid == pid) {
              state = PFS_ready;
            } else if (testProcessIdentifier(oldPid)) {
              LogPrint(LOG_ERR, "instance already running: PID=%" PRIpid, oldPid);
              state = PFS_clash;
            }
          }
        } else {
          LogError("read");
        }

        if (state == PFS_stale) {
          state = PFS_error;

          if (ftruncate(file, 0) != -1) {
            snprintf(buffer, sizeof(buffer), "%" PRIpid "\n%n", pid, &length);

            if (write(file, buffer, length) != -1) {
              state = PFS_ready;
            } else {
              LogError("write");
            }
          } else {
            LogError("ftruncate");
          }
        }

        if (locked) releaseFileLock(file);
      }

      close(file);
    } else {
      LogPrint(LOG_WARNING, "%s: %s: %s",
               gettext("cannot open process identifier file"),
               path, strerror(errno));
    }

    switch (state) {
      case PFS_ready:
        return 1;

      case PFS_clash:
        errno = EEXIST;
        break;

      case PFS_error:
        break;

      default:
        LogPrint(LOG_WARNING, "unexpected PID file state: %u", state);
        break;
    }
  }

  return 0;
}
