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
addMountEntry (struct mntent *entry) {
  int added = 0;
  FILE *table;

  if ((table = openMountsTable(MOUNTED, 1))) {
    if (!addmntent(table, entry)) {
      added = 1;
    } else {
      LogPrint(LOG_WARNING,
               "file systems mount table update error: %s",
               strerror(errno));
    }

    endmntent(table);
  }

  return added;
}

static void
removeMountEntry (const char *path) {
  FILE *oldTable;

  if ((oldTable = openMountsTable(MOUNTED, 0))) {
    struct mntent *entry;

    while ((entry = getmntent(oldTable))) {
      if (strcmp(entry->mnt_dir, path) != 0) {
      }
    }

    endmntent(oldTable);
  }
}

int
mountFileSystem (const char *path, const char *target, const char *type) {
  if (mount(path, target, type, 0, NULL) != -1) {
    {
      struct mntent entry;
      memset(&entry, 0, sizeof(entry));
      entry.mnt_dir = (char *)path;
      entry.mnt_fsname = (char *)target;
      entry.mnt_type = (char *)type;
      entry.mnt_opts = "rw";
      addMountEntry(&entry);
    }

    return 1;
  } else {
    LogPrint(LOG_ERR, "mount error: %s[%s] -> %s: %s",
             type, target, path, strerror(errno));
  }

  return 0;
}

int
unmountFileSystem (const char *path) {
  if (umount(path) != -1) {
    removeMountEntry(path);
    return 1;
  } else {
    LogPrint(LOG_WARNING, "unmount error: %s: %s", path, strerror(errno));
  }

  return 0;
}
