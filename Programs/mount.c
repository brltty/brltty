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

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <mntent.h>
#include <sys/mount.h>

#include "misc.h"
#include "mount.h"

static FILE *
openMountsTable (const char *path, int update) {
  FILE *table = setmntent(path, (update? "a": "r"));
  if (!table)
    LogPrint((errno == ENOENT)? LOG_WARNING: LOG_ERR,
             "mounted file systems table open erorr: %s: %s",
             path, strerror(errno));
  return table;
}

char *
getMountPoint (int (*test) (const char *path, const char *type)) {
  char *path = NULL;
  FILE *table;

  if ((table = openMountsTable(MOUNTED, 0))) {
    struct mntent *entry;

    while ((entry = getmntent(table))) {
      if (test(entry->mnt_dir, entry->mnt_type)) {
        path = strdupWrapper(entry->mnt_dir);
        break;
      }
    }

    endmntent(table);
  }

  return path;
}

static int
appendMountEntry (FILE *table, struct mntent *entry) {
  if (!addmntent(table, entry)) return 1;
  LogPrint(LOG_ERR, "mounts table entry add error: %s[%s] -> %s: %s",
           entry->mnt_type, entry->mnt_fsname, entry->mnt_dir, strerror(errno));
  return 0;
}

static int
addMountEntry (struct mntent *entry) {
  int added = 0;
  FILE *table;

  if ((table = openMountsTable(MOUNTED, 1))) {
    if (appendMountEntry(table, entry)) added = 1;
    endmntent(table);
  }

  return added;
}

static int
removeMountEntry (const char *path) {
  const char *oldFile = MOUNTED;
  const char *newFile = MOUNTED "." PACKAGE_NAME;
  int found = 0;
  int error = 0;

  {
    FILE *oldTable = openMountsTable(oldFile, 0);

    if (oldTable) {
      FILE *newTable = openMountsTable(newFile, 1);

      if (newTable) {
        struct mntent *entry;

        while ((entry = getmntent(oldTable))) {
          if (strcmp(entry->mnt_dir, path) == 0) {
            found = 1;
          } else if (!appendMountEntry(newTable, entry)) {
            error = 1;
            break;
          }
        }

        endmntent(newTable);
      }

      endmntent(oldTable);
    }
  }

  if (!error) {
    if (found) {
      if (rename(newFile, oldFile) == -1) {
        LogPrint(LOG_ERR, "file rename error: %s -> %s: %s",
                 newFile, oldFile, strerror(errno));
        error = 1;
      }
    }
  }

  if (!error) return 1;
  unlink(newFile);
  return 0;
}

int
mountFileSystem (const char *path, const char *reference, const char *type) {
  if (mount(reference, path, type, 0, NULL) != -1) {
    LogPrint(LOG_NOTICE, "file system mounted: %s[%s] -> %s",
             type, reference, path);

    {
      struct mntent entry;
      memset(&entry, 0, sizeof(entry));
      entry.mnt_dir = (char *)path;
      entry.mnt_fsname = (char *)reference;
      entry.mnt_type = (char *)type;
      entry.mnt_opts = "rw";
      addMountEntry(&entry);
    }

    return 1;
  } else {
    LogPrint(LOG_ERR, "file system mount error: %s[%s] -> %s: %s",
             type, reference, path, strerror(errno));
  }

  return 0;
}

int
unmountFileSystem (const char *path) {
  if (umount(path) != -1) {
    LogPrint(LOG_NOTICE, "file system unmounted: %s", path);
    removeMountEntry(path);
    return 1;
  } else {
    LogPrint(LOG_WARNING, "file system unmount error: %s: %s", path, strerror(errno));
  }

  return 0;
}
