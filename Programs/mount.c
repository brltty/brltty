/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2012 by The BRLTTY Developers.
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

#include "log.h"
#include "system.h"
#include "async.h"
#include "mount.h"

#if defined(HAVE_MNTENT_H)
#include <mntent.h>

#if defined(MOUNTED)
#define MOUNTS_TABLE_PATH MOUNTED
#elif defined(MNT_MNTTAB)
#define MOUNTS_TABLE_PATH MNT_MNTTAB
#endif /* MOUNTS_TABLE_PATH */

typedef struct mntent MountEntry;
#define mountPath mnt_dir
#define mountReference mnt_fsname
#define mountType mnt_type
#define mountOptions mnt_opts

static FILE *
openMountsTable (int update) {
  FILE *table = setmntent(MOUNTS_TABLE_PATH, (update? "a": "r"));
  if (!table)
    logMessage((errno == ENOENT)? LOG_WARNING: LOG_ERR,
               "mounted file systems table open erorr: %s: %s",
               MOUNTS_TABLE_PATH, strerror(errno));
  return table;
}

static void
closeMountsTable (FILE *table) {
  endmntent(table);
}

static MountEntry *
readMountsTable (FILE *table) {
  return getmntent(table);
}

static int
addMountEntry (FILE *table, MountEntry *entry) {
#ifdef HAVE_ADDMNTENT
  if (addmntent(table, entry)) {
    logMessage(LOG_ERR, "mounts table entry add error: %s[%s] -> %s: %s",
               entry->mnt_type, entry->mnt_fsname, entry->mnt_dir, strerror(errno));
    return 0;
  }
#endif /* HAVE_ADDMNTENT */
  return 1;
}

#elif defined(HAVE_SYS_MNTTAB_H)
#include <sys/mnttab.h>

typedef struct mnttab MountEntry;
#define mountPath mnt_mountp
#define mountReference mnt_special
#define mountType mnt_fstype
#define mountOptions mnt_mntopts

static FILE *
openMountsTable (int update) {
  FILE *table = fopen(MNTTAB, (update? "a": "r"));
  if (!table)
    logMessage((errno == ENOENT)? LOG_WARNING: LOG_ERR,
               "mounted file systems table open erorr: %s: %s",
               MNTTAB, strerror(errno));
  return table;
}

static void
closeMountsTable (FILE *table) {
  fclose(table);
}

static MountEntry *
readMountsTable (FILE *table) {
  static struct mnttab entry;
  if (getmntent(table, &entry) == 0) return &entry;
  return NULL;
}

static int
addMountEntry (FILE *table, MountEntry *entry) {
  errno = ENOSYS;
  if (!putmntent(table, entry)) return 1;
  logMessage(LOG_ERR, "mounts table entry add error: %s[%s] -> %s: %s",
             entry->mnt_fstype, entry->mnt_special, entry->mnt_mountp, strerror(errno));
  return 0;
}

#else /* mount paradigm */
#warning mounts table support not available on this platform

typedef struct {
  char *mountPath;
  char *mountReference;
  char *mountType;
  char *mountOptions;
} MountEntry;

static FILE *
openMountsTable (int update) {
  return NULL;
}

static void
closeMountsTable (FILE *table) {
}

static MountEntry *
readMountsTable (FILE *table) {
  return NULL;
}

static int
addMountEntry (FILE *table, MountEntry *entry) {
  return 0;
}
#endif /* mount paradigm */

#if defined(MNTOPT_RW)
#define MOUNT_OPTION_RW MNTOPT_RW
#else /* MOUNT_OPTION_RW */
#define MOUNT_OPTION_RW "rw"
#endif /* MOUNT_OPTION_RW */

char *
findMountPoint (MountPointTester test) {
  char *path = NULL;
  FILE *table;

  if ((table = openMountsTable(0))) {
    MountEntry *entry;

    while ((entry = readMountsTable(table))) {
      if (test(entry->mountPath, entry->mountType)) {
        if (!(path = strdup(entry->mountPath))) logMallocError();
        break;
      }
    }

    closeMountsTable(table);
  }

  return path;
}

static void updateMountsTable (MountEntry *entry);

static void
retryMountsTableUpdate (void *data) {
  MountEntry *entry = data;
  updateMountsTable(entry);
}

static void
updateMountsTable (MountEntry *entry) {
  int retry = 0;

  {
    FILE *table;

    if ((table = openMountsTable(1))) {
      addMountEntry(table, entry);
      closeMountsTable(table);
    } else if ((errno == EROFS) || (errno == EACCES)) {
      retry = 1;
    }
  }

  if (retry) {
    asyncRelativeAlarm(5000, retryMountsTableUpdate, entry);
  } else {
    if (entry->mountPath) free(entry->mountPath);
    if (entry->mountReference) free(entry->mountReference);
    if (entry->mountType) free(entry->mountType);
    if (entry->mountOptions) free(entry->mountOptions);
    free(entry);
  }
}

int
makeMountPoint (const char *path, const char *reference, const char *type) {
  if (mountFileSystem(path, reference, type)) {
    MountEntry *entry;

    logMessage(LOG_NOTICE, "file system mounted: %s[%s] -> %s",
               type, reference, path);

    if ((entry = malloc(sizeof(*entry)))) {
      memset(entry, 0, sizeof(*entry));

      if ((entry->mountPath = strdup(path))) {
        if ((entry->mountReference = strdup(reference))) {
          if ((entry->mountType = strdup(type))) {
            if ((entry->mountOptions = strdup(MOUNT_OPTION_RW))) {
              updateMountsTable(entry);
              return 1;
            } else {
              logMallocError();
            }

            free(entry->mountType);
          } else {
            logMallocError();
          }

          free(entry->mountReference);
        } else {
          logMallocError();
        }

        free(entry->mountPath);
      } else {
        logMallocError();
      }

      free(entry);
    } else {
      logMallocError();
    }
  } else {
    logMessage(LOG_ERR, "file system mount error: %s[%s] -> %s: %s",
               type, reference, path, strerror(errno));
  }

  return 0;
}
